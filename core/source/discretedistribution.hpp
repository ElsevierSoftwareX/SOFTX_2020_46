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
#ifndef DISCRETEDISTRIBUTION_HPP
#define DISCRETEDISTRIBUTION_HPP

#include "abstractdistribution.hpp"

namespace src {

class DiscreteDistribution: public Distribution
{
public:
	DiscreteDistribution(const std::vector<double> &vvec, const std::vector<double> &pvec);
    DiscreteDistribution(double value, double probability);
    std::vector<double> getDiscretePoints() const override;
	double getProbability(double value, double lowerWidth, double upperWidth) const override;
	double getPdf(double value) const override;
	std::string toString() const override;
protected:
	// value, probability のペアで保存
	std::vector<std::pair<double, double>> valuePairs_;

};


}  // end namespace src

#endif // DISCRETEDISTRIBUTION_HPP
