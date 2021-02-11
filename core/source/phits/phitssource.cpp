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
#include "phitssource.hpp"

#include <functional>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

#include "phitscylindersource.hpp"
#include "core/source/multigroupdistribution.hpp"
#include "core/source/proportionaldistribution.hpp"
#include "core/source/discretedistribution.hpp"
#include "core/io/input/mcmode.hpp"
#include "core/io/input/common/trcard.hpp"
#include "core/io/input/common/commoncards.hpp"
#include "core/io/input/phits/phitsinputsection.hpp"
#include "core/io/input/phits/phits_metacards.hpp"
#include "core/utils/utils.hpp"
#include "core/utils/numeric_utils.hpp"
#include "core/physics/particle/uncollidedphoton.hpp"
#include "core/math/nvector.hpp"
#include "core/math/constants.hpp"
#include "core/geometry/cell/cell.hpp"

namespace {
// ソース作成関数は、Tr行列マップ、セルスマポマップ、 Phitsセクションを引数にとってphitsソースのスマポを返す
typedef std::function<std::unique_ptr<src::PhitsSource>(const std::unordered_map<size_t, math::Matrix<4>>&,
					   const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>>&,
					   const inp::phits::PhitsInputSection&)> src_creator_type;

src_creator_type getCreator(int type) {
	namespace sp = std::placeholders;
	static 	std::unordered_map<int, src_creator_type> cmap {
		{1, std::bind(src::PhitsCylinderSource::create, sp::_1, sp::_2, sp::_3, false)},
		{4, std::bind(src::PhitsCylinderSource::create, sp::_1, sp::_2, sp::_3, true)},
//		{2, std::bind(src::PhitsRectangularSource::create, sp::_1, sp::_2, sp::_3, false)},
//		{5, std::bind(src::PhitsRectangularSource::create, sp::_1, sp::_2, sp::_3, true)},
	};
	if(cmap.find(type) == cmap.end()) {
        // 未実装の部分は警告出して常にnullptr返す関数を返す
        mWarning("Source type=" + std::to_string(type) + " is not implemented");
        //throw std::invalid_argument("Source type=" + std::to_string(type) + " is not implemented");
        return [](const std::unordered_map<size_t, math::Matrix<4>>& matm,
                const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>>& cm,
                const inp::phits::PhitsInputSection& ps){
                    (void)matm; (void)cm; (void) ps;
                    return nullptr;
                };
	}
	return cmap.at(type);
}

}

// 単一のソースを作成する。後でfactor調整する可能性があるので、ここではptr<const source>ではない。
std::unique_ptr<src::PhitsSource>
	src::PhitsSource::createSingleSource(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
									const std::unordered_map<std::string, std::shared_ptr<const geom::Cell> > &cellMap,
									const inp::phits::PhitsInputSection &inputSection)
{

//	assert(!inputSection.input().empty());
    // 最初にS-typeを確定させる。
	std::regex stypePattern(R"(s-type *= *([1-9]+))");
    std::smatch sm;
    int stypeValue = 0;
    for(auto &dataline: inputSection.input()){
        if(std::regex_search(dataline.data, sm, stypePattern)) {
            assert(sm.size() == 2);
            stypeValue = utils::stringTo<int>(sm.str(1));
            break;
        }
    }
    if(stypeValue == 0) {
        throw std::invalid_argument(inputSection.input().front().pos() +  " Input section has no s-type value.");
    }

	return getCreator(stypeValue)(trMap, cellMap, inputSection);
}




std::string src::PhitsSource::toString() const
{
	std::stringstream ss;
	for(auto &x: parameterMap_) {
		ss << std::setw(10) << x.first  << " = " << x.second << std::endl;
	}
//    ss << "Particle type = " << particleType_ << std::endl;
    ss << "Factor = " << factor_ << std::endl;
	ss << "Energy distribution\n" << energyDistribution_->toString() << std::endl;
	ss << "Angular distribution\n" << angularDistribution_->toString() << std::endl;

	int dim = 0;
    for(auto &x: spatialDistributions_) {
		ss << "Spatial distribution in direction " << ++dim << "\n" << x->toString() << std::endl;
	}

	return ss.str();
}


/*
 * 無衝突線束算出粒子は
 * １．発生位置確定
 * ２．検出位置確定
 * ３．角度確定
 * ４．エネルギー確定
 * しないと発生させられない。
 */

