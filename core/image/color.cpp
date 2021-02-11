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
#include "color.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"



const std::unordered_map<std::string, std::array<double, 3>> &getNameHSBMap()
{
	// phitsの定義ではhueは逆になっているので
	// 1から引いてかつ(hue)は1.2で割る
    static std::unordered_map<std::string, std::array<double, 3>> hsbMap {
        {"white",     {0, 0, 1.0}},
        {"lightgray", {0, 0, 0.8}},
        {"gray",      {0, 0, 0.6}},
        {"darkgray",  {0, 0, 0.4}},
        {"matblack",  {0, 0, 0.2}},
        {"black",     {0, 0, 0.0}},
        {"w", {0, 0, 1.0}},
        {"o", {0, 0, 0.8}},
        {"k", {0, 0, 0.6}},
        {"j", {0, 0, 0.4}},
        {"f", {0, 0, 0.2}},
        {"e", {0, 0, 0.0}},

		{"r",       {0.833333*(1.0-1.000), 1, 1}},
		{"red",     {0.833333*(1.0-1.000), 1, 1}},
		{"orange",  {0.833333*(1.0-0.933), 1, 1}},
		{"rr",      {0.833333*(1.0-0.933), 1, 1}},
		{"rrr",     {0.833333*(1.0-0.867), 1, 1}},
		{"y",       {0.833333*(1.0-0.800), 1, 1}},
		{"yellow",  {0.833333*(1.0-0.800), 1, 1}},
		{"yy",      {0.833333*(1.0-0.733), 1, 1}},
		{"yyy",     {0.833333*(1.0-0.667), 1, 1}},
		{"green",   {0.833333*(1.0-0.600), 1, 1}},
		{"g",       {0.833333*(1.0-0.600), 1, 1}},
		{"gg",      {0.833333*(1.0-0.533), 1, 1}},
		{"ggg",     {0.833333*(1.0-0.467), 1, 1}},
		{"c",       {0.833333*(1.0-0.400), 1, 1}},
		{"cyan",    {0.833333*(1.0-0.400), 1, 1}},
		{"cc",      {0.833333*(1.0-0.333), 1, 1}},
		{"ccc",     {0.833333*(1.0-0.267), 1, 1}},
		{"b",       {0.833333*(1.0-0.200), 1, 1}},
		{"bb",      {0.833333*(1.0-0.133), 1, 1}},
		{"bbb",     {0.833333*(1.0-0.067), 1, 1}},
		{"blue",    {0.833333*(1.0-0.200), 1, 1}},
		{"violet",  {0.833333*(1.0-0.133), 1, 1}},
		{"magenta", {0.833333*(1.0-0.067), 1, 1}},

		{"darkred",       {0.833333*(1.0-1.000), 1.000, 0.600}},
		{"red",           {0.833333*(1.0-1.000), 1.000, 1.000}},
		{"pink",          {0.833333*(1.0-1.000), 0.500, 1.000}},
		{"pastelpink",    {0.833333*(1.0-0.900), 0.500, 1.000}},
		{"orange",        {0.833333*(1.0-0.933), 1.000, 1.000}},
		{"brown",         {0.833333*(1.0-0.900), 1.000, 0.500}},
		{"darkbrown",     {0.833333*(1.0-0.900), 1.000, 0.300}},
		{"pastelbrown",   {0.833333*(1.0-0.900), 0.600, 0.500}},
		{"orangeyellow",  {0.833333*(1.0-0.867), 1.000, 1.000}},
		{"camel",         {0.833333*(1.0-0.800), 0.700, 0.700}},
		{"pastelyellow",  {0.833333*(1.0-0.800), 0.700, 1.000}},
		{"yellow",        {0.833333*(1.0-0.800), 1.000, 1.000}},
		{"pastelgreen",   {0.833333*(1.0-0.700), 0.600, 1.000}},
		{"yellowgreen",   {0.833333*(1.0-0.700), 1.000, 1.000}},
		{"green",         {0.833333*(1.0-0.600), 1.000, 1.000}},
		{"darkgreen",     {0.833333*(1.0-0.600), 1.000, 0.600}},
		{"mossgreen",     {0.833333*(1.0-0.500), 1.000, 0.300}},
		{"bluegreen",     {0.833333*(1.0-0.500), 1.000, 1.000}},
		{"pastelcyan",    {0.833333*(1.0-0.400), 0.400, 1.000}},
		{"pastelblue",    {0.833333*(1.0-0.250), 0.400, 1.000}},
		{"cyan",          {0.833333*(1.0-0.400), 1.000, 1.000}},
		{"cyanblue",      {0.833333*(1.0-0.400), 1.000, 0.500}},
		{"blue",          {0.833333*(1.0-0.200), 1.000, 1.000}},
		{"violet",        {0.833333*(1.0-0.133), 1.000, 1.000}},
		{"purple",        {0.833333*(1.0-0.100), 1.000, 0.500}},
		{"magenta",       {0.833333*(1.0-0.067), 1.000, 1.000}},
		{"winered",       {0.833333*(1.0-0.002), 0.800, 0.700}},
		{"pastelmagenta", {0.833333*(1.0-0.067), 0.600, 1.000}},
		{"pastelpurple",  {0.833333*(1.0-0.100), 0.400, 0.500}},
		{"pastelviolet",  {0.833333*(1.0-0.133), 0.400, 1.000}},
    };
    return hsbMap;
}

