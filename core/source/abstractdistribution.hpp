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
#ifndef ABSTRACTDISTRIBUTION_HPP
#define ABSTRACTDISTRIBUTION_HPP

#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/io/input/dataline.hpp"


namespace src {


// 空間、エネルギー 、角度の3種類


class Distribution {
public:
    static const char MGRP_CHARS[];// = "multigroup";
    static const char DISC_CHARS[];// = "discrete";
    static const char PROP_CHARS[];// = "proportional";

	static std::vector<double> ReadDataFromList(size_t num,
												const std::list<inp::DataLine> &slist,
												std::list<inp::DataLine>::const_iterator &it);
	// group-wiseデータをbounds, pvecへ読み込む
	static void ReadMultiGroupData(int numPoints,
								   const std::list<inp::DataLine> &sourceInput,
								   std::list<inp::DataLine>::const_iterator &it,
								   std::vector<double> *bounds, std::vector<double> *pvec);

	// valuesの範囲で x^order に比例した分布を作る。
	static std::unique_ptr<Distribution> makeProportionalDistribution(const std::pair<double, double> &bounds, double order);
	// 離散分布の作成
	static std::unique_ptr<Distribution> makeDiscreteDistribution(const std::vector<double> &vvec, const std::vector<double> &pvec);



    Distribution();
    virtual ~Distribution();
	virtual std::string type() {return type_;}
	virtual std::string toString() const = 0;
    //　分布が離散的な場合確率が定義されている離散点を返す。
    virtual std::vector<double> getDiscretePoints() const = 0;

	virtual double getPdf(double value) const = 0;

	// getProbabilityのvector化
	std::vector<double> getProbabilities(const std::vector<double> &values,
										 const std::vector<std::pair<double, double>> &widthPairs) const;
	// pdfを(value-lowerWidth, value+upperWidth)間で積分した値(=確率)
	virtual double getProbability(double value, double lowerWidth, double upperWidth) const = 0;

protected:
	std::string type_;

};







}

#endif // ABSTRACTDISTRIBUTION_HPP
