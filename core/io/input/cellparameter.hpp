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
#ifndef CELLPARAMETER_HPP
#define CELLPARAMETER_HPP

#include <string>

#include <utility>

namespace inp{

bool isCellParam(std::string str);

// strが固定長パラメータならtrueとパラメータ数のペアを返す。
std::pair<bool, int> isCellFixedParam(const std::string &str);

// TRCL=...から始まる文字列cellStrを与えて、 trStringにTR文字列を格納する。読み込んだ部分はcellStrから削除する。
// 第一引数がtrueならdegree単位入力の"*"をTr文字列の先頭に追加する。
void getCellTrStr(bool hasAsterisk, std::string *cellStr, std::string *trString);



}  // end namespace inp
#endif // CELLPARAMETER_HPP
