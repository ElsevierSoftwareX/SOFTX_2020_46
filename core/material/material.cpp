/*!
 * gxsview version 1.2
 *
 * Copyright (c) 2020 Ohnishi Seiki and National Maritime Research Institute, Japan
 *
 * Released under the GPLv3
 * https://www.gnu.org/licenses/gpl-3.0.txt
 *
 * If you need to distribute with another license,
 * ask ohnishi@m.mpat.go.jp
 */
#include "material.hpp"

#include <algorithm>
#include <cstdlib>
#include <regex>
#include <stdexcept>
#include <vector>

#include "core/io/input/dataline.hpp"
#include "core/utils/numeric_utils.hpp"
#include "core/utils/utils.hpp"
#include "core/utils/system_utils.hpp"
#include "core/formula/fortran/fortnode.hpp"

#ifndef NO_ACEXS
#include "component/libacexs/libsrc/xsdir.hpp"
#endif

namespace {

#ifndef NO_ACEXS
ace::NTY particleTypeToNty(phys::ParticleType ptype)
{
	if(ptype == phys::ParticleType::PHOTON) {
		return ace::NTY::CONTIUNOUS_PHOTOATOMIC;
	} else if (ptype == phys::ParticleType::NEUTRON) {
		return ace::NTY::CONTINUOUS_NEUTRON;
	} else {
		std::cerr << "ProgramError: particle type other than neutron and photon is not implemented yet." << std::endl;
		abort();
	}
}
#endif

}

const char mat::Material::UNDEF_NAME[] = "*M_undef*";
const char mat::Material::VOID_NAME[] = "*M_void*";
const char mat::Material::BOUND_NAME[] = "*M_bound*";
const char mat::Material::UBOUND_NAME[] = "*M_ubound*";
const char mat::Material::DOUBLE_NAME[] = "*M_double*";
const char mat::Material::OMITTED_NAME[] = "*M_omitted*";
constexpr int mat::Material::VOID_ID;
constexpr int mat::Material::UNDEF_ID;





const std::shared_ptr<const mat::Material> mat::Material::void_material_ptr()
{
	static std::shared_ptr<const Material> voidMatPtr = std::make_shared<const Material>();
	return voidMatPtr;
}


