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
#ifndef CGCONVERSIONMAP_HPP
#define CGCONVERSIONMAP_HPP

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>


namespace inp {
// CGbodyのmnemonic(string)とパラメータ(vector<double>)を受け取って、
// MCNPのサーフェイスカードで読めるカード入力文字列に変換する関数の型
using cgconv_functype = std::function<std::string(int, const std::string&, const std::vector<double>&)>;






const std::unordered_map<std::string, cgconv_functype> &getConvFuncMap();

} // end namespace inp
#endif // CGCONVERSIONMAP_H
