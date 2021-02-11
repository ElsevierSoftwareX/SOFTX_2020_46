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
#ifndef STRINGUTIL_HPP
#define STRINGUTIL_HPP

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "type_utils.hpp"

namespace utils {

//! @brief 文字列分割関数。複数種のセパレータを用いて引用符(複数種)を考慮しながら文字列を分割し、文字列ベクトルを返す。
std::vector<std::string> splitString(const std::unordered_map<char, char> &qmarkMap, const std::string& delims,
									 const std::string &sourceStr, bool ignoreEmpty = true);
std::vector<std::string> splitString(const std::pair<char, char> &qmarkPair, const std::string& delims,
									 const std::string &sourceStr, bool ignoreEmpty = true);
std::vector<std::string> splitString(char qmark, const std::string& delims,
									 const std::string &sourceStr, bool ignoreEmpty = true);
std::vector<std::string> splitString(const std::string& delims,
									 const std::string &sourceStr, bool ignoreEmpty = true);

//! @brief クオーテーションを外す。引用符は複数種左右非対称を考慮する。
std::string dequote(const std::unordered_map<char, char> &qmarkMap, std::string str, bool doTrim = true); // 複数記号、左右ペア版
std::string dequote(std::pair<char, char> marks, std::string str, bool doTrim = true);  // 左右の区別がある版
std::string dequote(char qmark, std::string str, bool doTrim = true); // 単一記号版



//! @brief 余分な一番外側の括弧を削除する。doTrim==trueならtrimする。
void removeRedundantBracket(char leftBracket, char rightBracket, std::string *str, bool doTrim);
//! @brief bracketPosにあるleftBracketに対応するrightBracketの位置を返す。
std::string::size_type findMatchedBracket(const std::string &str, std::string::size_type braketPos,
										  char leftBracket, char rightBracket);

// 括弧の対応が取れているかを調べる。左括弧があると-1、右括弧があると+1を積算し、トータルを返す。
// acceptMismatch==falseなら右括弧過剰の場合に例外を投げる。
int parenthesisBalance(char left, char right, const std::string &str, bool acceptMismatch);
// 文字列が長さszになるまでpadでパディングする。第四引数がtrueなら左側に、falseなら右側にpadを埋める
std::string paddedString(char pad, int sz, const std::string &str, bool left = true);
void uniteSpaces(std::string* str);  // 複数の連続する空白を1個にまとめる
void trim(std::string* str);
std::string trimmed(const std::string &str);
void toupper(std::string* str);
void tolower(std::string* str);
std::string lowerString(const std::string &str);
bool isArithmetic(const std::string& str);
bool isInteger(const std::string& str);
bool isArithmeticVector(const std::vector<std::string>& svec);
bool isArithmeticVector(const std::vector<std::string>::const_iterator beg,
					const std::vector<std::string>::const_iterator en);

//! @brief  "{n-m}" 文字列を"n n+1 n+2 ... m"に展開する
std::string expandRangeString(const std::string &str);

// 文字列が aaa/bbb/ccc なら 返り値のfirstはaaa/bbb/ secondはcccとなる。
std::pair<std::string, std::string> separatePath(const std::string& source);
// n 文字分の空白を返す
std::string spaces(size_t n);
// patternStrで始まっているか判定する
bool startWith(const std::string &source, const std::string &pattern);



/*
 * セル、サーフェイスの名前を正準な形に変換する。
 * 1．数値化できる文字列は一旦数値化してから文字列化する。例："0500" → "500"、 "-0010" → "-10"
 * 2．数値化出来ない文字列はそのまま。 例: "c1" 等はそのまま
 * 3．マクロボディの場合は小数点よる上の所に上記ルールを適用。
 *		"cone.3"はそのまま。 "0100.3" → "100.3"に変換
 * 4．反射境界や白色境界は頭文字を除いて換算 "*0500" → "*500"
 */
std::string canonicalName(const std::string &orgName);

// 補集合化する関数。
int complimentedFactor(const int fac);
std::string complimentedFactor(const std::string &fac);

// 以下テンプレート関数

template<class T>
std::string toString(const T& val)
{
	std::stringstream ss;
	ss << val;
	return ss.str();
}

template<class T>
std::string toString(const T& val, std::size_t prec)
{
	std::stringstream ss;
	ss<< std::setprecision(prec) << val;
	return ss.str();
}






/*!
 * 文字列から数値への変換を行う。動作は遅いので、引数が「確実に非浮動小数点表現の場合」std::sto[di]を検討すること。
 * @param[in] (str) 変換対象文字列
 * @return 変換結果
 * @sa std::stod
 *
 * 標準の文字列変換関数(stod, stoi)には不便ながあるので独自関数を作る。
 * C++標準関数は以下のような挙動を示す。
 * 1．std::stoi("1.00E+3")は 1になる。   -> 当関数では1000を返す
 * 2．std::stod("1.5abcde")は1.5になる   -> 例外発生させたい。
 * 3. std::stod("1.5+2") は1.5になる     -> 150にしたい
 * 3．std::stod("abcd1.5")は std::invalid_argument発生 -> これはこのままでOK.
 *
 * 上記の標準関数との挙動の違いは以下の通り。
 * ・浮動小数点表現文字列から整数への変換をサポートし、stringTo<int>("1E+3")は1000を返す。
 * ・不完全な文字列には例外を発生させる。 stringTo<double>("1.5abcde") はinvalid_argumentを発生させる
 * ・Fortran式浮動小数点表現に対応する。strintTo<double>("1.0+2")は100を返す。(但し非推奨として警告を出す)
 * ・整数へ丸める場合roundで最近接整数へ丸められる。 stringTo<int>("0.9999") は1を返す。
 * ・負数を表す文字列からunsignedな型変換しようとしたら警告
 */
template <class T>
inline T stringTo(std::string str) {
	std::size_t idx;
	utils::trim(&str);
	double retval;
	try {
		retval = std::stod(str, &idx);
	} catch (std::invalid_argument &e) {
        // DEBUG
//        (void) e;
//        throw std::invalid_argument("String conversion error");
        throw std::invalid_argument(std::string("Conversion of string \"") + str + "\" to " + Type<T>::str() + " failed."
                                    " " + e.what());
	}

	// 変換出来ていない部分がある場合。
	if(idx != str.size()) {
		// "1.5+06"のようなFortran式e無し浮動小数点表記の検知
		// 未変換部分の先頭が+-で以降が数字のみの場合はfortran表記
		std::string sign = str.substr(idx, 1);
		std::string expStr = str.substr(idx+1);
		if((sign == "+" || sign == "-")
			&& expStr.find_first_not_of("0123456789") == std::string::npos){
			std::cerr << "Warning! Old fortran style fp notation WITHOUT \"e\" is deprecated. src = " << str << std::endl;
			if(sign == "+") {
				retval *= std::pow(10, stringTo<double>(expStr));
			} else {
				retval *= std::pow(10, -stringTo<double>(expStr));
			}
		} else {
			throw std::invalid_argument(std::string("Conversion of str \"") + str + "\" to " + Type<T>::str() + " failed.");
		}
	}
	// 負の数をunsignedにキャストする場合は警告。
	if(!std::numeric_limits<T>::is_signed && retval < 0) {
		std::cerr << "Warning! Negative value string(=\"" + str + "\") is casted to unsigned type(" + Type<T>::str() << std::endl;
	}
	// 整数型へのキャストはroundを掛けてからになる。
	if(std::numeric_limits<T>::is_integer) retval = std::round(retval);

	return static_cast<T>(retval);
}
template<>
inline std::string stringTo(std::string str)
{
	return str;
}





// 第三引数を省略した場合はstartIndexより後ろ全てを変換する
// vector外を指定した場合はout_of_rangeを投げる。
// 変換できない文字があればinvalid_argumentを投げる。
template<class T>
std::vector<T> stringVectorTo(const std::vector<std::string> &vec,
							  const std::size_t& startIndex,
							  std::size_t numElements = std::numeric_limits<std::size_t>::max())
{
	if(numElements == std::numeric_limits<std::size_t>::max()) numElements = vec.size();
	// startIndex+numElementsがvecの外の場合std::out_of_rangeをなげる。
	if(startIndex + numElements - 1  > vec.size()) {
		std::stringstream ss;
		ss << "stringVectorTo, start index = " << startIndex << " is out of vector, size=" << vec.size();
		throw std::out_of_range(ss.str());
	}

	std::vector<T> retvec(numElements);
	for(std::size_t i = startIndex; i < startIndex + numElements; i++) {
		try {
			retvec.at(i-startIndex) = utils::stringTo<T>(vec.at(i));
		} catch (std::invalid_argument &e) {
            (void) e;
			throw std::invalid_argument(std::string("Conversion of \"") + vec.at(i)
										+ "\" from string to "  + Type<T>::str() + " failed " );
		}
	}
	return retvec;
}

template<class T>
std::vector<T> stringVectorTo(const std::vector<std::string> &vec)
{
	return stringVectorTo<T>(vec, 0, vec.size());
}


// ベクトルを sepで連結して1つの文字列にする。 operator(ostream, T)がある型なら使えるはず…
//std::string concat(const std::vector<std::string> &strVec, const std::string& sep = " ");
template <class T>
std::string concat(const std::vector<T> &vec, const std::string& sep = " ") {
	std::stringstream ss;
	for(std::size_t i = 0; i < vec.size(); i++) {
		ss << vec.at(i);
		if(i != vec.size()-1) ss << sep;
	}

	return ss.str();
}


// Tから整数型への変換
template<class T>
int toInt(const T& val) {return utils::stringTo<int>(utils::toString<T>(val));}
template<>
int toInt(const int& val);


// コメント文字以降を削除
void removeInlineComment(char commChar, std::string *str);


} // end namespace utils


#endif // STRINGUTIL_HPP
