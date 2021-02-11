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
#include "formula_utils.hpp"

#include <algorithm>
#include <regex>
#include <stdexcept>

#include "core/utils/message.hpp"
#include "core/formula/logical/lpolynomial.hpp"

std::vector<std::string> formula::splitOutmost(const std::string &source, char leftBracket, char rightBracket,
											   char separator, bool ignoreQuote)
{
	std::string eqstr = source;
	std::vector<std::string> termStrings;
	std::string::size_type separatorPos, previousPos = 0;
	/*
	 * 1．separatorの位置を探す。
	 * 2．先頭からseparatorまでの区間内の左右括弧数が同じならそのseparatorは括弧外
	 * 3．よって区間内の文字列を保存・削除して1．へ戻る。
	 */
	while(separatorPos = eqstr.find_first_of(separator, previousPos), separatorPos != std::string::npos) {
		std::string tmpStr = eqstr.substr(0, separatorPos);
		if(std::count(tmpStr.begin(), tmpStr.end(), leftBracket) == std::count(tmpStr.begin(), tmpStr.end(), rightBracket)){
			termStrings.emplace_back(tmpStr);
			eqstr = eqstr.substr(separatorPos+1);
			previousPos = 0;
		} else {
			previousPos = separatorPos + 1;
		}
	}
	// 最後の残存部分の括弧が非対称なら括弧が左右matchしていないから例外発生にする
	if(std::count(eqstr.begin(), eqstr.end(), leftBracket) != std::count(eqstr.begin(), eqstr.end(), rightBracket)) {
		throw std::invalid_argument(std::string("Blacket not matched in string =") + source);
	}

	if(!eqstr.empty()) termStrings.emplace_back(eqstr);

	if(ignoreQuote) return termStrings;



	std::vector<std::string> retvec;
	std::string buff="";
	bool isDataCompleted = true;
	for(auto &val: termStrings) {
		// 最初に”が奇数個出てきたら次に”が奇数個出るまでは連結対象データ
		if(std::count(val.begin(), val.end(), '"')%2 != 0) {
			// 連結部分終了時にはbuffにdelimを追加する。
			if(!isDataCompleted && !buff.empty()) buff += std::string{separator};
			isDataCompleted = !isDataCompleted;
		}
		if(isDataCompleted) {
			buff += val;
			retvec.emplace_back(buff);
			buff.clear();
			isDataCompleted = true;
		} else {
			if(!buff.empty()) buff += std::string{separator};  // データ連結時には頭以外にはdelimで連結する。
			buff += val;
		}
	}
	if(!buff.empty()) {
		throw std::invalid_argument(std::string("Quotation is not closed, str=\"") + source + "\"");
	}
	return retvec;
}



// str内のtargetCharの隣にacceptableCharsに含まれる以外の文字が存在した場合、
// 修復文字でinsertすべき位置のiteratorを返す。
// checkLeft == trueならtargetCharの左隣を、それ以外の場合は右隣をチェックする。
// 最初に出てきた箇所を返す。
std::string::const_iterator formula::checkNextChar(const std::string &str, char targetChar,
										  const std::string &acceptableChars, bool checkLeft)
{
	std::smatch sm;
	static const std::string metaChars =".^$|[]()\\+*?"; // 正規表現でメタ文字として扱われる文字
	// targetCharがメタ文字ならエスケープする。
//	std::string targetStr{targetChar};
	std::string targetStr = (metaChars.find(targetChar) != std::string::npos) ? "\\" + std::string{targetChar} : std::string{targetChar};
	std::string regStr = (checkLeft) ? "(.)" + targetStr : targetStr + "(.)";
	const std::regex reg(regStr);

	std::string::const_iterator it = str.cbegin();
	while (it != str.cend()) {
		if(std::regex_search(it, str.cend(), sm, reg)) {
			assert(sm.str(1).size() == 1);
			// sm.str(1)がチェック対象なので、これがacceptableCharsに含まれないならその位置のiteratorを返す。
			if(acceptableChars.find(sm.str(1).front()) == std::string::npos) {
				return checkLeft ? sm[1].second: sm[1].first;
			} else {
				// acceptableCharsに含まれるなら検索継続
				it = checkLeft ? sm[1].second : sm[1].first;
			}
		} else {
			break;
		}
	}
	return str.cend();
}




