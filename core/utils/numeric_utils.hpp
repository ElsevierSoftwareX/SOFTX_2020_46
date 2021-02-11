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
#ifndef NUMERIC_UTILS_HPP
#define NUMERIC_UTILS_HPP

#include "core/math/constants.hpp"
namespace utils {

// 差の相対値を比較するようにしないと、割とある1e-12とかのエントリに対応できない
bool isSameDouble(double d1, double d2, double eps = math::EPS);

}

#endif // NUMERIC_UTILS_HPP
