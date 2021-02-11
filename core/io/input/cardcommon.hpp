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
#ifndef CARDCOMMON_HPP
#define CARDCOMMON_HPP

#include <regex>
#include <string>

namespace inp {

// TRCLでtransformされた後の面の名前を作成する。
std::string getTransformedSurfaceName(const std::string &tredCellName, const std::string &oldSurfaceName);


// cell名の正規表現 [-+.,_@<\\[\\]\\w]+
const std::string &cellNameRegexStr();
// surface名の正規表現文字列を返す。符号とsurface名で構成されsurface名はアンダースコアで始まらないこと
//"([-+]*)([-+.,_@<\\[\\]\\w]+)" でmatch(1)が符号、match(2)がsurface名
const std::string &surfaceNameRegexStr();
const std::string &acceptableUserInputNameRegexStr(); // ユーザーがサーフェイス名に入力して良い文字のregestr

// index番号からlattice要素セルの名前を生成する。
std::string indexToElementName(const std::string &baseName, int i, int j, int k);


// セル/サーフェイス名が妥当な文字列でできているかチェック。
// 第二引数がtrueの場合、ユーザー入力として[\w]*であることをチェックする。
void checkNameCharacters(const std::string &name, bool asUserInput);

bool appendCanonicalTrStr(const std::string &srcStr, std::string *trStr);


}// end namesapce inp
#endif // CARDCOMMON_HPP
