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
#ifndef COLOR_HPP
#define COLOR_HPP

#include <string>
#include <sstream>

#include "component/picojson/picojson.h"

namespace img{


struct Color {
	static Color fromPhitsString(std::string colorStr);
    constexpr Color(int rv, int gv, int bv, double av):r(rv), g(gv), b(bv), a(av) {}
    // 16進文字列#RRGGBBでの色設定
    explicit Color(const std::string &str, double alpha = 1.0);
	int r; // 0-255整数
	int g;
	int b;
	double a;

    double rf() const { return static_cast<double>(r)/255.0;}
    double gf() const { return static_cast<double>(g)/255.0;}
    double bf() const { return static_cast<double>(b)/255.0;}
	std::string toRgbString() const;
	static Color getDefaultColor(int i);
	static const Color &NOT_COLOR();

	picojson::value jsonValue() const;
	static Color fromJsonObject(picojson::object obj);
};

bool operator == (const Color &c1, const Color &c2);
std::ostream &operator << (std::ostream&os, const Color& color);

bool isRgbString(const std::string &rgbStr);

}  // end namespace img
#endif // COLOR_HPP
