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
#ifndef SURFACE_UTILS_HPP
#define SURFACE_UTILS_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>
#include "core/math/nmatrix.hpp"

namespace geom {
class Surface;

std::shared_ptr<Surface> createSurface(const std::string &surfName,
                                         const std::string &surfSymbol,
                                         const std::vector<double> &surfParams,
                                         const std::map<std::string, std::string> &paramMap,
                                         const math::Matrix<4> &trMatrix,
                                         bool warnPhitsCompat);


}  // end namespace geom

#endif // SURFACE_UTILS_HPP