// ptypesが空ならば断面積データは読み込まない
std::shared_ptr<const mat::Material> mat::Material::createMaterial(const std::string &idStr,
																   std::size_t id,
																   const std::shared_ptr<const ace::XsDir> &xsdir,
																   const std::vector<phys::ParticleType> &ptypes,
																   const std::vector<std::string> &nuclideParams,
                                                                   const std::unordered_map<std::string, std::string> &opts,
																   const std::string buildupFactorName)
{
// NO_ACEXSが定義されている場合、パラメータは全て無視してvoidマテリアルを返す。
#ifndef NO_ACEXS
    assert(xsdir);  // xsdirは未割り当てではいけない(aceを使うなら)

	/*
	 * ptypesが空ならばnuclidesは空の値が入り、mat::MaterialはIDだけの空のmaterialが返る。
	 * この時はxsdirは使用されないのでxsdirは空でも良い。
	 *
	 * ptypesとxsdirの整合性を取る必要があるのは設計が良くない。
	 *
	 * ptypesが非emptyでxsdirがemptyの場合多分バグるのでとりあえずチェックしておく。
	 * TODO sourceセクションが定義済みなのにxsdirが未定義の場合問題になる。
	 */
	//assert(ptypes.empty() || !xsdir->empty());
	if(!ptypes.empty() && xsdir->empty()) {
        std::stringstream ss;
        ss << "No xs file was found in the xsdir file for patricle(s) = {";
        for(size_t i = 0; i < ptypes.size(); ++i) {
            ss << phys::particleTypeTostr(ptypes.at(i));
            if(i != ptypes.size()-1) ss << ", ";
        }
        ss << "}";
        throw std::invalid_argument(ss.str());
	}

	// Xsdir::datapathは環境変数の読み取りもサポートしている。
	std::string datapath = xsdir->datapath();

	// particleをキーにして核種スマポと存在比のペアを保存する。
	std::unordered_multimap<phys::ParticleType, std::pair<std::shared_ptr<const Nuclide>, double>> nuclides;
	for(size_t i = 0; i < nuclideParams.size(); i += 2) {
        std::string nuclideName = nuclideParams.at(i);
//        double abundance = utils::stringTo<double>(nuclideParams.at(i+1));
        double abundance = fort::eq(nuclideParams.at(i+1));
//		mDebug() << "nuclideName===" << nuclideName << "abundance===" << abundance;
		/*
		 * ここで 核種名と存在量が与えられる。
		 * ・問題1：ZAIDのidentifier,classが省略されている場合。→ 最初に見つかったライブラリを読み込む
		 * ・問題2：フルZAIDが与えられている場合でも、ptypesに存在する粒子種の分は読み込む必要がる。
		 *			→ フルZAIDの分は指定されたテーブルを、それ以外の粒子についてはidentifier, classなしとして最初に見つかったライブラリを読み込む
		 */
		/*
		 * ZAIDは
		 * ZZZAAA.ddC
		 * Z:atomic number、A:mass number, d:identifier, c:class
		 *
		 * SZAX(拡張ZAID)
		 * SSSZZZAAA.dddCC
		 * S:excited state, Z:atomic number, A:mass number, d:identifier, c:class
		 * ※SZAXのclassはZAIDのクラスを拡張していて非互換。
		 *
		 */
		for(phys::ParticleType ptype: ptypes) {
			// とりあえず中性子と光子以外はエラーにしておく
			if(ptype != phys::ParticleType::NEUTRON && ptype != phys::ParticleType::PHOTON) {
				std::string message = "Reading cross sections for this kind of particle is not implmented. particle = ";
				throw std::invalid_argument(message + "\"" + phys::particleTypeTostr(ptype) + "\"");
			}
			ace::NTY nty = particleTypeToNty(ptype);
			std::string targetID;

			if(ace::isZAIDX(nuclideName) || ace::isSZAX(nuclideName)) {
			// nuclideNameがZAID/SZAXの場合、ntyはclassから読み取る
//                mDebug() << "nty===" << ace::ntyToString(nty) << "class str===" << ace::getClassStr(nuclideName)
//                         << "classNTY===" << ace::ntyToString(ace::classStrToNty(ace::getClassStr(nuclideName)));
				// ptypeから指定されるntyとclassから読み取ったntyが同じならフルのzaid/szaxでxsdirを検索
				// ptypeがneutronでntyがドシメトリの場合も例外的にフルのZAID/SZAXでxsdirを検索
				ace::NTY classByNuclideName = ace::classStrToNty(ace::getClassStr(nuclideName));
				if((classByNuclideName == nty)
				|| ((classByNuclideName == ace::NTY::DOSIMETRY) && (ptype == phys::ParticleType::NEUTRON))  ) {
					targetID = nuclideName;
				} else {
					// classで指定されている粒子種以外はZAIDではなくZAでxsdirを検索
					// 但しPhotoatomicに関してはZAではなくZで検索する。
					auto za = ace::getZaStr(nuclideName);
					if(nty == ace::NTY::CONTIUNOUS_PHOTOATOMIC) {
						za = za.substr(0, za.size()-3) + "000";
					}
					targetID = za;
				}
			} else {
                // 光子(photoatomic)の場合26056も26054も26000にまとめられる。
                if(nty == ace::NTY::CONTIUNOUS_PHOTOATOMIC) {
                    //mDebug() << "photonはZでアクセス za=" << nuclideName << "z=" << nuclideName.substr(nuclideName.size()-3);
                    targetID = nuclideName.substr(0, nuclideName.size()-3) + "000";
                } else {
                    targetID = nuclideName;
                }
                //mDebug() << nuclideName << " is not full ZAID/SZAX";

                /*
                 *  nuclideNameが完全なZAIDXではない(class, identifierが無いZA）場合
                 * オプションでidentifier, classが指定されていればそれを使う。
                 * なければZA=nuclideNameで検索
                 * NLIB PLIB PNLIB ELIB HLIB
                 */
                if(nty == ace::NTY::CONTINUOUS_NEUTRON && opts.find("nlib") != opts.end()) {
                    targetID += "." + opts.at("nlib");
                } else if (nty == ace::NTY::CONTIUNOUS_PHOTOATOMIC && opts.find("plib") != opts.end()) {
                    targetID += "." + opts.at("plib");
                } else if (nty == ace::NTY::PHOTONUCLEAR && opts.find("pnlib") != opts.end()) {
                    targetID += "." + opts.at("pnlib");
                } else if (nty == ace::NTY::ELECTRON && opts.find("elib") != opts.end()) {
                    targetID += "." + opts.at("elib");
                } else if(nty == ace::NTY::PROTON && opts.find("hlib") != opts.end()) {
                    targetID += "." + opts.at("hlib");
                }
			}
			// ここまででtargetのIDが確定。
			ace::XsInfo info = xsdir->getNuclideInfo(targetID, nty);

			// Type2 Ace(バイナリフォーマット)には未対応なので例外発生にする。
			// 中途半端に核種を読み込むとMaterialレベルのfractionが合わなくなるなどの不都合が生じる。
			if(info.filetype == 2) {throw std::runtime_error("Binary ACE file (type = 2) is not supported yet.");}

			// 核種が重複する場合はmapに追加するのではなくabundanceを足し算する。
			auto range = nuclides.equal_range(ptype);
			bool hasEmplaced = false;
			for(auto it = range.first; it != range.second; ++it) {
				if(it->second.first->zaid() == info.tableID) {
					it->second.second += abundance;
					hasEmplaced = true;
				}
			}
			if(!hasEmplaced) {
				//mDebug() << "Start reading Acefile, ID===" << info.tableID << "file=" << datapath + PATH_SEP + info.filename;
				utils::SimpleTimer timer;
				timer.start();
                std::shared_ptr<const Nuclide> nuc
                        = Nuclide::createNuclide(datapath + PATH_SEP + info.filename,
                                                 info.awr, info.tableID, info.address);
				nuclides.emplace(ptype, std::make_pair(nuc, abundance));
				timer.stop();
				mDebug() << "Nuclide data from " << info.tableID << "for ZAIDX =" << info.tableID << "constructed in " << timer.msec() << "(ms)";
			}
		}  // ここまでptypesループ
	}



	/*
	 *  nuclidesには{ParticleType, pair<shared<Nuclide>,abundance>} が入っているので
	 * これをMaterialの内部表現に合わせる。
	 */

	// まずabundanceが1になるように規格化
	std::unordered_map<phys::ParticleType, double> totalAbundances;
	for(auto ptype: ptypes) {
		totalAbundances[ptype] = 0;  // 核種ごとの合計値
	}
	for(auto it = nuclides.begin(); it != nuclides.end(); ++it) {
		// it->firstがparticle
		totalAbundances.at(it->first) += (it->second).second;
	}
	for(auto totalPair: totalAbundances) {
		if(utils::isSameDouble(totalPair.second, 0)) {
//			mDebug() << "fraction====" << totalPair.second;
			throw std::invalid_argument(std::string("Total fraction is zero in material ") + idStr);
		} else if(!utils::isSameDouble(std::abs(totalPair.second), 1)) {
			mWarning() << "In material" << idStr << "for" << phys::particleTypeTostr(totalPair.first)
			           << ", Sum of each nuclides' fraction is not 1 in material\"" << idStr << "\""
			           << " , sum = " << std::abs(totalPair.second) << ", normalized.";
		}
	}
	for(auto it = nuclides.begin(); it != nuclides.end(); ++it) {
		(it->second).second /= totalAbundances.at(it->first);
	}  // 規格化終わり。



	/*
	 * FIXME mode Pの時、6012, 6013両者とも6000として扱っているが、(密度が正、即ち1e24 g/atomで入力している場合)実際には原子量を考慮して密度換算する必要がある。
	 * これは密度を正の値(個数密度)で与えた時に問題となる。
	 * 実際には6012は6000より軽く、 6013は6000よりも重い。
	 * "6012 0.989  6013 0.011" の時だいたい一致する。
	 */



	// さらにabundanceが負の場合は原子数比に変換する。
	// NOTE あとでの使用を考えるとnuclides.secondは質量割合にしたほうが良いかも???
	for(auto it = totalAbundances.begin(); it != totalAbundances.end(); ++it) {
		if(it->second < 0) {
			auto particleType = it->first;
			double newTotal = 0;
			for(auto nit = nuclides.begin(); nit != nuclides.end(); ++nit) {
				if(nit->first == particleType) {
					newTotal += (nit->second).second * (nit->second).first->awr();
				}
			}
			for(auto nit = nuclides.begin(); nit != nuclides.end(); ++nit) {
				if(nit->first == particleType) {
					(nit->second).second /= newTotal;  // newTotalもsecondも負だからこれで正になる
				}
			}
		}
	}

	// ipモードではptypesが空になっているので、totalAbundances及びnuclidesも空のままとなっている。

    return std::make_shared<const mat::Material>(idStr, id, nuclides, buildupFactorName);
#else
    (void) idStr;
    (void) id;
    (void) xsdir;
    (void) ptypes;
    (void) nuclideParams;
    (void) opts;
    (void) buildupFactorName;

    return std::make_shared<const mat::Material>(idStr, id);
#endif
}


