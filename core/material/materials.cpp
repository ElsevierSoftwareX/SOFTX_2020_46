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
#include "materials.hpp"

#include <regex>
#include <unordered_map>


#include "nmtc.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/common/commoncards.hpp"
#include "core/io/input/original/original_metacard.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/time_utils.hpp"
#include "core/physics/physconstants.hpp"

#ifndef NO_ACEXS
#include "component/libacexs/libsrc/xsdir.hpp"
#endif
namespace {

const size_t MAX_POOL_SIZE = 100;
std::set<std::string> IMPLEMENTED_OPTS{"nlib", "plib", "pnlib", "elib", "hlib"};


// 引数*argsからmaterialパラメータを(GAS ESTEP HSTEP NLIB PLIB PNLIB ELIB HLIB COND)を削除し、
// パラメータ名、値のマップ(params)へ格納する。
void separateParams(std::vector<std::string> *args,
					std::unordered_map<std::string, std::string> *params)
{
	static std::vector<std::regex> paramRegs
	{
		std::regex("gas", std::regex_constants::icase),
		std::regex("estep", std::regex_constants::icase),
		std::regex("hstep", std::regex_constants::icase),
		std::regex("nlib", std::regex_constants::icase),
		std::regex("plib", std::regex_constants::icase),
		std::regex("pnlib", std::regex_constants::icase),
		std::regex("elib", std::regex_constants::icase),
		std::regex("hlib", std::regex_constants::icase),
        std::regex("cond", std::regex_constants::icase),
        std::regex("dedxfile", std::regex_constants::icase)
	};
	for(const auto &reg: paramRegs){
		for(auto itArgs = args->begin(); itArgs != args->end();) {
			if(std::regex_search(*itArgs, reg)) {

				std::vector<std::string> paramAndArg = utils::splitString(" =", *itArgs, true);
//                mDebug() << "paramandarg===" << paramAndArg;
                if(paramAndArg.size() != 2) {
                    throw std::invalid_argument(std::string("Invalid material parameter \"") + *itArgs + "\"");
                }
				params->emplace(std::make_pair(paramAndArg.at(0), paramAndArg.at(1)));
				itArgs = args->erase(itArgs);
			} else {
				++itArgs;
			}
		}
	}
}

}


