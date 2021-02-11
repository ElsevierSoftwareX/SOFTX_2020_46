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
#ifndef FILEFORMAT_HPP
#define FILEFORMAT_HPP

#include <string>

namespace ff{

enum class FORMAT3D : int {NOT_DEFINED, VTP, VTK, STL, PLY};
enum class FORMAT2D: int {NOT_DEFINED, PNG, JPG, XPM, BMP};
enum class FORMAT2DV: int{NOT_DEFINED, EPS, PDF, PS, SVG, TEX};

bool isXMLFormat(FORMAT3D f);


const std::string getFormat3DSuffix(FORMAT3D format);
const std::string getFormat2DSuffix(FORMAT2D format);
const std::string getFormat2DVSuffix(FORMAT2DV format);


FORMAT3D strToFormat3D(const std::string &str);
FORMAT2D strToFormat2D(const std::string &str);
FORMAT2DV strToFormat2DV(const std::string &str);

std::string format3DToStr(FORMAT3D format);
std::string format2DToStr(FORMAT2D format);
std::string format2DVToStr(FORMAT2DV format);

}

class FileFormat
{
public:
    FileFormat();
};

#endif // FILEFORMAT_HPP