bool isMaterialTitle(const std::string str)
{
	std::string argstr(str);
	utils::toupper(&argstr);
	auto inputVec = utils::splitString(" ", argstr, true);
	if(inputVec.empty()) return false;  // 空白行はMaterialタイトルではない
	return inputVec.at(0).substr(0,1) == "M";
}





// nuclidesはparticleをキーにして、 shared<Nuclide>と質量割合のペアを格納
mat::Material::Material(const std::string &name, int id,
						const nuclide_map_type &nuclides,
						const std::string &bfName)
	: id_(id), name_(name), buildupFactorName_(bfName)
{
	// nuclides_ は高速アクセスできるようにvectorなのでまずnuclideが全粒子データを格納できるように拡張する。
	int maxParticleCount = 0; // 粒子種数
	for(auto it = nuclides.begin(); it != nuclides.end(); ++it) {
		int particleNumber = static_cast<int>(it->first);
		if(maxParticleCount < particleNumber) maxParticleCount = particleNumber;
	}
	nuclides_.resize(maxParticleCount + 1);
	for(auto it = nuclides.begin(); it != nuclides.end(); ++it) {
		// it->first はParticleType, secondはpair<shared_ptr<const Nuclide>, double>
		nuclides_.at(static_cast<int>(it->first)).emplace_back(it->second);
	}

	// sourceなしやipモードで表示のみの場合はnuclidesが空
	if(!nuclides.empty()) {
		// 平均原子量を計算
		// nuclide_map_type はmultimap<ParticleType, pair<shared_ptr<Nuclide>, double>>
		auto ptype = nuclides.begin()->first;  // 最初の粒子種を使って計算する。
		auto range = nuclides.equal_range(ptype);
		double total = 0;
		for(auto it = range.first; it != range.second; ++it) {
			//mDebug() << "abandance =" << it->second.second << "nucmass=" << it->second.first->awr();
			total += it->second.second * it->second.first->awr();
		}
		//mDebug() << "total==" << total << "nmass=" << phys::NEUTRON_MASS_AMU;
		averageNuclideMassAmu_ = total*phys::NEUTRON_MASS_AMU;
	} else {
		// 原子質量はACEファイル記載の値を使用しているので、--no-xsにはamuが不明になる。
		// 不明時は負の値を返すこととする。
		averageNuclideMassAmu_ = -1;
	}

}

