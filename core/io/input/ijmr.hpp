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
#ifndef IJMR_HPP
#define IJMR_HPP

#include <string>
#include <vector>

namespace inp {

// strから区切り文字sep(複数種類可)で区切った最初の成分をカットして返り値で返す。
std::string cutFirstElement(const std::string &sep, std::string *str);

namespace ijmr {


/*
 * ijmr展開について
 * ijmrは、n[ijmr]のように前に整数を付けて入力される。無い場合は1がデフォルト。
 * i,mは一個前の入力が数値でなければならない。
 * jは前後の制限なし
 * rは前に何らかの入力(文字列、数値)が必要
 *
 * Q.セルカードでijmrは使われるか？
 *   →TRCL, FILLで使われる。*
 * Q.サーフェイスカードで使われるか？→使われる。故にsurface名と引数は区別する必要がある。
 * あと粒子名にiとかjがあるので、データカードや particle designatorでは区別する必要がある。
 */
// string*を引数にとってijmrを展開する。結局のところijmrを適用してよいのは特定カードのargument部分のみ。
// 本来例外でも投げればよいが、位置を示しながら警告するためにpos文字列を引数に取っている。
// separatorsは区切り文字。phitsなら" "のみ。mcnpなら=も区切り文字にするべし。
// 返り値は展開を実行したか否か
bool expandIjmrExpression(const std::string &separators, std::string *str);

// ijmr表現に該当する文字列ならtrue
bool checkIjmrExpression(const std::string &str);


}  // end namespace ijmr
}  // end namespace inp




#endif // IJMR_HPP
