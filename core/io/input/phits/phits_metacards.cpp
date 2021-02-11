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
#include "phits_metacards.hpp"

#include <cassert>
#include <map>
#include <limits>
#include <regex>
#include <stdexcept>
#include <utility>

#include "core/utils/utils.hpp"
#include "core/utils/message.hpp"

namespace{

const std::regex rangePattern = std::regex(R"( *\[ *([0-9]*) *- *([0-9]*) *\] *)");
const std::regex formerPattern = std::regex(R"( *\[ *([0-9]*) *-)");
const std::regex latterPattern = std::regex(R"(- *([0-9]*) *\] *)");

}

// 入力ファイル打ち切りカードは　q:と[end]
const std::regex &inp::phits::getForceEndCard()
{
    static const std::regex forceEndPattern = std::regex(R"(^ {0,4}q:|\[\s*e\s*n\s*d\s*\])", std::regex::icase);
//    static const std::regex forceEndPattern = std::regex(R"(^ {0,4}q:)", std::regex::icase);
	return forceEndPattern;
}
const std::regex &inp::phits::getforceEndSectionCard()
{
    static const std::regex endSectionPattern = std::regex(R"(^ {0,4}qp:)", std::regex::icase);
	return endSectionPattern;
}

/*
 * set:はinfl:以外のメタカード処理はされていない前提で実行する必要があるのでしんどい。
 * 本当は行連結やコメントは処理してからset:を処理したいが、
 * セクションによって行連結等のルールが異なるため入力ファイル読み取り直後の
 * 行連結等は実施できない。
 */
const std::regex &inp::phits::getSetConstantPattern()
{
	static const std::regex pat= std::regex(R"(^ {0,4}[sS][eE][tT] *: *)");
	return pat;
}
const std::regex &inp::phits::getConstantPattern()
{
	static const std::regex pat= std::regex(R"((c\d{1,2})\[(.*?)\])");
	return pat;
}


// Phitsのインクルードは infl: { file.name } [ n − m ]
// 行番号[n-m]は省略可能なのでpatternには含めない。別の処理で取得する。
const std::regex &inp::phits::getIncludeCardPattern()
{
	static const std::regex includeCardPattern
//			= std::regex(R"(^ {0,4}[iI][nN][fF][lL]: *\{ *[a-zA-Z0-9_.]+ *\} *\[ *[0-9]* *- *[0-9]* *\])");
//			= std::regex(R"(^ {0,4}[iI][nN][fF][lL]: *\{ *([a-zA-Z0-9_.]+) *\})");
//			= std::regex(R"(^ *[iI][nN][fF][lL]: *\{ *([a-zA-Z0-9_.]+) *\})");
			= std::regex(R"(^ *[iI][nN][fF][lL]: *\{ *([^}]+) *\})"); // for multibyte
	return includeCardPattern;
}
const std::regex &inp::phits::getIncludeRangePattern()
{
	static const std::regex retPattern = std::regex(R"( *\[ *[0-9]* *- *[0-9]* *\] *)");;
	return retPattern;
}


