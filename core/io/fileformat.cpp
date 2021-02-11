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
#include "fileformat.hpp"

#include <algorithm>
#include <unordered_map>

namespace {

const std::unordered_map<ff::FORMAT3D, std::string> &getFormat3DMap()
{
    static std::unordered_map<ff::FORMAT3D, std::string> suffMap {
        {ff::FORMAT3D::VTP, "vtp"},
        {ff::FORMAT3D::VTK, "vtk"},
        {ff::FORMAT3D::STL, "stl"},
        {ff::FORMAT3D::PLY, "ply"}
    };
    return suffMap;
}


const std::unordered_map<ff::FORMAT2D, std::string> &getFormat2DMap()
{
    static std::unordered_map<ff::FORMAT2D, std::string> suffMap {
        {ff::FORMAT2D::PNG, "png"},
        {ff::FORMAT2D::JPG, "jpg"},
        {ff::FORMAT2D::XPM, "xpm"},
        {ff::FORMAT2D::BMP, "bmp"}
    };
    return suffMap;
}

    const std::unordered_map<ff::FORMAT2DV, std::string> &getFormat2DVMap()
{
    static std::unordered_map<ff::FORMAT2DV, std::string> suffMap {
        {ff::FORMAT2DV::EPS, "eps"},
        {ff::FORMAT2DV::PDF, "pdf"},
        {ff::FORMAT2DV::PS, "ps"},
        {ff::FORMAT2DV::SVG, "svg"},
        {ff::FORMAT2DV::TEX, "tex"},
        };
    return suffMap;
}


}  // end anonymous namespace


const std::string ff::getFormat3DSuffix(FORMAT3D format)
{
    static auto suffMap = getFormat3DMap();
    return (suffMap.find(format) == suffMap.end()) ? "" : suffMap.at(format);
}


ff::FORMAT2D ff::strToFormat2D(const std::string &str)
{
    for(const auto &sufPair: getFormat2DMap()) {
        if(sufPair.second == str) return sufPair.first;
    }
    return ff::FORMAT2D::NOT_DEFINED;
}
ff::FORMAT2DV ff::strToFormat2DV(const std::string &str)
{
    for(const auto &sufPair: getFormat2DVMap()) {
        if(sufPair.second == str) return sufPair.first;
    }
    return ff::FORMAT2DV::NOT_DEFINED;
}

ff::FORMAT3D ff::strToFormat3D(const std::string &str)
{
    for(const auto &p: getFormat3DMap()) {
        if(p.second == str) return p.first;
    }
    return ff::FORMAT3D::NOT_DEFINED;
}



const std::string ff::getFormat2DSuffix(FORMAT2D format)
{
    static auto suffMap = getFormat2DMap();
    return (suffMap.find(format) == suffMap.end()) ? "" : suffMap.at(format);
}

const std::string ff::getFormat2DVSuffix(ff::FORMAT2DV format)
{
    static auto suffMap = getFormat2DVMap();
    return (suffMap.find(format) == suffMap.end()) ? "" : suffMap.at(format);
}

std::string ff::format2DToStr(ff::FORMAT2D format)
{
    auto suffix = getFormat2DSuffix(format);
    std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
    return suffix;
}



std::string ff::format2DVToStr(ff::FORMAT2DV format)
{
    auto suffix = getFormat2DVSuffix(format);
    std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
    return suffix;
}



std::string ff::format3DToStr(ff::FORMAT3D format)
{
    auto suffix = getFormat3DSuffix(format);
    std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
    return suffix;
}