// 左括弧の左側に演算子(AND)が省略されている場合、それを復元する。
// equationStr内に括弧があった場合はLPolynimialクラスでは再帰処理されるのでここでは一番左の括弧だけに着目すればよい
// 左括弧の左側に何が来たらANDを足すべきかは要検討
// 例えば"(("は足さなくて良い。 ")("は足すべし、"c1(" は足すべし、など

/*
 * equationStr中の
 * 左括弧の左隣がleftAcceptablesに含まれない文字の場合、左隣にpaddingを挿入する。
 * 右括弧の右隣がrightAcceptablesに含まれない文字の場合、右隣にpaddingを挿入する。*
 */
void formula::fixOmittedOperator(std::string *equationStr, char padding,
								 const std::string &leftAcceptables, const std::string &rightAcceptables)
{
	// そもそも括弧を含まない数式はfixする必要がない。
	if(equationStr->find_first_of("()") == std::string::npos) return;

	// 左側チェック
	std::string::const_iterator it;
	while(it = checkNextChar(*equationStr, '(', leftAcceptables, true), it != equationStr->cend()) {
		equationStr->insert(it, padding);
	}

	// 右側チェック
	while(it = checkNextChar(*equationStr, ')', rightAcceptables, false), it != equationStr->cend()) {
		equationStr->insert(it, padding);
	}
}


void formula::checkValidEquation(const std::string &equationStr, const std::string &op_chars)
{
	// 左右の括弧数が合わなければ例外発生
	// 左右括弧の数チェック
	if(std::count(equationStr.begin(), equationStr.end(), '(') != std::count(equationStr.begin(), equationStr.end(), ')')) {
		throw std::invalid_argument(std::string("parentheses mismatched str=") + equationStr);
	}

	// 左括弧の左側は演算子かコンプリメントか左括弧のみ許される
	auto it = checkNextChar(equationStr, '(', op_chars + "#(", true);
	if(it != equationStr.cend()) {
		throw std::invalid_argument(std::string("left of left-bracket should be operator or left-bracket, string=") + equationStr);
	}

	// 右括弧の右側チェック
	it = checkNextChar(equationStr, ')', op_chars + ")", false);
	if(it != equationStr.cend()) {
		throw std::invalid_argument(std::string("right of right-bracket should be operator or right-bracket, string=") + equationStr);
	}
}

// 第二引数は項を区切ることになる演算子(加算と減算、あるいは加算のみ)をまとめた文字列
bool formula::isOnlyFactors(const std::string &factorStr, const std::string &addAndSubOps)
{
	// 因子のみならtrue, 二項演算子を含むならfalse
	if(factorStr.find_first_of(addAndSubOps) != std::string::npos) {
		//mDebug() << "return false";
		return false;
	} else {
		//mDebug() << "return true";
		return true;
	}
}


std::pair<std::string::size_type, std::string::size_type> formula::findOutmostParenthesis(const std::string &source)
{
	std::string::size_type leftPos = source.find_first_of("(");
	if(leftPos == std::string::npos) {
		if(source.find(')') == std::string::npos) {
			return std::make_pair(std::string::npos, std::string::npos);
		} else {
			// ここのsourceは加工中の文字列だから出力しても意味不明になるだけ。なので例外文字列には含めない。
			throw std::invalid_argument(std::string("Too much right parenthesis (or forget {}?). str=") + source);
		}
	}
	int nestCount = 1;  // これから検索する部分のネスト深さ
	std::string::size_type rightPos, prevPos = leftPos;
	while(rightPos = source.find_first_of(")", prevPos + 1), rightPos != std::string::npos) {
		--nestCount;
		auto insidePar =  source.substr(prevPos+1, rightPos - prevPos - 1);
		//mDebug() << "pcound=" << pCount<< "括弧間=" << insidePar;
        nestCount += static_cast<int>(std::count(insidePar.begin(), insidePar.end(), '('));
		if(nestCount == 0) {
			return std::make_pair(leftPos, rightPos);
		}
		prevPos = rightPos;
	}

	// ↑のwhileループを抜けた場合マッチする右括弧なしだから例外
	throw std::invalid_argument(std::string("Too much left parenthesis (or forget {}?). str=") + source);
}

