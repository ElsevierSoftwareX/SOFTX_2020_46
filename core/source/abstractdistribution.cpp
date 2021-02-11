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
#include "abstractdistribution.hpp"

#include <algorithm>
#include <cmath>

#include "discretedistribution.hpp"
#include "multigroupdistribution.hpp"
#include "proportionaldistribution.hpp"


#include "core/formula/fortran/fortnode.hpp"
#include "core/io/input/phits/phits_metacards.hpp"
#include "core/math/constants.hpp"
#include "core/utils/utils.hpp"



const char src::Distribution::MGRP_CHARS[] = "multigroup";
const char src::Distribution::DISC_CHARS[] = "discrete";
const char src::Distribution::PROP_CHARS[] = "proportional";

/*
 * NOTE namespace 内ではstd::ostream& operator<<(std::ostream&, std::vector<T>&)はオーバーロードできていない。
 * namespace srcの中でoperator<<を呼ぶと同じ名前空間内にないので、ADLで別の名前空間が探され、
 * stream_utils.hppで定義している(グローバル空間の)operator<<よりも
 * <ostream>で定義されている(std内の)operator<<の方が一致度が高いとして呼ばれる。
 * 引数返り値みんなstd::だから仕方ないね
 * traitsでも使うか
 */

namespace {

}  // end anonymous namespace

std::vector<double> src::Distribution::ReadDataFromList(size_t num,
									 const std::list<inp::DataLine> &slist,
									 std::list<inp::DataLine>::const_iterator &it)
{
	std::vector<double> retVec;
	while(retVec.size() < num) {
		if(it == slist.end()) {
			throw std::invalid_argument("Unexpected end of angular subsection, too few data.");
		}
		auto tmpVec = utils::splitString(" ", it->data, true);
		auto dataVec = utils::stringVectorTo<double>(tmpVec);
		std::copy(dataVec.begin(), dataVec.end(), std::back_inserter(retVec));
		++it;
	}
	return retVec;
}

// 境界値 n+1個と群の値n個を読み取りそれぞれ第一引数、第二引数に格納する。
void src::Distribution::ReadMultiGroupData(int numPoints,
										   const std::list<inp::DataLine> &sourceInput,
										   std::list<inp::DataLine>::const_iterator &it,
										   std::vector<double> *bounds, std::vector<double> *pvec)
{
	if(numPoints < 0) {
		mWarning(it->pos())<< " Equi-lethargy configuration is ignored.";
		numPoints = std::abs(numPoints);
	}

	auto startLine = *it;
	std::vector<double> dataVec;
	try {
		dataVec = ReadDataFromList(2*numPoints + 1, sourceInput, it);
	} catch (std::exception &e) {
		std::stringstream ss;
		ss << "Error: While reading angular distribution starting from " << startLine.pos() << std::endl;
		ss << e.what() << " or number of data, \"n[ae]=\", is wrong.";
		throw std::invalid_argument(ss.str());
	}

	if(dataVec.size() != static_cast<size_t>(2*numPoints + 1)) {
		std::cerr << "Waring: Number of input is too much in angular distribution" << std::endl;
	}

	for(int i = 0; i < 2*numPoints-1; i += 2) {
		bounds->emplace_back(dataVec.at(i));
		pvec->emplace_back(dataVec.at(i+1));
	}
	bounds->emplace_back(dataVec.back());
}


// z=-1,0 間に均一分布みたいな場合がある。円筒ソースでz0=1;z1=0はPhitsでOKか？要チェック
// phitsの挙動はz0=0; z1=-1 は許容される。かつ粒子参照方向は逆を向かない。
// FIXME 基底クラスに派生クラスの関数を書くとファイル間依存が複雑になるので要リファクタリング
std::unique_ptr<src::Distribution>
	src::Distribution::makeProportionalDistribution(const std::pair<double, double> &bounds, double order)
{
	// もし群境界下限と上限が一致する場合は１要素離散分布として扱う。
	if(std::abs(bounds.second - bounds.first) < math::EPS) {
		return std::unique_ptr<DiscreteDistribution>(new DiscreteDistribution(std::vector<double>{bounds.first}, std::vector<double>{1}));
	}
	// NOTE 定義域の方向が負の分布を許容すべきか？ 許容しない。
	if(bounds.first > bounds.second) { // 上限と下限の大小が逆なら反転させる。
	  return std::unique_ptr<ProportionalDistribution>
			  (new ProportionalDistribution(std::make_pair(bounds.second, bounds.first), order));
	}
	return std::unique_ptr<ProportionalDistribution>(new ProportionalDistribution(bounds, order));
}

std::unique_ptr<src::Distribution>
	src::Distribution::makeDiscreteDistribution(const std::vector<double> &vvec,
												const std::vector<double> &pvec)
{
	return std::unique_ptr<DiscreteDistribution>(new DiscreteDistribution(vvec, pvec));
}

src::Distribution::Distribution(){}

src::Distribution::~Distribution(){}

std::vector<double> src::Distribution::getProbabilities(const std::vector<double> &values,
														const std::vector<std::pair<double, double> > &widthPairs) const
{
    std::vector<double> retVec;
    for(size_t i = 0; i < values.size(); ++i) {
		retVec.emplace_back(getProbability(values.at(i), widthPairs.at(i).first, widthPairs.at(i).second));
    }
	return retVec;
}


