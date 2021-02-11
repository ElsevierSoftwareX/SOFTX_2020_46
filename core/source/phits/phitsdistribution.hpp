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
#ifndef PHITSDISTRIBUTION_HPP
#define PHITSDISTRIBUTION_HPP

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

namespace inp {
class DataLine;
}

namespace src {
namespace phits {

std::unordered_map<std::string, std::string> readParamsMap(std::vector<std::string> requiredParamNames,
														   std::vector<std::string> optionalParamNames,
														   const std::list<inp::DataLine> &sourceInput,
														   std::list<inp::DataLine>::const_iterator &it);

std::vector<double> getProbabilityGroup(const std::vector<double> &epoints, const std::string &funcStr);

std::vector<double> getPoints(double emin, double emax, int numPoints);


}  // end namespace phits
}  // end namespace src

#endif // PHITSDISTRIBUTION_HPP
