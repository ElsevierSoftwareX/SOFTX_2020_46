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
#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <cmath>
#include <complex>
#include <limits>
namespace math {

static constexpr double PI = 3.1415926535897932384626433832795029;
static constexpr double INV_PI = 1.0/PI;
//static constexpr double EPS = 100000000*std::numeric_limits<double>::epsilon();
// epsは固定値にしたほうが安定する。
static constexpr double EPS = 1e-8;


// cube root of 1.
static constexpr std::complex<double> CUBE_ROOT_UNITY1(-0.5, 0.5*1.732050807568877);
static constexpr std::complex<double> CUBE_ROOT_UNITY2(-0.5, -0.5*1.732050807568877);


constexpr double toRadians(double deg) {
	return  PI*0.005555555555555555555*deg;
}

constexpr double toDegrees(double rad) {
	return rad*INV_PI*180.0;
}



}

#endif // CONSTANTS_HPP
