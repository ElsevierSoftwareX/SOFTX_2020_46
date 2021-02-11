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
#include "trcard.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"
#include "core/formula/fortran/fortnode.hpp"
#include "core/utils/matrix_utils.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/ijmr.hpp"

namespace {
const int NUM_ORTHORELAX_LOOP = 50;
}


const std::regex &inp::comm::TrCard::regex()
{
//	static const std::regex pat = std::regex(R"(^ {0,4}[tT][rR][0-9]+)");
	//static const std::regex pat = std::regex(R"(^ {0,4}\*?[tT][rR][0-9]+)");
	static const std::regex pat = std::regex(R"(^ {0,4}(\*?)(TR)([-+0-9*]+) *([-+/*.(){}0-9a-zA-Z ]+ *$))", std::regex_constants::icase);
	return pat;
}

std::unordered_map<size_t, math::Matrix<4>>
	inp::comm::TrCard::makeTransformMap(const std::list<inp::DataLine> &inputLines)
{
	// ijmr展開はここで実施。
	std::string element;
	std::unordered_map<size_t, comm::TrCard> trCards;
	for(auto& dl: inputLines) {
		element = dl.data;
		std::string firstElement = inp::cutFirstElement(" ", &element);
		if(inp::ijmr::checkIjmrExpression(firstElement)) {
            throw std::invalid_argument(dl.pos() + " TRcard name should not be ijmr-expression. str=" + dl.data);
		}
		// TODO phits互換性
		inp::ijmr::expandIjmrExpression(" ", &element);
		element = firstElement + " " + element;
		try {
			// ここからTRカードパターン検索
			if(std::regex_search(element, TrCard::regex())) {
				TrCard trCard(element);
				if(trCards.find(trCard.idNumber()) != trCards.end()) {
                    std::stringstream ess;
                    ess << dl.pos() << " Multiple definition of TR" << trCard.idNumber();
                    throw std::invalid_argument(ess.str());
				}
				trCards.emplace(trCard.idNumber(), trCard);
			} else {
				// TransformSectionで非TRカードが出たら警告して無視。
                mWarning(dl.pos()) << "Warning! non-TR card found, ignored. data=" << dl.data;
			}
		} catch (std::exception& e) {
            throw std::invalid_argument(dl.pos() + " " + e.what());
		}
	}
	std::unordered_map<size_t, math::Matrix<4>> retMatMap;
	for(auto trPair: trCards) {
		retMatMap.emplace(trPair.first, trPair.second.trMatrix());
	}
	return retMatMap;
}

inp::comm::TrCard::TrCard(const std::string &str, bool warnCompat)
	:DataCard(str, TrCard::regex())
{
	// *はid_の末尾に付く場合もあるのでその時はmodifier_へ移す。
	if(!id_.empty() && id_.back() == '*') {
		modifier_ = id_.back();
		id_ = id_.substr(0, id_.size()-1);
	}
	// 負のTR番号は反射境界指定と重複するから認められない
	if(!utils::isArithmetic(id_) || utils::stringTo<int>(id_) < 0) {
		throw std::invalid_argument("Invalid TR card number=" + id_);
	}
	matrix_ = utils::generateSingleTransformMatrix(modifier_ + argument_, warnCompat);
}

std::string inp::comm::TrCard::toString() const
{
	std::stringstream ss;
	ss << DataCard::toString() + ", matrix = " << matrix_;
	return ss.str();
}

//inp::comm::TrCard::TrCard(std::string str, bool warnCompat)
//{
//	utils::trim(&str);
//	utils::toupper(&str); // Card生成までにはincludeを処理済みでなければならない。
//	// TR1 0 0 0 1 0 0 0 1 0 0 0 1 1  のTR1とその他を分ける必要がある。
//	std::string::size_type spacepos = str.find_first_of(" ");
//	if(spacepos == std::string::npos) throw std::invalid_argument("Invalid trcard input=" + str);


//	std::string numberStr;
//	// カード名 がTRnか*TRnでなければならない
//	name_ = str.substr(0, spacepos);
////	mDebug() << "name=" << name_;
//	// 最短でもTR1のように3文字はあるはず
//	if(name_.size() < 3) throw std::invalid_argument(std::string("TR card name \"") + name_ + "\" is invalid");

//	bool isDegreeInput = false;
//	if(name_.front() == '*') {  // 最初の文字が*ならcos入力
//		isDegreeInput = true;
//		numberStr = name_.substr(3);
//	} else if (name_.back() == '*') {
//		if(warnCompat) mWarning() << "Postpositional * in TR card is not phits compatible";
//		// TR2* のような場合TR番号は3文字めから1文字の部分となる。
//		isDegreeInput = true;
//		numberStr = name_.substr(2, name_.size() - 3);
//	} else {
//		numberStr = name_.substr(2);
//		//mDebug() << "numStr=" << numberStr;

//	}

//	// 負のTR番号は反射境界指定と重複するから認められない
//	if(!utils::isArithmetic(numberStr) || utils::stringTo<int>(numberStr) < 0) {
//		throw std::invalid_argument("Invalid TR card number=" + numberStr);
//	} else {
//		id_ = numberStr;//utils::stringTo<int>(numberStr);
//	}

//	std::string argStr = str.substr(spacepos+1);
//	//mDebug() << "name=" << name_ << "trN=" << number_ << "arg=" << argStr;
//	utils::trim(&argStr);
//	if(isDegreeInput) argStr = "*" + argStr;
//	matrix_ = utils::generateSingleTransformMatrix(argStr, warnCompat);
//}





std::ostream &inp::comm::operator <<(std::ostream &os, const inp::comm::TrCard &trc)
{
	os << trc.toString();
	return os;
}
