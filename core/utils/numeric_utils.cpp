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
#include "numeric_utils.hpp"


#include <algorithm>
#include <cmath>

template<class T>
constexpr double spow(T base, int exp) noexcept {
  //static_assert(exp >= 0, "Exponent must not be negative");
  return (exp == 0) ? 1
	  :  (exp > 0) ? spow(base, exp-1)*base
	  :              spow(base, exp+1)/base;
}


bool utils::isSameDouble(double d1, double d2, double eps)
{
	return std::abs(d1 - d2) <= eps * std::max(1.0, std::max(std::abs(d1), std::abs(d2)));
}
