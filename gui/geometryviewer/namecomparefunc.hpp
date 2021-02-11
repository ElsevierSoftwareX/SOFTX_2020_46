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
#ifndef NAMECOMPAREFUNC_HPP
#define NAMECOMPAREFUNC_HPP

#include <string>

// 文字列比較
bool isIntName(const std::string &str);

// s1, s2が共に数値なら数値の昇順になるように比較。
// 何れかが数値なら数値<文字列 となるように比較
// 何れもが文字列なら辞書順で比較
bool numberDictLess(const std::string &s1, const std::string &s2);

bool cellNameLess (const std::string &s1, const std::string &s2);

#endif // NAMECOMPAREFUNC_HPP
