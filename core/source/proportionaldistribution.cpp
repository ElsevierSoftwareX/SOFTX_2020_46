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
#include "proportionaldistribution.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include "core/utils/utils.hpp"
#include "core/utils/numeric_utils.hpp"




src::ProportionalDistribution::ProportionalDistribution(const std::pair<double, double> &bounds, double order)
	:order_(order), bounds_(bounds)
{
	type_ = Distribution::PROP_CHARS;
	if(std::abs(bounds_.second - bounds_.first) < math::EPS) {
		throw std::invalid_argument("Bounds to define proportional distribution are the same.");
	}
    // 1/xの積分はlogなので別にする。
	if(std::abs(order - (-1)) < math::EPS) {
		normalizedFactor_ = std::log(bounds_.second) - std::log(bounds_.first);
	} else {
		normalizedFactor_ = (std::pow(bounds_.second, order_+1) - std::pow(bounds_.first, order_+1))/(order_+1);
    }
	assert(normalizedFactor_ > math::EPS);
//	assert(std::abs(normalizedFactor_) > math::EPS);

}

double src::ProportionalDistribution::getPdf(double val) const
{
	if(val > bounds_.second || val < bounds_.first) {
		std::stringstream ss;
		ss << "value(" << val << ") is out of range in which proportional distribution was defined("
		   << bounds_.first << ", " << bounds_.second << ").";
		throw std::out_of_range(ss.str());
	} else {
		return std::pow(val, order_)/normalizedFactor_;
	}
}



double src::ProportionalDistribution::getProbability(double value, double lowerWidth, double upperWidth) const
{
	double lowerBound = value - lowerWidth;
	double upperBound = value + upperWidth;
	if(lowerBound < bounds_.first) lowerBound = bounds_.first;
	if(upperBound > bounds_.second) upperBound = bounds_.second;
    // 1/xの積分はlogなので別にする。
	if(utils::isSameDouble(order_, -1.0)) {
        return (std::log(upperBound) - std::log(lowerBound))/normalizedFactor_;
    } else {
        return (std::pow(upperBound, order_+1) - std::pow(lowerBound, order_+1))
                /(order_+1)/normalizedFactor_;
    }
}

std::string src::ProportionalDistribution::toString() const
{
	std::stringstream ss;
	ss << type_ << std::endl;
	ss << "  defined between " << SCIOUT(3) << bounds_.first << ", " << bounds_.second
	   << " order " << std::fixed << order_;
	return ss.str();
}
