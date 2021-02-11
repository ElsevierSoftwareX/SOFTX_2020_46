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
#include "multigroupdistribution.hpp"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <iostream>
#include "core/utils/utils.hpp"
#include "core/utils/numeric_utils.hpp"




src::MultigroupDistribution::MultigroupDistribution(const std::vector<double> &vvec, const std::vector<double> &pvec)
	:bounds_(vvec), pdf_(pvec)
{
	assert(bounds_.size() == pdf_.size()+1);
	// 昇順であることは必須
	if(!utils::isAscendant(vvec, false)) {
		throw std::invalid_argument("Distribution values are not ascendant order.");
	}
	// 値の非負も必須
	for(size_t i = 0; i < pvec.size(); ++i) {
		if(pvec.at(i) < 0) {
			throw std::out_of_range("Probabilities are not positive.");
		}
	}

	double total = 0;
	for(size_t i = 0; i < pdf_.size(); ++i) {
		total += pdf_.at(i)*(bounds_.at(i+1) - bounds_.at(i));
	}
	// 積分値が0なら例外発生
	if(utils::isSameDouble(total, 0)) {
		throw std::invalid_argument("Total of pobabilities are 0");
	}
	// 積分値が1でない場合は警告出して規格化
	if(!utils::isSameDouble(total, 1)) {
		std::cerr << "Warning: Total probability of multigroup data is not 1, current = "
				  << std::scientific << total << ", normalized." << std::endl;
		for(auto &elem: pdf_) {
			elem /= total;
		}
	}
	type_ = Distribution::MGRP_CHARS;
}

double src::MultigroupDistribution::getPdf(double val) const
{
	try{
		auto lowerIndex = utils::getLowerBoundIndex(bounds_, val);
		if(bounds_.size() == pdf_.size()) {
			// pdfが離散点で定義されている場合
			return (pdf_.at(lowerIndex) * (bounds_.at(lowerIndex + 1) - val)
					+ pdf_.at(lowerIndex + 1)*(val - bounds_.at(lowerIndex)))
						/(bounds_.at(lowerIndex + 1) - bounds_.at(lowerIndex));
		} else if (bounds_.size() == pdf_.size() + 1) {
			// pdfが群形式で定義されている場合。
			auto lowerIndex = utils::getLowerBoundIndex(bounds_, val);
			return pdf_.at(lowerIndex);
		} else {
			std::cerr << "ProgramError: size of energy vector and value vector are not the same nor +1. E size="
					  << bounds_.size() << " value size=" << pdf_.size() << std::endl;
			std::exit(EXIT_FAILURE);
		}
	} catch (std::exception & e) {
		mDebug() << e.what();
		std::cerr << "E=" << val << " is out of energy distribution." << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

// pdfを (value-lowerWidth, value+upperWidth)の範囲で積分して確率を返す。
double src::MultigroupDistribution::getProbability(double value, double lowerWidth, double upperWidth) const
{
	assert(lowerWidth >= 0 && upperWidth >= 0);
	double lowerIntBound = value - lowerWidth;  // 積分範囲下限
	double upperIntBound = value + upperWidth;

	//mDebug() << "lower/upper integral bounds=" << lowerIntBound << upperIntBound;
	// 特殊ケース.積分範囲がゼロ、積分範囲が定義域から外れている場合。
	if(utils::isSameDouble(lowerIntBound, upperIntBound)
	|| lowerIntBound > bounds_.back() || upperIntBound < bounds_.front()) {
		return 0;
	}

	//mDebug() << "lower, upper integral bounds =" << lowerIntBound << upperIntBound;
	auto lowerIndex = utils::getBoundIndexes(bounds_, lowerIntBound).first;  // 積分上限/下限が所属するpdfの群番号
	auto upperIndex = utils::getBoundIndexes(bounds_, upperIntBound).first;  // 含まれる区間の下限側indexが所属するpdfのindexに等しい
	const auto invalid_index = utils::INVALID_INDEX<decltype(lowerIndex)>();

	//mDebug() << "lower,upperIndex=" << lowerIndex << upperIndex;
	if(lowerIndex == upperIndex && lowerIndex != invalid_index) {
		// value±widthが同じ群に所属する場合
		return (upperIntBound - lowerIntBound)*pdf_.at(upperIndex);
    } else {
        double probability = 0;
		// 積分範囲の上限、下限が定義域外でない場合、含まれる群からの寄与
		if(lowerIndex != invalid_index) {
			double lowerContrib = pdf_.at(lowerIndex) * (bounds_.at(lowerIndex+1)-lowerIntBound);
			probability += lowerContrib;
			//mDebug() << "bounds[lowerIndex-1] = " << bounds_[lowerIndex-1] << ", bounds[lowerIndex] = " << bounds_[lowerIndex];
			//mDebug()  << "下側からの寄与=" << lowerContrib;
		}

		if(upperIndex != invalid_index) {
			double upperContrib = pdf_.at(upperIndex) * (upperIntBound - bounds_.at(upperIndex));
			probability += upperContrib;
			//mDebug()  << "上側からの寄与=" << upperContrib;
		}

		//mDebug() << "lower,upperIndex=" << lowerIndex << upperIndex;
		auto startIndex = (lowerIndex != invalid_index) ? lowerIndex+1 : 0;
		auto endIndex = (upperIndex != invalid_index) ? upperIndex :pdf_.size() ;
		for(size_t i = startIndex; i < endIndex; ++i) {
			//mDebug() << "i=" << i << "途中の群寄与" << (bounds_.at(i+1) - bounds_.at(i))*pdf_.at(i);
			probability += (bounds_.at(i+1) - bounds_.at(i))*pdf_.at(i);
        }

        return probability;
    }
}


std::string src::MultigroupDistribution::toString() const
{
	std::stringstream ss;
	ss << type_ << std::endl;
	for(size_t i = 0; i < pdf_.size(); ++i) {
		ss << SCIOUT(3) << bounds_.at(i) << "  " << pdf_.at(i) << std::endl;
	}
	ss << SCIOUT(3) << bounds_.back();
	return ss.str();
}

