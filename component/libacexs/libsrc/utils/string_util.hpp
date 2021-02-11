/*
 * string_util.hpp
 *
 *  Created on: 2010/01/22
 *      Author: sohnishi
 */

#ifndef STRING_UTIL_HPP_
#define STRING_UTIL_HPP_

#include <string>
#include <vector>
#include <iostream>

namespace utils {


std::string GetDataBlock(const int& num, const std::string& str);

// OSの要求する文字コードへエンコードする。
std::string toEncodedString(const std::string &str);

std::string toUTF8(const std::string &str);

}  // end namespace utils

class dDebug {
public:
	template <class T>
	dDebug operator << (T val) {
		std::cout << " " << val << " ";
		return *this;
	}

	template <class T>
	dDebug operator << (std::vector<T> vec) {
		for(auto& elem: vec) {
			std::cout << elem << std::endl;;
		}
		return *this;
	}


	~dDebug() {std::cout << std::endl;}
};

#endif /* STRING_UTIL_HPP_ */