mat::Materials::Materials(std::list<inp::DataLine> materialInput,
                          const std::string &xsdirFileName,
                          const std::vector<phys::ParticleType> &ptypes,
                          std::atomic_size_t &counter, bool verbose)
	: particleTypes_(ptypes)
{
// NO_ACEXSで使わない変数
#ifdef NO_ACEXS
    (void) xsdirFileName;
    (void) counter;
#endif
//    for(auto it = materialInput.begin(); it != materialInput.end(); ++it) {
//        mDebug() << "materialInputline ===" <<it->pos() << it->data;
//    }

    // NuclidePoolの容量は入力ファイル再読込直後にチェックして一定以上のサイズになっていたら全消去する。
    if(Nuclide::poolSize() > MAX_POOL_SIZE) Nuclide::clearPool();
	utils::SimpleTimer timer0;
	timer0.start();
    nameIdMap_.emplace(Material::VOID_NAME, Material::VOID_ID);
    nameIdMap_.emplace(Material::UNDEF_NAME,  Material::UNDEF_ID);

// NO_ACEXSが定義されている場合xsdir_は常にnullptrになる。
#ifndef NO_ACEXS
    utils::SimpleTimer timer;
    timer.start();
    std::shared_ptr<const ace::XsDir> xsdir(new ace::XsDir());
    try{
        /*
         * XsDirコンストラクタの
         * 第一引数はxsdirファイル名
         * 第二引数はntyの種類。空なら全NTY対象
         * 第三引数はaceファイル位置情報をよみとるかどうか
         */
        if(!particleTypes_.empty()) {
            std::vector<ace::NTY> ntyVec = phys::ptypesToNtys(particleTypes_);
            xsdir.reset(new ace::XsDir(xsdirFileName, ntyVec, true));
        } else {
            // awrテーブルしか読み取らない
            // TODO 第二引数がからだからwindowsでの読み取りが長いのか？
            xsdir.reset(new ace::XsDir(xsdirFileName, std::vector<ace::NTY>(), false));
        }
    } catch (std::invalid_argument &e) {
        // xsdirファイルが見つからない場合invalid_argが発生してここへ来る。
        mWarning("While reading xsdir file,", e.what(), ", No cross section files will be read.");
        particleTypes_.clear();
    }
    xsdir_ = xsdir;
    timer.stop();
    if(verbose) mDebug() << "Reading xsdir file done in " << timer.msec() << "(ms)";
#endif

    for(auto &mi: materialInput) {
		utils::tolower(&(mi.data));
        //mDebug() << "material input===" << mi.data;
	}



	// ################ ここからmaterialのマップを作成する。

	/*
	 * xsdirは空の場合と未定義の場合がある。前者は正常処理で後者はイレギュラー。
	 * ・xsdirが空(ptypeが空のため)→ mat::Material::createMaterialで対処する。
	 * ・xsdirが未定義→materialmapも空を返す。
	 */
    //xsdir_ = xsdirFuture.get(); // 結局xsdirの遅延はここまでしか粘れない


	// この関数が呼ばれる段階ではすでにセクション入力は１材料１行状態になっている。
	/*
	 * Materialカードで指定されるIDは必ずしもフルセットのIDではなく、
	 * identifierとclassが省略されている場合がある。
	 * その場合identifierはxsdirから探すことになるが、そのときには既に
	 * classが必要になる。で、classはどうするかと言うとptypesから決定する。
	 * まとめると
	 * 1. ptypesからclassを決定する。
	 * 2. xsdirから、最初にclassが該当するACEファイルを求める
	 */
//    for(auto it = materialInput.begin(); it != materialInput.end(); ++it) {
//        mDebug() << "materialInputline ===" <<it->pos() << it->data;
//    }

// NO_ACEXSが定義されている場合、Materialクラス側でダミーデータを作成する。
	for(auto it = materialInput.begin(); it != materialInput.end(); ++it) {
        std::string materialStr = it->data;
        // pass mt card
        if(std::regex_search(materialStr, std::regex(R"(^ {0,4}[mM][tT])"))) continue;

		std::string idStr = inp::comm::GetMaterialIdStr(materialStr);  // idStrはマテリアル名。M1 の1の部分あるいはMAT[2]の2
        //mDebug() << "idStr===" << idStr << "materialStr ===" << materialStr;
        // GetMaterialIdStr
        if(idStr.empty()) {
            mWarning(it->pos()) << materialStr << " is not a valid material card.";
            continue;
        }
        /*
         * mat[ 6 ] h 2 o 1 のような入力は解釈しやすいように
         * m6 h 2 o 1 のように置き換える。
         */
        if(materialStr.find_first_of("[") != std::string::npos) {
            std::string::size_type pos = materialStr.find_first_of("]");
            if(pos == std::string::npos) {
                throw std::invalid_argument(it->pos() + " bracket mismatched, \"]\" not found.");
            }
            materialStr = "m" + idStr + " " + materialStr.substr(pos+1);
        }
        if(idStr.empty()) throw std::invalid_argument(it->pos() + " Material name is empty or invalid. name =" + idStr);
		auto matLine = *it;
		// 拡張入力(BD=のビルドアップ係数)がないか調べる
		//++it;
		std::string bfName;
		static std::regex bfRegex(R"(^ *[bB][fF] *= *([\w_]+) *$)");
		if(it != --materialInput.end()) {
			++it;
			std::smatch sm;
			if(std::regex_search(it->data, sm, inp::org::getOriginalCommandPattern())) {
				std::string exCommand = sm.str(1);
				if(std::regex_search(exCommand, sm, bfRegex)) {
					bfName = sm.str(1);
				}
				++it;
			}
			--it;
		}


		// M1のとかの部分は処理できた。以降は核種データに読み込み
		auto materialVec = utils::splitString(" ", materialStr, true);
		if(materialVec.size() < 2) {
            throw std::invalid_argument(matLine.pos() + " Too few material input, data = " + materialStr);
		}



        // Input data separated with equals and spaces like "nlib = 51c" are splited to {"nlib", "=", "51c"} by utils::SplitString.
        // These data should be concatinated again.
        for(auto matIt = materialVec.begin(); matIt+1 != materialVec.end();++matIt) {
            if((matIt+1)->front() == '=') {
                *matIt = *matIt + *(matIt+1);
                materialVec.erase(matIt+1);
            }
            if(matIt->back() == '=' && matIt+1 != materialVec.end()) {
                *matIt = *matIt + *(matIt+1);
                materialVec.erase(matIt+1);
            }
            if(matIt == materialVec.end()) break;
        }


		// ここから核種データ作成。materialVec.at(0)はM1 とかMAT[1]が入っている。のでそれ以外の部分を取得
        std::vector<std::string> nuclideParams(materialVec.begin()+1, materialVec.end());

        // ここでパラメータ(GAS ESTEP HSTEP NLIB PLIB PNLIB ELIB HLIB COND DEDXFILE)を分離する
        std::unordered_map<std::string, std::string> opts;
        separateParams(&nuclideParams, &opts);


        if(!opts.empty()) {
            // 実装済みoptionについては警告しない。
			size_t counter2 = 0;
			std::stringstream ss;
            for(auto it = opts.begin(); it != opts.end(); ++it, ++counter2) {
                if(IMPLEMENTED_OPTS.find(it->first) == IMPLEMENTED_OPTS.end()) {
                    ss << it->first << "=" << it->second;
                }
                if(counter2 != opts.size()-1) ss << ", ";
			}
            mWarning() << "Material parameters(" << ss.str() << ") are not implemented";
		}
		// マテリアルカードのパラメータを削除した後は入力は偶数個でなければならない。
		if(nuclideParams.size()%2 != 0 || nuclideParams.empty()) {
            if(verbose)mDebug() << "nuclide parametes ===" << nuclideParams;
            throw std::invalid_argument(matLine.pos() + " Number of input parameters should be non-zero even.");
		}

		// ここで核種名のNMTC方式 C 12C 等をZAIDに変換する。
//        mDebug() << "nuclideParams ===" << nuclideParams;
		convertNmtcStyle(&nuclideParams);
//        mDebug() << "idStr=======" << idStr << "params===" << nuclideParams;
		try {
			// ここでidStrが重複していたら例外発生
			if(materialMapByName_.find(idStr) != materialMapByName_.end()) {
				throw std::invalid_argument("Material name " + idStr + " is duplicated." );
			}

            // xsdirの遅延は最大限粘ってもここまでくらいなのでfutureで遅延させても余り効果はでない。
            materialMapByName_.emplace(
                        idStr, mat::Material::createMaterial(
                            idStr, materialMapByName_.size(), xsdir_, particleTypes_, nuclideParams, opts, bfName
                            )
                        );
			nameIdMap_.emplace(idStr, materialMapByName_.size());
		} catch (std::out_of_range &oor) {
			throw std::runtime_error(std::string(matLine.pos() + " " + oor.what()));
		}catch (std::exception &e) {
			throw std::runtime_error(std::string(matLine.pos() + " " + e.what()));
		}
		++counter;
	}  // Materialカード終わり



//	for(auto it = materialMap.begin(); it != materialMap.end(); ++it) {
//		mDebug() << "matID===" << it->first << "matName=" << it->second->name() << "buildup=" << it->second->bfName();
//	}
	timer0.stop();
    if(verbose) mDebug() << "Reading xs files done in " << timer0.msec() << "(ms)";

}

const std::shared_ptr<const ace::XsDir> &mat::Materials::xsdir() const {return xsdir_;}



int mat::Materials::nameToId(const std::string &name)
{
	auto itr = nameIdMap_.find(name);
	return (itr != nameIdMap_.end()) ? itr->second : -1;
}

std::string mat::Materials::idToName(const int id)
{
	for(auto itr = nameIdMap_.begin(); itr != nameIdMap_.end(); ++itr) {
		if(itr->second == id) return itr->first;
	}
	return std::string();
}

void mat::Materials::clear()
{
	nameIdMap_.clear();
	nameIdMap_.emplace(Material::VOID_NAME, Material::VOID_ID);
	nameIdMap_.emplace(Material::UNDEF_NAME,  Material::UNDEF_ID);
}
