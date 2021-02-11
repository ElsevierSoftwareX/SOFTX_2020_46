/*
 * AceUtils.hpp
 *
 *  Created on: 2015/11/06
 *      Author: sohnishi
 */

#ifndef ACE_ACEUTILS_HPP_
#define ACE_ACEUTILS_HPP_



#include <cassert>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>


namespace ace {

namespace tmp {
inline
double stod(const std::string& str) {
	std::stringstream ss;
	ss << str;
	double val;
	ss >> val;
	return val;
}
}

template <class T>
inline T Sto(const std::string& str) {
	std::cerr << "non-specialized Sto<T> should not be used. source string = " << str;
	abort();
	return T(0);
}

template<>
inline double Sto(const std::string& str) {
	return tmp::stod(str);
}
// std::stoi(3.100000000000E+01) は3になる！ので,整数はstodで受けてからintへキャストする。
template<>
inline int Sto(const std::string& str) {
	return static_cast<int>(tmp::stod(str));
}
template<>
inline long Sto(const std::string& str) {
	return static_cast<long>(tmp::stod(str));
}
template<>
inline std::size_t Sto(const std::string &str) {
	return static_cast<std::size_t>(tmp::stod(str));
}



template <class T>
std::vector<T> getData(std::size_t num_data, std::istream& is) {
	using namespace std;

	// num_data 個のデータを取得
	//
	vector<T> dvec(num_data);
	// ifs >> data  はdataがintなら 1.0000E0 は.を区切りとして認識して以降を読まない。だから一度stringで受ける。
	std::string temp;
	for(std::size_t i=0; i < num_data; i++) {
		if(is.eof())throw std::runtime_error("End of istreasm");
		is>>temp;
//		std::cout << "temp=" << temp << ", casted=" << Sto<T>(temp) << std::endl;
		dvec.at(i) = Sto<T>(temp);
	}

	assert(num_data == dvec.size());

	return dvec;
}

template <class T>
T getData(std::istream& is) {
	if(is.eof())throw std::runtime_error("End of istreasm");
	std::string buff;
	is >> buff;
	return Sto<T>(buff);
}


/*
 * xss_のpositionからnumberOfElement個のデータをT型として取得する。
 *
 * positionをどう解釈するか？
 * このpositionはマニュアルの記述に合わせるため、fortranスタイル
 * の1から始まるindexを取る。
 * 従ってposition == 0は許容されない。
 *
 * 例えば
 * ACEではエネルギー分点がデータの最初に来るが、この時の
 * データ開始位置(ESZ)はJXSテーブルで1が指定されており、
 * position == 1を与えて読み込む。
 */
template <class T>
std::vector<T> getXssData(const std::vector<std::string> &xss, int numberOfElement, int position) {
	std::vector<T> retvec;
	//std::cout << "number =" << numberOfElement << "offset=" << offset << std::endl;
	for(int i = position -1; i < position - 1 + numberOfElement; i++) {
		//std::cout << "i=" << i << "datastr = " << xss_.at(i) << std::endl;
		retvec.emplace_back(ace::Sto<T>(xss.at(i)));
	}
	return retvec;
}

template <class T>
T getXssData(const std::vector<std::string> &xss, int position) {
	return ace::Sto<T>(xss.at(position -1)); // Fortran(ACE)とc++ではindexが1個ずれる。
}

}  // end namespace ace

#endif /* ACE_ACEUTILS_HPP_ */
