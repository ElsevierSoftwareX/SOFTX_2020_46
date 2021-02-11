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
#ifndef MULTIGROUPDISTRIBUTION_HPP
#define MULTIGROUPDISTRIBUTION_HPP

#include "abstractdistribution.hpp"

namespace src {

class MultigroupDistribution: public Distribution
{
public:
	MultigroupDistribution(const std::vector<double> &vvec, const std::vector<double> &pvec);
	// ここからvirtualの実装
    std::vector<double> getDiscretePoints() const override {return std::vector<double>();}
	double getProbability(double value, double lowerWidth, double upperWidth) const override;
	double getPdf(double value) const override;
	std::string toString() const override;

protected:
	// value は群境界、 pdf_はその間のpdf。values_.sieze() == pdf_.size()+1
	std::vector<double> bounds_;
	std::vector<double> pdf_;
};



}  // end namespace src







#endif // MULTIGROUPDISTRIBUTION_HPP
