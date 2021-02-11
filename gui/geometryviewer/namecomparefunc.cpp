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
#include "namecomparefunc.hpp"

#include <algorithm>

bool numberDictLess(const std::string &s1, const std::string &s2)
{
	// 一番外側のセル名の比較は数値-辞書順。
	if(!isIntName(s1) || !isIntName(s2)) {
		if(isIntName(s1)) {
			return true;
		} else if (isIntName(s2)) {
			return false;
		} else {
			return s1 < s2;
		}
	} else {
		// 両者数値なら数値で比較
		return std::stoi(s1) < std::stoi(s2);
	}
}

bool cellNameLess(const std::string &s1, const std::string &s2)
{
	/*
	 * s1, s2を比較する。 trueならs1が前に、falseならs2が前の順になる
	 * セル名の並び順は
	 * ・数値文字列同士は数値にして比較
	 * ・数値文字列と文字列なら数値が前にくる
	 * ・lattice/universeは文字列とみなして後ろにし、かつ上位の外側セル名の辞書順
	 */
	if(isIntName(s1)) {
		if(!isIntName(s2)) {
			return true;
		} else {
			return std::stoi(s1) < std::stoi(s2);
		}
	}

	const char SEP = '<';
	auto elemPos1 = s1.rfind(SEP), elemPos2 = s2.rfind(SEP);

	if(elemPos1 == std::string::npos || elemPos2 == std::string::npos) {
		if(elemPos1 != std::string::npos) {
			// s1のみがuniv要素セル、s2は非要素
			return false;
		} else if(elemPos2 != std::string::npos) {
			// s2のみが要素セル
			return true;
		} else {
			// 両者非要素セルの場合辞書順比較
			return s1 < s2;
		}
	} else {
		// s1, s2共にelementセルの場合、universeの深い方を大きいとする。
		size_t depth1 = std::count(s1.begin(), s1.end(), SEP);
		size_t depth2 = std::count(s2.begin(), s2.end(), SEP);
		if(depth1 == depth2) {
			// 深さが同じ場合、一番外側のセル名で比較
			std::string outerCell1 = s1.substr(elemPos1+1);
			std::string outerCell2 = s2.substr(elemPos2+1);
			if(outerCell1 != outerCell2) {
				return numberDictLess(outerCell1, outerCell2);
			} else {
				// 最外殻セル名で順序が確定しないなら、充填セル名で再帰的に解決
				return cellNameLess(s1.substr(0, elemPos1),s2.substr(0, elemPos2));
			}
		} else {
			return depth1 < depth2;
		}
	}
}

bool isIntName(const std::string &str) {
	return str.find_first_not_of( "0123456789" ) == std::string::npos;
}
