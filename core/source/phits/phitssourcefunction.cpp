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
#include "phitssourcefunction.hpp"

#include<regex>

#include "phitsangulardistribution.hpp"
#include "phitsenergydistribution.hpp"

#include "core/source/abstractdistribution.hpp"
#include "core/source/discretedistribution.hpp"
#include "core/source/proportionaldistribution.hpp"
#include "core/source/multigroupdistribution.hpp"

#include "core/formula/fortran/fortnode.hpp"
#include "core/geometry/cell/cell.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/meshdata.hpp"
#include "core/io/input/phits/phits_metacards.hpp"
#include "core/io/input/phits/phitsinputsection.hpp"
#include "core/utils/container_utils.hpp"
#include "core/utils/numeric_utils.hpp"

namespace {
const char EX_HEADER[] = "c * ";
}

void src::phits::readPhitsExtendedParameters(const std::list<inp::DataLine> &extendedInput,
											 const std::vector<std::string> &acceptableExtendedKeywords,
											 const std::vector<std::string> &essentialExtendedKeywords,
											 const std::shared_ptr<src::Distribution> energyDistribution,
											 const std::array<std::shared_ptr<src::Distribution>, 3> spatialDistributions,
											 const std::array<std::string, 3> &spatialChars,
											 inp::MeshData *energyMesh,
											 std::array<inp::MeshData, 3> *spatialMeshes)
{
	(void) acceptableExtendedKeywords;
	(void) essentialExtendedKeywords;
	// エネルギーメッシュ
	auto exStartPos = extendedInput.empty() ? "" : extendedInput.front().pos();
	auto eDistTypeStr = energyDistribution->type();
	if(eDistTypeStr == src::Distribution::DISC_CHARS) {
		// ソースが離散エネルギー分布でc *e-typeを指定していたら警告する。
		if(utils::findDataLine(extendedInput, "e-type") != extendedInput.end()) {
			mWarning(utils::findDataLine(extendedInput, "e-type")->pos())
					<<"\"c * e-type\" are defined for discrete distribution, ignored.";
		}
		*energyMesh = inp::MeshData::fromDiscreteValues(energyDistribution->getDiscretePoints());
	} else if(!extendedInput.empty()) {
		*energyMesh = inp::MeshData::fromPhitsInput(extendedInput, inp::MeshData::KIND::E);
	}
	for(size_t i = 0; i < 3; ++i) {
		std::string typeParamName = spatialChars.at(i) + "-type";
		// メッシュ生成する対象の分布が離散分布の場合、メッシュも離散点から生成する
		auto distType = spatialDistributions.at(i)->type();
		if(distType == src::Distribution::DISC_CHARS) {
			if(utils::findDataLine(extendedInput, typeParamName) != extendedInput.end()) {
				mWarning(utils::findDataLine(extendedInput, typeParamName)->pos())
						<< EX_HEADER << typeParamName << " is defined for discrete distribution, ignored";
			}
			spatialMeshes->at(i) = inp::MeshData::fromDiscreteValues(spatialDistributions[i]->getDiscretePoints());
			//		} else if(i == 1 && spatialDistributions.at(0)->type() == src::Distribution::DISC_CHARS &&) {
			//		// i=1の空間分布が角度分布で、径方向分布r=0に離散分布の場合、角度空間メッシュは1点のみの自動生成を採用する。
			//			mWarning() << "Source r-distribution is discrete and r=0, theta-distribution is set to be discrete, 0";
			//			spatialMeshes->at(i) = inp::MeshData(1, std::pair<double, double>(0, 360), inp::MeshData::InterPolate::LIN);
		} else {
			// メッシュ生成する対象の分布が離散分布以外の場合拡張入力からメッシュを生成する。
			spatialMeshes->at(i)= inp::MeshData::fromPhitsInput(extendedInput, inp::MeshData::strToKind(spatialChars.at(i)));
		}
	}

	if(energyMesh->empty()) {
        throw std::invalid_argument(exStartPos + " Energy mesh is required but not defined.");
	} else {
		for(size_t i = 0; i < 3; ++i) {
			if(spatialMeshes->at(i).empty()) {
                std::stringstream ess;
                ess << exStartPos << " Spatial mesh for direction " << i << " is requred but not defined.";
                throw std::invalid_argument(ess.str());
			}
		}
	}
}