// "[ n - m ]" から{n, m}を算出。
const std::pair<std::size_t, std::size_t> inp::phits::getFileRange(const std::string &str)
{
	std::smatch sm;
	// 行レンジ指定パターンに該当しない場合はstd::invalid_argumentを投げる
	if(!std::regex_search(str, sm, rangePattern)) {
		throw std::invalid_argument("Not valid line number range.");
	}

	// 開始位置読み取り
	std::size_t n = 0;
	if(std::regex_search(str, sm, formerPattern)) {
		// sm.str(0)は全体, sm.str(1)が括弧内に対応
		if(sm.size() == 2) {
			// 全体と括弧内で合計2マッチでOK。正常処理。
			std::string numberStr = sm.str(1);
			utils::trim(&numberStr);
			n = numberStr.empty() ? 0: utils::stringTo<long>(numberStr);
		} else {
			// 3箇所以上マッチはプログラムのパターンがおかしいのでユーザーエラーではなく即終了。
			std::cerr << "Error! Too much matched." << std::endl;
			std::exit(EXIT_FAILURE);
		}
	} else {
		// range指定文字列がおかしい。 std::invalid_argumentを投げる
		throw std::invalid_argument("Not valid range, start position.");
	}

	// 終了位置読み取り
	std::size_t m = inp::phits::MAX_LINE_NUMBER;
	if(std::regex_search(str, sm, latterPattern)) {
		if(sm.size() == 2) {
			// 全体と括弧内で合計2マッチでOK。正常処理。
			std::string numberStr = sm.str(1);
			utils::trim(&numberStr);
			m = numberStr.empty() ? inp::phits::MAX_LINE_NUMBER : utils::stringTo<long>(numberStr);
		} else {
			// 3箇所以上マッチはプログラムのパターンがおかしい。到達しないはず。
			std::cerr << "Error! Too much matched." << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}
	return std::make_pair(n, m);
}

const std::regex &inp::phits::getSectionPattern()
{
	// 非継続行で大カッコ内が全てスペースかアルファベット-ならsection該当とする。
	static const std::regex sectionPattern = std::regex(R"(^ {0,4}\[ *([-a-zA-Z0-9 ]+) *\])");
	return sectionPattern;
}

const std::regex &inp::phits::getDisabledSectionPattern()
{
	static const std::regex sectionPattern = std::regex(R"(^ {0,4}\[ *([-0-9a-zA-Z ]+) *\] *[Oo][Ff][Ff])");
	return sectionPattern;
}

void inp::phits::canonicalizeSectionName(std::string *sectionStr)
{
	// 空白除去
	std::string::size_type pos;
	while((pos = sectionStr->find_first_of(" ")) != std::string::npos){
	  sectionStr->erase(pos,1);
	}
	// 小文字化
	utils::tolower(sectionStr);
}



const std::regex &inp::phits::parameterPattern()
{
	// NOTE parameterの右辺はもっとバリエーションがあるので今後拡張する必要がある。
//	static std::regex paramPattern(R"(([0-9a-zA-Z-<>() ]+) *= *([-+*/0-9a-zA-Z(){}.]+.*))");
//	static std::regex paramPattern(R"(([0-9a-zA-Z-<>() ]+) *= *(.+)$)");
    static std::regex paramPattern(R"(([-0-9a-zA-Z<>():]+) *= *([-+\w.:/\\ )()]+[^=]*)(\s|$))");

	return paramPattern;
}

const std::regex &inp::phits::distributionPattern()
{
	static std::regex distPattern(R"(([aAeE])-type *)");
	return distPattern;
}




std::pair<std::string, std::string> inp::phits::getParameterPair(const std::string &str)
{
	std::smatch sm;
	if(std::regex_search(str, sm, parameterPattern())) {
		assert(sm.size() >= 3);
		std::string paramName = sm.str(1);
		std::string paramValue = sm.str(2);
		utils::trim(&paramName);
		utils::trim(&paramValue);
		// 入力ファイルはInputDataに読み込んだ時点で小文字化されているのでここでの変換は不要
//		utils::tolower(&paramName);
//		utils::tolower(&paramValue);
		return std::make_pair(paramName, paramValue);
	} else {
		throw std::invalid_argument(std::string("\"") + str + "\" is not a parameter setting string");
	}

}




std::pair<std::string, std::pair<std::size_t, std::size_t> > inp::phits::procInflCard(std::string &buff)
{
	std::smatch sm;

	// PHITSのinflカードは範囲指定がオプションなるに存在するので少しややこしい。、
	if (!std::regex_search(buff, sm, phits::getIncludeCardPattern())) {
		throw std::invalid_argument("Invalid infl: card, str=" + buff);
	}
	assert(sm.size() == 2);

	// infl文にはコメントアウト以外のメタカードは適用しない。
	// 前方に#が無いことだけpatternでチェック済みなので、追加のメタカード処理はしない。
	std::string includedFileName = sm.str(1);
	utils::trim(&includedFileName);
	includedFileName = utils::dequote('\"',includedFileName);

	// includeする行範囲を読み取る。
	std::pair<std::size_t, std::size_t> range{0, phits::MAX_LINE_NUMBER};
	if(std::regex_search(buff, sm, phits::getIncludeRangePattern())) {
		range = phits::getFileRange(sm.str());
	}

	return std::make_pair(includedFileName, range);

}
