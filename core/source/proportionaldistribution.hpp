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
#ifndef PROPORTIONALDISTRIBUTION_HPP
#define PROPORTIONALDISTRIBUTION_HPP

#include "abstractdistribution.hpp"

namespace src {

class ProportionalDistribution: public Distribution
{
public:
	ProportionalDistribution(const std::pair<double, double> &bounds, double order);
	// valに対応したpdfの値を返す。
	double getPdf(double val) const override;
	// virtualは実装しなければならない。
        std::vector<double> getDiscretePoints() const override {return std::vector<double>();}
	double getProbability(double value, double lowerWidth, double upperWidth)const override;
	std::string toString() const override;
private:
	// 次数
	double order_;
	// 定義されている区間
	std::pair<double, double> bounds_;
	// bounds間の面積を1に規格化するために使った規格化因子
	double normalizedFactor_;

};



}  // end namespace src



#endif // PROPORTIONALDISTRIBUTION_HPP
