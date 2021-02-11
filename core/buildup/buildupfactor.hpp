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
#ifndef BUILDUPFACTOR_HPP
#define BUILDUPFACTOR_HPP

#include <vector>
namespace bd {

double calcDose(std::vector<double> cells, std::vector<double> tracks);

class BuildupFactor
{
public:
	BuildupFactor(){;}
};


}  // end namespace bd
#endif // BUILDUPFACTOR_HPP