// 設定された分布に従って、代表的な点から粒子を発生させる。
std::vector<std::shared_ptr<phys::UncollidedPhoton> > src::PhitsSource::generateUncollideParticles(const math::Point &detPoint,
											 const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cellList,
											 bool recordEvent) const
{
	// 設定済みかチェック。
	if (!detPoint.isValid()) {
        throw std::invalid_argument("Detection point(Tallying point) is not defiend.");
    } else if(energyDistribution_ == nullptr) {
        throw std::invalid_argument("Energy destribution is not defined.");
    } else {
        for(auto &element: spatialDistributions_) {
            if(element == nullptr) {
                throw std::invalid_argument("Spatial distribution is not defined.");
            }
        }
    }

	// meshが分布をカバーしているかは、粒子発生時に総確率が1(というかfactor_)になっているかでチェックする。
    // エネルギー。Eが離散分布なら代表点が定義されていないので注意。
	std::vector<double> energyPoints = energyMesh_.centers();
	std::vector<double> energyProbs = energyDistribution_->getProbabilities(energyPoints, energyMesh_.widthPairs());
	assert(energyPoints.size() == energyProbs.size());

	std::array<std::vector<double>, 3> coordinates;
	std::array<std::vector<double>, 3> spatialProbs;
    for(size_t i = 0; i < spatialProbs.size(); ++i) {
		coordinates[i] = spatialMeshes_[i].centers();
        spatialProbs[i]
				= spatialDistributions_[i]->getProbabilities(coordinates[i],
															 spatialMeshes_[i].widthPairs());
    }

	mDebug() << "\nemesh=" << energyMesh_.toString();
	mDebug() << "Epoints=" <<energyPoints << "probs=" << energyProbs;
	mDebug() << "npos0mesh=" << spatialMeshes_[0].toString();
	mDebug() << "pos0s=" << coordinates[0] << "probs=" << spatialProbs[0];
	mDebug() << "npos1mesh=" << spatialMeshes_[1].toString();
	mDebug() << "pos1s=" << coordinates[1] << "probs=" << spatialProbs[1];
	mDebug() << "npos2mesh=" << spatialMeshes_[2].toString();
	mDebug() << "pos2s=" << coordinates[2] << "probs=" << spatialProbs[2];
	mDebug();



	// factor_はソースのfactorや<source>,totfactorなどを考慮済み。のはず
	std::vector<std::shared_ptr<phys::UncollidedPhoton>> particles;
	double totalProb = 0, totalRejectedProb = 0;
	// ここからEループ
	for(size_t eindex = 0; eindex < energyProbs.size(); ++eindex) {
		const double energy = energyPoints.at(eindex);
		const double energyProb = energyProbs.at(eindex); //エネルギーがenergyの粒子発生確率
		// ここから位置座標に関するループ
		for(size_t i = 0; i < spatialProbs[0].size(); ++i) {
			double x1Prob = spatialProbs[0].at(i);  //
			for(size_t j = 0; j < spatialProbs[1].size(); ++j) {
				double x2Prob = spatialProbs[1].at(j);
				for(size_t k = 0; k < spatialProbs[2].size(); ++k) {
					double x3Prob = spatialProbs[2].at(k);
					//mDebug() << "probs e, p1, p2, p3=" << energyProbs.at(eindex) << spatialProbs[0].at(i) << spatialProbs[1].at(j) << spatialProbs[2].at(k);

					// phitsのソースはxyzEθ全て座標が分離しており、それぞれの確率の積で粒子発生確率を記述できる。
					double spacialProb = x1Prob*x2Prob*x3Prob;
					// 空間分布からxyz位置への変換が必要。
					math::Point point = this->toCartesian(coordinates[0].at(i), coordinates[1].at(j), coordinates[2].at(k));
					math::Vector<3> dir = (detPoint - point).normalized();
					// ここでdirを実現する確率をangularDistribution_から求める
					auto cosine = math::dotProd(dir, this->referenceDir_.normalized());
					// 角度方向確率 getPdf(cos)がθ方向の分布確率、1/2πがφ方向の分布確率(phitsではφ方向は常に均一等方分布)
					auto angularProb = angularDistribution_->getPdf(cosine)*0.5*math::INV_PI ;
					double spaceEnergyPprob = energyProb*spacialProb;
					if(isInAcceptedCells(point)) {
						/*
						 * ここまででxyz,E,θそれぞれの方向の確率が求まったのでそれぞれの方向での合計を保存して
						 * 離散化した範囲が確率分布の全体をカバーしているかチェックする。
						 */
						totalProb += spaceEnergyPprob;
						/*
						 *  totalProbはメッシュが線源領域全体をカバーしているかチェックするためのものなので
						 *  angularPdfはtotalProbには加えない。
						 */

						mDebug() << "Cell accepted point =" << point << ", dir =" << dir << ", E =" << energy
								 << ", p(E) =" << energyProb << ", p(x) =" << spacialProb << "p(angular)=" <<angularProb
								 << ", weight = "<< spaceEnergyPprob*angularProb;
						double length = (detPoint - point).abs();
						particles.emplace_back(std::make_shared<phys::UncollidedPhoton>(
												   spaceEnergyPprob*angularProb*factor_, point, dir, energy, nullptr, cellList, length,  recordEvent, false));
					} else {
						// セルで棄却された場合はこちら。
						mDebug() << "Cell rejected point =" << point;
						totalRejectedProb += spaceEnergyPprob;
					}
				}
			}
		}  // end spacial variable loop
	}  // end energy loop

	mDebug() << "Total (accepted+rejected) probability = " << totalProb + totalRejectedProb;
	if(!utils::isSameDouble(totalProb + totalRejectedProb, 1.0)) {
        throw std::invalid_argument(std::string("Total (accepted+rejected) probability is not equal to 1.\n")
                 + "Meshes may not cover whole source distributions in energy or space.\n"
                 + "Actual probability = " + std::to_string(totalProb+totalRejectedProb));
	} else if (utils::isSameDouble(totalProb, 0)) {
        throw std::invalid_argument("No particle was generated. Cell does not contain source region?");
	}
	// acceptedされた分のトータルが1になるように再度規格化
	for(auto &p: particles) {
		p->setWeight(p->weight()/totalProb);
	}
	return particles;
}


