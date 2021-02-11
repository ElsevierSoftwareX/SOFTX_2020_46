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
#ifndef TYPE_UTILS_HPP
#define TYPE_UTILS_HPP

#include <string>
#include <type_traits>

template <typename T>
struct Type
{
	static std::string str() {
		if(std::is_enum<T>::value) {
			return "enum";
		} else if (std::is_arithmetic<T>::value) {
			return "arithmetic";
		} else {
			return "undefined";
		}
	}
};
// 使用頻度の高い型は特殊化する。それ以外はarithmeticかどうかだけ判定すれば良い。
// charはarithmeticと判定されるがテキスト処理上はarithmeticとして扱わないので必ず特殊化する。
// どうせエラー出力にしか使わないから割と適当で良い。特殊化の代わりにstd::is_same<>でもよい。
template<> struct Type<char> {static std::string str() {return "char";} };
template<> struct Type<int> {static std::string str() {return "int";} };
template<> struct Type<double> {static std::string str() {return "double";} };
template<> struct Type<float> {static std::string str() {return "float";} };
template<> struct Type<std::size_t> {static std::string str() {return "size_t";} };
template<> struct Type<std::string> {static std::string str() {return "string";} };

#endif // TYPE_UTILS_HPP
