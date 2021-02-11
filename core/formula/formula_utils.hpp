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
#ifndef FORMULA_UTILS_HPP
#define FORMULA_UTILS_HPP

#include <string>
#include <utility>
#include <vector>


namespace formula {

// 左から検索して最初に出現した一番外側の括弧の位置pair(左側、右側)を返す
std::pair<std::string::size_type, std::string::size_type> findOutmostParenthesis(const std::string &source);

// 一番外側の括弧の外のseparatorでsourceを分割する。
// ignoreQuoteをfalseにすると ""で囲まれている部分は分割しないようにする。
// separatorが見つからない場合は1成分vectorを返す。
std::vector<std::string> splitOutmost(const std::string &source,
										char leftBracket, char rightBracket,
										char separator,
										bool ignoreQuote = true);

// str内のtargetCharの隣にacceptableCharsに含まれる以外の文字が存在した場合、その文字の位置のiteratorを返す。
// checkLeft == trueならtargetCharの左隣を、それ以外の場合は右隣をチェックする。
std::string::const_iterator checkNextChar(const std::string &str, char targetChar,
										  const std::string &acceptableChars, bool checkLeft);

// 左側括弧前に論理ANDが省略されている場合演算子文字列を補って修復する。
void fixOmittedOperator(std::string* equationStr, char padding,
						const std::string &leftAcceptables, const std::string &rightAcceptables);

// 妥当な式の文字列かチェック
void checkValidEquation(const std::string & factorStr, const std::string &op_chars);

// 論理和演算を含まない因子のみの妥当な文字列かチェック
bool isOnlyFactors(const std::string &factorStr, const std::string &addAndSubOps);

}  // end namespace form

#endif // FORMULA_UTILS_HPP
