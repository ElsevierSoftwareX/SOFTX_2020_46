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
#ifndef VECTOR_UTIL_H
#define VECTOR_UTIL_H

#include <algorithm>
#include <iostream>
#include <list>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "message.hpp"

namespace inp {
class DataLine;
}

namespace utils {
template <class T>
bool isAscendant(const std::vector<T> &srcVector, bool acceptSameValue = false) {
	for(size_t i = 0; i < srcVector.size()-1; ++i) {
		if(acceptSameValue) {
			if(srcVector[i+1] < srcVector[i]) return false;
		} else {
			if(srcVector[i+1] <= srcVector[i]) {
                std::cout << "v[i+1]=" << srcVector[i+1] << ", v[i]=" << srcVector[i] << std::endl;
				return false;
			}
		}
	}
	return true;
}

template<class T>
T INVALID_INDEX()
{
	return std::numeric_limits<T>::max();
}

/*
 * valueが含まれる区間の下限のindexを返す。
 * valueが下限に一致する場合は区間に含むものとする。
 */
template <class T>
std::size_t getLowerBoundIndex(const std::vector<T> &srcVector, const T& value)
{
	// 当然srcVectorは昇順でなければだめ
	if(srcVector.empty()) {
		throw std::invalid_argument("ProgramError: Argument vector of getLowerBoundIndex is empty");
	} else if(!utils::isAscendant(srcVector, false)) {
		std::stringstream ss;
		ss << "ProgramError: Argument vector of getLowerBoundIndex should be ascendant. vec = ";
		ss << srcVector;
		throw std::invalid_argument(ss.str());
	}

//	// 逐次比較版
//	size_t linIndex =0;
//	for(std::size_t i = 0; i < srcVector.size()-1; ++i) {
//		if(srcVector[i] <= value &&  value < srcVector[i+1]) {
//			//return i;
//			linIndex = i;
//			return linIndex;
//		}
//	}
//	throw std::out_of_range("value out of vector");

	// 2分探索
	auto itr = std::lower_bound(srcVector.cbegin(), srcVector.cend(), value);
	if(itr == srcVector.begin() || itr == srcVector.end()) {
		std::stringstream ss;
		ss << "value(=" << value << ") is out of vector{";
		for(size_t i = 0; i < srcVector.size(); ++i) {
			ss << srcVector.at(i);
			if(i != srcVector.size()-1) ss << ", ";
		}
		ss << "}";
		throw std::out_of_range(ss.str());
	} else {
		return std::distance(srcVector.cbegin(), itr) - 1;
	}
}

// valueの含まれる区間の上限、下限indexペアを返す
template <class T>
std::pair<size_t, size_t> getBoundIndexes(const std::vector<T> &srcVector, const T &value)
{
	// 実装はgetLowerBoundIndexのものを使う。昇順チェックなどもそちらで。
	size_t lowerIndex = 0;
	try{
		lowerIndex = getLowerBoundIndex(srcVector, value);
	} catch (std::out_of_range &oor) {
        (void) oor;
		return std::make_pair(INVALID_INDEX<size_t>(), INVALID_INDEX<size_t>());
	}
	return std::make_pair(lowerIndex, lowerIndex+1);
}



// vec1 とvec2を連結したvectorを返す。
template <class T>
std::vector<T> makeConcatVector(const std::vector<T> & vec1, const std::vector<T> &vec2)
{
	std::vector<T> retVec(vec1);
	std::copy(vec2.begin(), vec2.end(), std::back_inserter(retVec));
	return retVec;
}

std::vector<double> normalize(const std::vector<double> &vec);


// この辺からDataLineリストの操作
std::list<inp::DataLine>::const_iterator findDataLine(const std::list<inp::DataLine> &inputList,
													  const std::string targetStr);

// list<dataline>をtitlePatternに合致する部分から次のtitleまでで分割する。
std::vector<std::list<inp::DataLine>>
    splitDataLineListByTitle(const std::list<inp::DataLine> &dataLines,
					  const std::regex &titlePattern, const std::regex &excludePattern);
}


//namespace utils{
//// unordered_mapで"最初に見つかった"VALを持つ値へのiteratorを返す。
//// NOTE VALは一意ではなく、unorderedなので値が複数あると予期した値が返らない
//template<class KEY, class VAL>
//typename std::unordered_map<KEY, VAL>::const_iterator findMapValue(const std::unordered_map<KEY, VAL> umap, const VAL& value)
//{
//	for(auto it = umap.begin(); it != umap.end(); ++it) {
//		if(it->second == value) return it;
//	}
//	return umap.end();
//}
//}  // end namespace utils
#endif // VECTOR_UTIL_H