constexpr double NAbarn = phys::NA*1e-24; // NA * 1e-24
double mat::Material::macroTotalXs(phys::ParticleType ptype, double dens, double energy) const
{
#ifndef NO_ACEXS
	if(dens < 0) {
        throw std::invalid_argument("ProgramError, negative density is set to calculate macroscopic total xs.");
	}
	double microTotalXs = 0;
	int paritcleIndex = static_cast<int>(ptype);
	if(nuclides_.empty()) return 0;  // voidなら核種データなしなのでxsは0
	//mDebug() << "AverageNuclideMass=" << averageNuclideMassAmu_;
	for(auto &nuclidePair: nuclides_[paritcleIndex]) {
		try {
//			mDebug() << "nuclide=" << nuclidePair.first->zaid() << "abandance(num)=" << nuclidePair.second
//			         << "abandance(wt)=" << nuclidePair.second*nuclidePair.first->awr()*phys::NEUTRON_MASS_AMU/averageNuclideMassAmu_
//			         << "E=" << energy << "totXs=" << nuclidePair.first->getTotalXs(energy);
			microTotalXs += nuclidePair.first->getTotalXs(energy)
			                *nuclidePair.second/averageNuclideMassAmu_;
		} catch (std::out_of_range &oor) {
            throw std::invalid_argument(std::string(oor.what()) + ", in nuclide =" + nuclidePair.first->zaid() + ", material =" + name_);
		}
	}
	return NAbarn*dens*microTotalXs;
#else
    (void) ptype;
    (void) dens;
    (void) energy;
    return 0;
#endif
}

//bool mat::Material::isReservedName(const std::string matName)
//{
//    if(matName.empty()) throw std::invalid_argument("empty material name");

//    return matName.front() == '*';
//}