std::array<double, 3> hsvToRgb(double h, double s, double v)
{
    double r = v;
    double g = v;
    double b = v;
    if (s > 0) {
        h *= 6.0;
        int i = static_cast<int>(h);
        double f = h - static_cast<double>(i);
        switch (i) {
            default:
            case 0:
                g *= 1 - s * (1 - f);
                b *= 1 - s;
                break;
            case 1:
                r *= 1 - s * f;
                b *= 1 - s;
                break;
            case 2:
                r *= 1 - s;
                b *= 1 - s * (1 - f);
                break;
            case 3:
                r *= 1 - s;
                g *= 1 - s * f;
                break;
            case 4:
                r *= 1 - s * (1 - f);
                g *= 1 - s;
                break;
            case 5:
                g *= 1 - s;
                b *= 1 - s * f;
                break;
        }
    }
    return std::array<double, 3>{r, g, b};
}

// 文字列から色を作成。論理名あるいは{H,S,V}あるいはRGB文字列。
img::Color img::Color::fromPhitsString(std::string colorStr)
{
    auto hsbMap = getNameHSBMap();
    auto it = hsbMap.find(colorStr);
    std::array<double, 3> colorArray;
    if(it != hsbMap.end()){
        // 色名の場合
        colorArray = it->second;
    } else {
        // phits数値文字列入力
        colorStr = utils::dequote(std::make_pair('{', '}'), colorStr, true);
		std::vector<double> values = utils::stringVectorTo<double>(utils::splitString(" ", colorStr, true));
		if(values.size() == 1) {
			/*
			 * 数値が1個の場合は
			 * ・正ならばHの値と解釈し、S=B=1を補う
			 * ・負ならばグレースケールなので-1倍してBの値として、H,S＝0を適用
			 */
			// HSB のSBが省略されている場合はS,B共に1とすることになっている。
			if(values.front() > 0) {
				values.emplace_back(1.0);
				values.emplace_back(1.0);
			} else {
				values = std::vector<double>{0, 0, -values.front()};
			}
		} else if(values.size() != colorArray.size()) {
			throw std::invalid_argument(std::string("HSB color string should consit of 1 or 3 elements, string =") + colorStr);
        }
        for(size_t i =0; i < colorArray.size(); ++i) colorArray[i] = values.at(i);
        // Hueの定義に合わせる
		colorArray[0] = (1.0 - colorArray[0])*0.83333333;
    }
    // hsv -> rgb 変換
    for(size_t i = 0; i < colorArray.size(); ++i) {
        if(colorArray.at(i) > 1 || colorArray.at(i) < 0) {
            throw std::out_of_range("hsb values should be <1 and >0, data = " + colorStr);
        }
    }
    colorArray = hsvToRgb(colorArray.at(0), colorArray.at(1), colorArray.at(2));
    return Color(static_cast<int>(std::round(255.0*colorArray.at(0))),
                 static_cast<int>(std::round(255.0*colorArray.at(1))),
				 static_cast<int>(std::round(255.0*colorArray.at(2))), 255);
}

