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
#include "discretedistribution.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <sstream>
#include "core/utils/utils.hpp"


src::DiscreteDistribution::DiscreteDistribution(const std::vector<double> &vvec,
                                                const std::vector<double> &pvec)
{
	assert(vvec.size() == pvec.size());
	// 離散分布のエネルギーは昇順でなくても良いが非負であること
	for(size_t i = 0; i < pvec.size(); ++i) {
		if(pvec.at(i) < 0) {
			throw std::out_of_range("Probabilitiy in discrete distribution is not positive.");
		}
	}

	// 規格化
	auto newPvec = utils::normalize(pvec);

	for(size_t i = 0; i < vvec.size(); ++i) {
		valuePairs_.push_back(std::make_pair(vvec.at(i), newPvec.at(i)));
	}
    type_ = Distribution::DISC_CHARS;
}

src::DiscreteDistribution::DiscreteDistribution(double value, double probability)
    :DiscreteDistribution(std::vector<double>{value}, std::vector<double>{probability})
{}

std::vector<double> src::DiscreteDistribution::getDiscretePoints() const
{
    std::vector<double> retVec;
    for(auto &p: valuePairs_) {
        retVec.emplace_back(p.first);
    }
    return retVec;
}


double src::DiscreteDistribution::getProbability(double value, double lowerWidth, double upperWidth) const
{
    double probability = 0;
	double lowerBound = value - lowerWidth;
	double upperBound = value + upperWidth;
    for(auto &vPair: valuePairs_) {
//		mDebug() << "lowerWidth=" << lowerWidth << "upperWidth=" << upperWidth;
//		mDebug() << "lowerBound=" << lowerBound << "upperBound" << upperBound;
        if(lowerBound <= vPair.first && vPair.first < upperBound) probability += vPair.second;
    }
	return probability;
}

double src::DiscreteDistribution::getPdf(double value) const
{
	(void) value;
	// NOTE デルタ関数の値は発散するので0を返す。
	return 0;
}

std::string src::DiscreteDistribution::toString() const
{
	std::stringstream ss;
	ss << type_ << std::endl;
	for(size_t i = 0; i < valuePairs_.size(); ++i) {
		ss << SCIOUT(3) << valuePairs_.at(i).first << "  " << valuePairs_.at(i).second;
		if(i != valuePairs_.size() - 1) ss << std::endl;
	}
	return ss.str();
}
