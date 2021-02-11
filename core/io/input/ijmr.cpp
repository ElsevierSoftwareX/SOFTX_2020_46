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
#include "ijmr.hpp"

#include <regex>
#include <stdexcept>

#include "common/commoncards.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/numeric_utils.hpp"
#include "core/utils/message.hpp"

std::string inp::cutFirstElement(const std::string &sep, std::string *str)
{
	if(!str) {
		throw std::invalid_argument("cutFirstElement called for empty string pointer.");
	} else if(str->empty()) {
		return "";
	}

	std::string::size_type start = str->find_first_not_of(sep); // 最初の要素の先頭文字
	if(start == std::string::npos) {
		*str = "";
		return "";
	}

	std::string retstr;
	std::string::size_type end = str->find_first_of(sep, start+1); // 要素の後ろのsepの位置。
	if(end == std::string::npos) {
		// 次のデータがない場合
		retstr = *str;
		str->clear();
	} else {
		// 次のデータがある場合
		retstr = str->substr(start, end-start);
		*str = str->substr(end);
		start = str->find_first_of(sep);
		while(start == 0) {
			*str = str->substr(start+1);
			start = str->find_first_of(sep);
		}
	}
	return retstr;
}

// separatorsは区切り文字。だいたい" "か" ="のどちらか
bool inp::ijmr::expandIjmrExpression(const std::string &separators, std::string *str)
{
	bool hasExpanded = false;
	if(!str || str->empty()) return false;

	static const std::regex leftRegex(R"(\()");
	static const std::regex rightRegex(R"(\))");
	static const std::regex commaRegex(R"(,)");
	// "(2 r 2j, 0)"を "(2 2 j j, 0)"に展開するため、 予め"( 2 r 2j , 0 )"へ置換する。
	// もし余剰スペースが問題になるならこの関数の最後で"( " → "("への置換をかければ良い。
	*str = std::regex_replace(*str, leftRegex, "( ");
	*str = std::regex_replace(*str, rightRegex, " )");
	*str = std::regex_replace(*str, commaRegex, " , ");
	std::vector<std::string> params = utils::splitString('\"', separators, *str);
	//mDebug() << "Enter expandIjmrExpression, str===" << *str;

	std::smatch sm;
	for(std::size_t i = 0; i < params.size(); i++) {
		if(std::regex_search(params.at(i), sm, inp::comm::iPattern())) {
			hasExpanded = true;
			if(i == 0 || i == params.size()-1) {
				// iがパラメータの最初か最後にでたら補間できないのでエラー
				throw std::invalid_argument("i-expression was found in the beginning/end");
			} else if(!utils::isArithmetic(params.at(i-1)) || !utils::isArithmetic(params.at(i+1))){
				// iの次に文字列データかimr(imrも展開前は文字列と解釈されるので同時にチェックできる)が来たらエラー
				throw std::invalid_argument(" Not a number was found before/after i-expression");
			}

			std::string numstr = sm.str(1);
			int num_points = (numstr.empty()) ? 1 : utils::stringTo<int>(numstr);
			bool isLog = !sm.str(2).empty();  // sm.str(2)はilogなら"log"、iだけなら空が入る。
			/*
			 * i処理。
			 *
			 */
			double prev = utils::stringTo<double>(params.at(i-1));
			double next = utils::stringTo<double>(params.at(i+1));
			double increment = 0;
			if(isLog) {
				if(utils::isSameDouble(0, next) || next/prev < 0) {
					std::stringstream ss;
					ss << "invalid data around \"ilog\". prev = " << prev << ", next = " << next;
					throw std::invalid_argument(ss.str());
				}
				increment = std::pow(10.0, std::log10(next/prev)/double(num_points + 1));
			} else {
				increment = (next - prev)/double(num_points + 1);
			}

			std::vector<std::string> iVec; // i表現を置換するvector
			double tmpValue = prev;
			for(int ui=0; ui < num_points; ui++){
				if(isLog) {
					tmpValue *= increment;
				} else {
					tmpValue += increment;
				}
				iVec.emplace_back(utils::toString(tmpValue));
			}

			// iを展開した文字列vectorを挿入
			// 現在のiが示す位置までiteratorを移動させる。現在のi表現文字列自体は削除する。
			std::vector<std::string>::iterator itr = params.begin();
			std::advance(itr, i);
			itr = params.erase(itr); // i表現 elementは削除する。
			params.insert(itr, iVec.begin(), iVec.end());

		} else if(std::regex_search(params.at(i), sm, inp::comm::mPattern())) {
			hasExpanded = true;
			/*
			 *  m表現の展開
			 * 一個前の入力をn倍
			 */
			//mDebug() << "m-patternfound = " << params.at(i);
			// mが最初に来たらエラー
			if( i == 0 ){
				throw std::invalid_argument("m-expression is found at the first datablock.");
			}
			// mの前が非数値ならエラー
			if( !utils::isArithmetic(params.at(i-1))){
				throw std::invalid_argument("Not a number is found before m-expression");
			}
			double factor = utils::stringTo<double>(params.at(i).substr(0, params.at(i).size()-1));
			params.at(i) = utils::toString(	factor*utils::stringTo<double>(params.at(i-1)));

		} else if(std::regex_search(params.at(i), sm, inp::comm::rPattern())) {
			hasExpanded = true;
			/*
			 *  r表現の展開
			 * 1個前の表現をn回反復
			 */
			// mDebug() << "r-pattern found =" << params.at(i);
			// rが最初に来たらエラー
			if(i == 0){
				throw std::invalid_argument("r-expression is found at the first datablock.");
				// rの前が非数値ならエラー
			}else if(!utils::isArithmetic(params.at(i-1))){
				throw std::invalid_argument("Not a number is found before r-expression. prev="
											+ params.at(i-1) + ", str=" + *str);
			}

			std::string prev_data = params.at(i-1);
			std::size_t num_repeat = 1;
			if(std::string(sm.str()).size() > 1) {
				num_repeat = utils::stringTo<unsigned int>(params.at(i).substr(0, params.at(i).size()-1));
			}
			// 現在のiが示す位置までiteratorを移動させる。現在のr表現文字列自体は削除する。
			auto itr = params.begin();
			std::advance(itr, i);
			itr = params.erase(itr);
			for( unsigned int ui=1; ui<=num_repeat; ui++){
				itr = params.insert(itr, prev_data);
			}
			/*
		 * j表現の展開
		 * これはnjをn個のjにするだけ。
		 */
		} else if(std::regex_search(params.at(i), sm, inp::comm::jPattern())) {
			hasExpanded = true;
			// j 表現は 2j  ->  j j への展開しかしない。のでまずこのリピート数2を取得する。
			std::size_t num_repeat = 1;
			if(std::string(sm.str()).size() > 1) {
				num_repeat = utils::stringTo<int>(params.at(i).substr(0, params.at(i).size() - 1));
			}
			//mDebug() << "jpart=" << jpart << "num_repeat=" << num_repeat;
			auto itr = params.begin();
			std::advance(itr, i);
			itr = params.erase(itr);
			for(std::size_t i = 0; i < num_repeat; i++) {
				itr = params.insert(itr, "j");
			}
			i += num_repeat-1; // 追加したデータ数(num_repeat-1)だけ進める
		}
	}
	*str = utils::concat(params, " ");
	static const std::regex leftSpaceRegex(R"(\( )");
	static const std::regex rightSpaceRegex(R"( \))");
	static const std::regex commaSpaceRegex(R"( , )");
	// 速度的に問題ならおそらく余剰空白は放置しても構わない。
	*str = std::regex_replace(*str, leftSpaceRegex, "(");
	*str = std::regex_replace(*str, rightSpaceRegex, ")");
	*str = std::regex_replace(*str, commaSpaceRegex, ",");
	//mDebug() << "Exit expandIjmrExpression,  str===" << *str;
	return hasExpanded;
}

bool inp::ijmr::checkIjmrExpression(const std::string &str)
{
	if(str.empty()) return false;

	return     std::regex_search(str, inp::comm::iPattern())
			|| std::regex_search(str, inp::comm::mPattern())
			|| std::regex_search(str, inp::comm::rPattern())
			|| std::regex_search(str, inp::comm::jPattern());
}