std::vector<math::Point> src::PhitsSource::sourcePoints() const
{
	std::vector<math::Point> points;
	std::array<std::vector<double>, 3> coordinates;
	for(size_t i = 0; i < coordinates.size(); ++i) {
		coordinates[i] = spatialMeshes_[i].centers();
	}
//	mDebug() << "pos0mesh=" << spatialMeshes_[0].toString();
//	mDebug() << "pos0s=" << coordinates[0];
//	mDebug() << "pos1mesh=" << spatialMeshes_[1].toString();
//	mDebug() << "pos1s=" << coordinates[1];
//	mDebug() << "pos2mesh=" << spatialMeshes_[2].toString();
//	mDebug() << "pos2s=" << coordinates[2];
	for(size_t i = 0; i < coordinates[0].size(); ++i) {
		for(size_t j = 0; j < coordinates[1].size(); ++j) {
			for(size_t k = 0; k < coordinates[2].size(); ++k) {
				//mDebug() << "probs e, p1, p2, p3=" << energyProbs.at(eindex) << spatialProbs[0].at(i) << spatialProbs[1].at(j) << spatialProbs[2].at(k);
				// 空間分布からxyz位置への変換が必要。
				math::Point point = this->toCartesian(coordinates[0].at(i), coordinates[1].at(j), coordinates[2].at(k));
				if(isInAcceptedCells(point)) {
					//mDebug() << "srcpos=" << point;
					points.emplace_back(point);
				}
			}
		}
	}
	return points;
}

//  ソース読み取り。ソースセクションは複数有り得て、ソース内でさらにマルチソース入力があり得る
std::vector<std::shared_ptr<const src::PhitsSource>>
	src::PhitsSource::createSources(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
									 const std::unordered_map<std::string,std::shared_ptr<const geom::Cell>> &cellMap,
									 const inp::phits::PhitsInput &phitsInp,
									 const conf::Config &config)
{
	namespace ip = inp::phits;
	std::vector<std::shared_ptr<const src::PhitsSource>> sources;
	// sourceInputLists.size()はソースセクションの数に等しい。
	std::vector<std::list<inp::DataLine>>  sourceInputLists = phitsInp.sourceSections();
	// ソースセクションはcreateSources内でPhitsInputSectionを経由するので小文字化はここで考えなくて良い。
	if(sourceInputLists.empty()) {
		mWarning() << "No source section(s) found.";
		return sources;
	}

	for(auto &srcInput: sourceInputLists) {
		auto tmpSrc = src::PhitsSource::createMultiSource(trMap, cellMap, srcInput,
														config.verbose, config.warnPhitsIncompatible);
		sources.insert(sources.end(), tmpSrc.begin(), tmpSrc.end());
	}
	for(auto &source: sources) {
		if(source && source->particleType() != phys::ParticleType::PHOTON) {
			mWarning() << "This code is only for photon.\n Check \"proj =\" in source section(s).";
		}
	}

	return sources;
}

const std::unordered_map<std::string, std::string> &src::PhitsSource::defaultParameterMap()
{
	static const std::unordered_map<std::string, std::string> defMap{
		// 全ソース共通のデフォルト値あり必須パラメータは少ない。
		{"factor", "1.0"}
	};
	return defMap;
}

const std::vector<std::string> &src::PhitsSource::essentialKeywords()
{
	static std::vector<std::string> essential{"proj", "dir", "s-type"};
	return essential;
}