std::unordered_map<std::string, std::string>
	src::phits::readPhitsParameters(const std::list<inp::DataLine> &sourceInput,
									const std::unordered_map<std::string, std::string> &defaultMap,
									const std::vector<std::string> &acceptables,
									const std::vector<std::string> &essentials,
									const std::vector<std::pair<std::string, std::string> > exclusives,
									const std::unordered_map<std::string, std::shared_ptr<const geom::Cell> > &cellMap,
									double *factor,
									std::shared_ptr<src::Distribution> *energyDistribution,
									std::shared_ptr<src::Distribution> *angularDistribution,
									std::vector<std::shared_ptr<const geom::Cell> > *acceptableCells,
									bool hasEnergyDistribution)
{
	auto startPos = sourceInput.empty() ? "" : sourceInput.front().pos();
	std::unordered_map<std::string, std::string> parameterMap = defaultMap;
	auto pattern = inp::phits::parameterPattern();
	std::smatch sm;
	for(auto it = sourceInput.cbegin(); it !=  sourceInput.cend(); ++it) {
		//mDebug() << "Reading cylinder source" << it->pos() << ", input=" << it->data;
		if(std::regex_search(it->data, sm, pattern)) {
			auto paramValuePair = inp::phits::getParameterPair(it->data);
			auto paramName = paramValuePair.first;
			auto paramValue = paramValuePair.second;
			// 許容可能パラメータを設定し、それ以外はエラーにする。
			if(std::find(acceptables.begin(), acceptables.end(), paramName) == acceptables.end()) {
                throw std::invalid_argument(it->pos() +  " \"" + paramName + "\" is not a valid parameter name.");
			}
			parameterMap[paramName] = paramValue;
			// *-type=行を見つけたら分布を読み取る。itはその分進める。
			if(std::regex_search(paramName, sm, inp::phits::distributionPattern())) {
				std::string kind = sm.str(1);
				std::string distType = paramValue;
				++it;  // 一つ進めて分布定義文の中身を指すようにする。
				auto sectionStartLine = *it;
				if(kind == "a" || kind == "A") {
					*angularDistribution = src::phits::createPhitsAngularDistribution(distType, sourceInput, it);
				} else if (kind == "e" || kind == "E") {
					*energyDistribution = src::phits::createPhitsEnergyDistribution(distType, sourceInput, it);
				} else {
                    throw std::invalid_argument(sectionStartLine.pos() + " Invalid distribution type string = \"" + paramName + "\".");
				}
				--it;  // ループに合わせるため一つ巻き戻す。
			}
		} else {
            throw std::invalid_argument(it->pos() + " \"" + it->data + "\" is not a valid source parameter line.");
		}
	}

	// sourceList読み取り終了。読み取り結果チェック
	try {
		// 必須パラメータを満たしているかチェック
		for(auto &essential :essentials) {
			if(parameterMap.find(essential) == parameterMap.end()) {
				throw std::invalid_argument("Source section does not contain essentail parameter =" + essential);
			}
		}
		// 排他的利用パラメータを両方指定している場合もエラー
		for(auto &exPair: exclusives) {
			if(parameterMap.find(exPair.first) != parameterMap.end()
					&& parameterMap.find(exPair.second) != parameterMap.end()) {
				throw std::invalid_argument("Both exclusive parameters {" + exPair.first + ", " + exPair.second + "} found");
			}
		}
		// エネルギー分布ありなのに読み取れていない場合や、無しなのに読み取っている場合もエラー
		if(hasEnergyDistribution && !(*energyDistribution)) {
			throw std::invalid_argument("\"e-type\" is missing while required");
		} else if(!hasEnergyDistribution && (*energyDistribution)) {
			throw std::invalid_argument("Both \"e-type\" and \"e0\" are found");
		}
		// s-typeとbool指定が矛盾していないかもチェック。s-typeは必須パラメータ。
		std::string stype = parameterMap.at("s-type");
		std::vector<std::string> haveEdistTypes{"4", "5", "6", "8", "10", "14", "16", "19", "21"};
		bool hasEdist = std::find(haveEdistTypes.begin(), haveEdistTypes.end(), stype) != haveEdistTypes.end();
		if(hasEnergyDistribution && !hasEdist) {
			throw std::invalid_argument("s-type=" + stype + " requires energy distribution but not found");
		} else if (!hasEnergyDistribution && hasEdist) {
			throw std::invalid_argument("s-type=" + stype + " dose not neet energy distribution but found");
		}

	} catch (std::invalid_argument &e) {
        throw std::invalid_argument(startPos + " In source section, " + e.what());
	}

	// regパラメータ処理
	if(parameterMap.find("reg") != parameterMap.end()) {
		if(parameterMap.at("reg") == "all") {
			parameterMap.at("reg").clear();  // allは入力なしと同一視
		} else {
			// TODO regパラメータでのlattice要素記法の展開
			parameterMap.at("reg") = inp::phits::PhitsInputSection::expandBrace(parameterMap.at("reg"));  // {N-M}を展開
		}
	}

	// 許容セル処理
	if(parameterMap.find("reg") != parameterMap.end()) {
		for(auto &regCell: utils::splitString(" ", parameterMap["reg"], true)) {
			if(cellMap.find(regCell) != cellMap.end()) {
				acceptableCells->emplace_back(cellMap.at(regCell));
			} else {
                throw std::invalid_argument(startPos + " cell \"" + regCell + "\" is not defined but used in source reg parameter.");
			}
		}
	}

	// エネルギー分布設定
	if(!hasEnergyDistribution) {
		assert(!(*energyDistribution));
		*energyDistribution = std::make_shared<src::DiscreteDistribution>(fort::eq(parameterMap.at("e0")), 1.0);
	}
	assert(*energyDistribution);

	// 角度分布設定
	// dirの設定dirの解釈は点源核とコードで対応できそうなソースではほぼ同じ。球ソース(s-type=15,16)の場合は特別扱いが必要
	auto dirValue = parameterMap.at("dir");
	if(dirValue == "all") {
		if(*angularDistribution) mWarning(startPos) << "Both a-type and dir=all were found. a-type was ignored";
		*angularDistribution = Distribution::makeProportionalDistribution(std::make_pair(-1, 1), 0);
	} else if (dirValue == "data") {
        if(!(*angularDistribution)) throw std::invalid_argument(startPos + " dir=data is set but no a-type is found.");
	} else {
		// 単一方向を即値指定の場合。
		try {
			double dir = fort::eq(dirValue);
			*angularDistribution = Distribution::makeDiscreteDistribution(std::vector<double>{dir}, std::vector<double>{1.0});
		} catch (std::invalid_argument &e) {
            throw std::invalid_argument(startPos + " dir= should be double [-1, 1], input =" + dirValue + e.what());
		}
	}

	// ここからパラメータ読み取り
	*factor = fort::eq(parameterMap.at("factor"));
	// 線源強度因子はマルチソースと通常ソースで扱いが変わるので面倒。
	if(parameterMap.find("<source>") != parameterMap.end()) {
		// マルチソースの一部の場合、factorは使われず、<source>=の値がfactorに相当する。
		// totfactorはPhitsSource::createMultiSource内で処理される。
		if(!utils::isSameDouble(*factor, 1.0)) mWarning() << "\"factor=\" can not be used in multi-source input, ignored.";
		*factor = utils::stringTo<double>(parameterMap.at("<source>"));
	} else {
		// 非マルチソースの場合、factor, totfactor両方ある時はその積が因子になる。
		if(parameterMap.find("totfactor") != parameterMap.end()) {
			*factor *= fort::eq(parameterMap.at("totfactor"));
		}
	}

	return parameterMap;
}