img::Color::Color(const std::string &str, double alpha)
{
	if(str.empty() || str.front() != '#' || str.size() != 7
			|| str.substr(1).find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
		throw std::invalid_argument(std::string("\"") + str + "\" is not a valid RGB string");
	} else if (alpha > 1.0 || alpha < 0) {
		throw std::invalid_argument(std::to_string(alpha) + " is not a valid alpha value");
	}
	size_t idx;
	r = std::stoi(str.substr(1, 2), &idx, 16);
	g = std::stoi(str.substr(3, 2), &idx, 16);
	b = std::stoi(str.substr(5, 2), &idx, 16);
	a = alpha;
}

std::string img::Color::toRgbString() const {
	std::stringstream ss;
	ss << std::hex;
	ss << std::setfill('0') << std::setw(2) << r
	   << std::setfill('0') << std::setw(2) << g
	   << std::setfill('0') << std::setw(2) << b;
	return std::string("#") + ss.str();
}

const img::Color &img::Color::NOT_COLOR()
{
	static Color NG = img::Color(std::numeric_limits<int>::max(),
								 std::numeric_limits<int>::max(),
								 std::numeric_limits<int>::max(), 0);
	return NG;
}

img::Color img::Color::getDefaultColor(int i)
{
    static const std::string V = "ee";
    static const std::string SH = "#";
	static std::unordered_map<int, Color> cmap {
        {0, Color(SH+V+"99cc")}, {1, Color(SH+"0000"+V)}, {2, Color(SH+"cc99"+V)}, {3, Color(SH+V+"00"+V)},
        {4, Color(SH+V+"8000")}, {5, Color(SH+V+V+"99")}, {6, Color(SH+"80"+V+"00")}, {7, Color(SH+"99"+V+"99")},
        {8, Color(SH+"00"+V+"80")}, {9, Color(SH+"99"+V+V)}, {10, Color(SH+"0080"+V)}, {11, Color(SH+"9999"+V)},
        {12, Color(SH+"7f00"+V)}, {13, Color(SH+V+"99"+V)}, {14, Color(SH+V+"007f")}, {15, Color(SH+V+"cc99")},
        {16, Color(SH+V+V+"00")}, {17, Color(SH+"cc"+V+"99")}, {18, Color(SH+"00"+V+"00")}, {19, Color(SH+"99"+V+"cc")},
        {20, Color(SH+"00"+V+V)}, {21, Color(SH+"99cc"+V)},
	};
	if(i >= static_cast<int>(cmap.size())) {
		return cmap.at(i%cmap.size());
	} else {
		return cmap.at(i);
	}
}


bool img::operator ==(const img::Color &c1, const img::Color &c2) {
	// 1個でも値が違えばfalse
	return (c1.r != c2.r || c1.g != c2.g || c1.b != c2.b || std::abs(c1.a - c2.a) > 1e-4) ? false : true;
}

std::ostream &img::operator <<(std::ostream &os, const img::Color &color)
{
	os << color.toRgbString() << " A=" << color.a;
	return os;
}


bool img::isRgbString(const std::string &rgbStr)
{
	return (rgbStr.empty() || rgbStr.substr(0, 1) != "#"
			|| rgbStr.substr(1).find_first_not_of("0123456789abcdefABCDEF") != std::string::npos);
}


#define VARNAME(ARG) #ARG
picojson::value img::Color::jsonValue() const
{
	/*
	 * Colorの内部変数は
	 * 	int r;
	 * 	int g;
	 * 	int b;
	 * 	double a;
	 * なので、
	 *
	 * ["r":rvalue, "g":gvalue, "b":bvalue, "a":alpha]
	 *
	 * というjsonvalueを作る。
	 *
	 */

	picojson::object colorObj;
	colorObj.insert(std::make_pair(VARNAME(r), static_cast<double>(r)));
	colorObj.insert(std::make_pair(VARNAME(g), static_cast<double>(g)));
	colorObj.insert(std::make_pair(VARNAME(b), static_cast<double>(b)));
	colorObj.insert(std::make_pair(VARNAME(a), static_cast<double>(a)));
	return picojson::value(colorObj);
}

img::Color img::Color::fromJsonObject(picojson::object obj)
{
	auto red = static_cast<int>(obj[VARNAME(r)].get<double>());
	auto green = static_cast<int>(obj[VARNAME(g)].get<double>());
	auto blue = static_cast<int>(obj[VARNAME(b)].get<double>());
	auto alpha = static_cast<int>(obj[VARNAME(a)].get<double>());

	return Color(red, green, blue, alpha);
}
#undef VARNAME