const std::vector<std::string> &src::PhitsSource::acceptableParams()
{
	static const std::vector<std::string> acceptable {
		"<source>", "a-type", "e-type", "e0", "dir", "dom", "dump", "idmpmode", "dmpmulti",
		"e-type", "factor", "file", "ispfs", "iscorr", "izst", "ntmax",
		"phi", "proj", "pz0", "r-type", "r0", "r1", "r2", "reg", "rn", "s-type",
		"sx", "sy", "sz", "t-type", "totfactor", "trcl", "wem", "wgt", "wt0",
		 "x0", "x1", "x2", "x3","xp", "xq", "y0", "y1", "y2", "y3", "yp", "yq",
        "z0", "z1", "z2", "z3", "exz", "nr"
	};
	return acceptable;
}

const std::vector<std::pair<std::string, std::string>> &src::PhitsSource::exclusiveKeywords()
{
	static const std::vector<std::pair<std::string, std::string>> params {
		std::make_pair("e0", "e-type"),
	};
	return params;
}

const std::vector<std::string> &src::PhitsSource::acceptableExtendedParams()
{
	static std::vector<std::string> accExParams {
		"a-type", "e-type", "r-type", "x-type", "y-type", "z-type"
	};
	return accExParams;
}
/*
 * ソースセクションはマルチソースの場合<source>で複数のサブセクションに区切られ、
 * 拡張入力も各々のサブセクションに所属する。PhitsInputSectionクラスは
 * 1クラス1セクションで、コンストラクタ内で通常入力と拡張入力を分けてしまうので
 * サブセクションの拡張入力をサブセクションごとに分けられないため、
 * この関数の引数にはlist<DataLine>を用いている。
 */
std::vector<std::shared_ptr<const src::PhitsSource>>
	src::PhitsSource::createMultiSource(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
									const std::unordered_map<std::string, std::shared_ptr<const geom::Cell> > &cellMap,
									const std::list<inp::DataLine> &inputList, bool verbose, bool warnPhitcCompat)
{
	namespace ip = inp::phits;
	/*
	 *　とりあえず<source>間で区切って個別にcreateSourceで生成していく。
	 * 　最後にtotfactorと各ソースの<source>から各ソースのfactor_を決定する。
	 */

	//NOTE コメント(std::regex(R"(^ {0,4}[cC])"))と区別のつくソースパターンを作りたい。
	std::regex multiSrcPattern(R"(^ *<source> *=)");
	auto sourceListVec = utils::splitDataLineListByTitle(inputList, multiSrcPattern, inp::comm::getPreCommentPattern());

	// ソースセクションじゃない場合。
	if(sourceListVec.size() == 0) {
		mWarning()<< "Source section not found.";
		return std::vector<std::shared_ptr<const src::PhitsSource>>{};
	} else if(sourceListVec.size() == 1) {
		// シングルソース
		ip::PhitsInputSection ipSection(ip::toSectionString(ip::Section::SOURCE), inputList, verbose, warnPhitcCompat);
		return std::vector<std::shared_ptr<const src::PhitsSource>>{src::PhitsSource::createSingleSource(trMap, cellMap, ipSection)};
	}

	// 以下マルチソース作成。 シングルソースとはfactorの扱いが異なるので同様の処理が適用できない。
	std::vector<std::shared_ptr<src::PhitsSource>> sourceVec;
	for(auto &srcList: sourceListVec) {
		ip::PhitsInputSection ipSection(ip::toSectionString(ip::Section::SOURCE), srcList, verbose, warnPhitcCompat);
		sourceVec.emplace_back(src::PhitsSource::createSingleSource(trMap, cellMap, ipSection));
	}

	// マルチソースの共通部分よみとり。
	// totfactorの読み取りをして合計のfactorがtotfactorに一致するように再度規格化する。
	double totfactor = 1.0;
	auto it = utils::findDataLine(inputList, "totfactor");
	if(it != inputList.end()) {
		auto strVec = utils::splitString("=", it->data);
		assert(strVec.size() == 2);
		totfactor = utils::stringTo<double>(strVec.at(1));
	}
	double sumOfFactors = 0;
	for(auto &x: sourceVec) {
		sumOfFactors += x->factor();
	}
	if(utils::isSameDouble(sumOfFactors, 0.0)) {
		throw std::invalid_argument("Sum of source factors = 0");
	}
	for(auto &x: sourceVec) {
		x->setFactor(x->factor()*totfactor/sumOfFactors);
	}
	// factor調整を行ったらshared<const Source>のようにconst化してから返す
	std::vector<std::shared_ptr<const src::PhitsSource>> retVec;
	for(auto &s: sourceVec) {
		retVec.emplace_back(s);
	}
	return retVec;
}

bool src::PhitsSource::isInAcceptedCells(const math::Point pos) const
{
	if(acceptableCells_.empty()) return true;

	for(auto &cell: acceptableCells_) {
		if(cell->isInside(pos)) return true;
	}
	return false;
}



