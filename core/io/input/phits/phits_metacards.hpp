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
#ifndef PHITS_METACARDS_HPP
#define PHITS_METACARDS_HPP

#include <limits>
#include <regex>
#include <string>
#include <utility>

#include <cstdlib>
#include <list>
#include <stdexcept>
#include "core/io/input/dataline.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"

#include "core/io/input/mcmode.hpp"

namespace inp {
namespace phits {

static constexpr std::size_t MAX_LINE_NUMBER = std::numeric_limits<std::size_t>::max()-1;

// コメントカード

const std::regex &getForceEndCard();
const std::regex &getforceEndSectionCard();
const std::regex &getSetConstantPattern();
const std::regex &getConstantPattern();
const std::regex &getIncludeCardPattern();
const std::regex &getIncludeRangePattern();
const std::regex &getSectionPattern();
const std::regex &getDisabledSectionPattern();
// 文字列 = 値 の正規表現
const std::regex &parameterPattern();
const std::regex &distributionPattern();


// inflカードのファイル名、範囲指定ペア を返す。
std::pair<std::string, std::pair<std::size_t, std::size_t>> procInflCard(std::string &buff);

/*
 *  [ n - m ] 形式の範囲指定のn, mペアを返す。
 *  [ n -] は(n, numelic_limits<int>::max())を
 *  [-m]   は(0, m)を返す
 */
const std::pair<std::size_t, std::size_t> getFileRange(const std::string& str);


// "文字列 = 値" から "文字列" "値"のペアを作成する。
std::pair<std::string, std::string> getParameterPair(const std::string& str);


// section名文字列のcanonicalな表現(空白なし、全て小文字)に変換
void canonicalizeSectionName(std::string *sectionStr);

/*
 *  itから パラメータ行(AAA = BBB 形式)を読み込んでBBBをT型かどうか判定して返す。
 * ・it->dataがパラメータ行ではない場合
 * ・expectedParameterNameがAAAと一致しない場合
 * ・BBBからT型への変換が失敗する場合
 * エラー終了する。it->がend()でないかはここではチェックしない。
 */
template <class T>
std::pair<std::string, T> GetParameterValue (const std::string &expectedParameterNamePattern,
					const std::list<inp::DataLine> &sourceInput,
					const std::list<inp::DataLine>::const_iterator &it)
{
	try {
		if(it == sourceInput.end()) {
			throw std::invalid_argument(std::string("Error: ") + it->pos() + " Unexpected end of source section.");
		}
		std::pair<std::string, std::string> paramNameAndValue;
		paramNameAndValue = inp::phits::getParameterPair(it->data);
//		if(paramNameAndValue.first != expectedParameterNamePattern)	{
		std::smatch sm;
		if(!std::regex_search(paramNameAndValue.first, sm, std::regex(expectedParameterNamePattern)))	{
			throw std::invalid_argument(std::string(it->pos())
										+ " Parameter \"" + expectedParameterNamePattern +"\" is expected (but not found) from input \""
										+ it->data + "\"");
		}
		return std::make_pair(sm.str(0), utils::stringTo<T>(paramNameAndValue.second));
	} catch (std::invalid_argument &e) {
        throw std::invalid_argument(it->pos() + " " +  e.what());
	}
	std::exit(EXIT_FAILURE);
}




}  // end namespace phits
}  // end namespace inp

#endif // PHITS_METACARDS_HPP
