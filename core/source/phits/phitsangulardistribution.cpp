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
#include "phitsangulardistribution.hpp"

#include <functional>
#include <vector>

#include "phitsdistribution.hpp"
#include "core/formula/fortran/fortnode.hpp"
#include "core/math/constants.hpp"
#include "core/physics/physconstants.hpp"
#include "core/utils/utils.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/phits/phits_metacards.hpp"
#include "core/source/abstractdistribution.hpp"
#include "core/source/discretedistribution.hpp"
#include "core/source/multigroupdistribution.hpp"
#include "core/source/proportionaldistribution.hpp"

namespace {
// distribution作成関数のタイプ 返り値がDistributionへのスマポで、
// 引数が文字列、DataLineリスト、 DataLineリストのiterator
typedef  std::function<std::unique_ptr<src::Distribution>(const std::list<inp::DataLine> &,
														  std::list<inp::DataLine>::const_iterator&)>
		  creation_function_type;

creation_function_type getAngularDistributionCreator(const std::string &typeStr)
{
	static std::unordered_map<std::string, creation_function_type> creatorMap {
		{"1", src::phits::AngularMultiGroupCreator("1", false)},
		{"4", src::phits::AngularMultiGroupCreator("4", false)},
		{"11", src::phits::AngularMultiGroupCreator("1", true)},
		{"14", src::phits::AngularMultiGroupCreator("4", true)},

		{"5", src::phits::AngularUserDefinedCreator("5", false)},
		{"6", src::phits::AngularUserDefinedCreator("6", false)},
		{"15", src::phits::AngularUserDefinedCreator("5", true)},
		{"16", src::phits::AngularUserDefinedCreator("6", true)}
	};
	return creatorMap.at(typeStr);
}

}  // end anonymous namespace


std::unique_ptr<src::Distribution>
src::phits::createPhitsAngularDistribution(const std::string &a_type,
										  const std::list<inp::DataLine> &sourceInput,
										  std::list<inp::DataLine>::const_iterator &it)
{
	//auto startLine = *it;
	std::string atype = a_type;
	utils::trim(&atype);
	return getAngularDistributionCreator(atype)(sourceInput, it);
}

std::unique_ptr<src::Distribution>
src::phits::AngularMultiGroupCreator::operator()(const std::list<inp::DataLine> &sourceInput,
												 std::list<inp::DataLine>::const_iterator &it)
{
	int numPoints = inp::phits::GetParameterValue<int>("na", sourceInput, it).second;
	++it;
	std::vector<double> avec, pvec;
	src::Distribution::ReadMultiGroupData(numPoints, sourceInput, it, &avec, &pvec);
	if(isDegree_) 	for(auto &aval: avec) aval = std::cos(math::toRadians(aval));

	for(size_t i = 0; i < pvec.size(); ++i) {
		pvec[i] = pvec.at(i)/(avec.at(i+1) - avec.at(i));
	}

	// cosin値のレンジチェック
	for(size_t i = 0; i < avec.size(); ++i) {
		if(avec.at(i) > 1 || avec.at(i) < -1) {
			std::stringstream ss;
			ss << "Direction cosine is out of range. angular bound =" << avec.at(i);
			throw std::out_of_range(ss.str());
		}
	}
	return std::unique_ptr<MultigroupDistribution>(new MultigroupDistribution(avec, pvec));
}

std::unique_ptr<src::Distribution>
src::phits::AngularUserDefinedCreator::operator()(const std::list<inp::DataLine> &sourceInput,
												  std::list<inp::DataLine>::const_iterator &it)
{
	using StrVec = std::vector<std::string>;
	// a-type=5の場合a-type定義行を除いて4行固定
	auto paramsMap = src::phits::readParamsMap(StrVec{"nn", "ag1", "ag2", "g(x)"}, StrVec{}, sourceInput, it);
	int numPoints = utils::stringTo<int>(paramsMap.at("nn"));
	if(numPoints < 0) {
		throw std::invalid_argument("Number of angular points should be positive, actual=" + paramsMap.at("nn"));
	}
	double amin = utils::stringTo<double>(paramsMap.at("ag1"));
	double amax = utils::stringTo<double>(paramsMap.at("ag2"));
	if(isDegree_) {
		amin = std::cos(math::toRadians(amin));
		amax = std::cos(math::toRadians(amax));
	}
	std::vector<double> apoints = src::phits::getPoints(amin, amax, numPoints);
	// ここでE分点データとその点での確率が得られた。しかしphitsではEと確率のペアでの分布定義を採用せず、
	// 群上限・下限と確率の入力しかサポートしていない。よって台形積分でp, Eを作成する。
	std::string funcStr = paramsMap.at("g(x)");
	std::vector<double> pvec = getProbabilityGroup(apoints, funcStr);
	return std::unique_ptr<MultigroupDistribution>(new MultigroupDistribution(apoints, pvec));
}

src::phits::AngularDistributionCreator::AngularDistributionCreator(const std::string &atype, bool isDeg):atype_(atype), isDegree_(isDeg){}

src::phits::AngularDistributionCreator::~AngularDistributionCreator(){}
