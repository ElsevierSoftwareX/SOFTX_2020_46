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
#include "commoncards.hpp"

#include <algorithm>
#include <cassert>
#include <regex>

#include "core/utils/message.hpp"

const std::regex &inp::comm::getPreContinuePattern()
{
	static const std::regex preContPattern = std::regex(R"(^ {5})");
	return preContPattern;
}

const std::regex &inp::comm::getMcnpPostContinuePattern()
{
	static const std::regex postContPattern = std::regex(R"(& *$)");
	return postContPattern;
}
const std::regex &inp::comm::getPhitsPostContinuePattern()
{
	static const std::regex postContPattern = std::regex(R"(\\ *$)");
	return postContPattern;
}

#include <iostream>
const std::regex &inp::comm::getPreCommentPattern()
{
//	static const std::regex preCommentPattern = std::regex(R"(^ {0,4}[cC] *(?![0-9a-zA-Z]))");
//	static const std::regex preCommentPattern = std::regex(R"(^ {0,4}[cC][ $][^*])");
//	static const std::regex preCommentPattern = std::regex(R"(^ {0,4}[cC]( *$| [^*]))");  // c *はコメントにしない版
	// とりあえず拡張入力な話は一時的に忘れる。
	static const std::regex preCommentPattern = std::regex(R"(^ {0,4}[cC]( +|$))");
	return preCommentPattern;
}

const std::regex &inp::comm::getPostCommentPattern()
{
	static const std::regex postCommentPattern = std::regex(R"(\$)");
	return postCommentPattern;
}

std::string inp::comm::preUncommentedString(const std::string &str)
{
    static const std::regex preCommentPattern = std::regex(R"(^( {0,4}[cC] )(.*))");
    std::smatch sm;
    if(std::regex_search(str, sm, preCommentPattern)) {
        return sm.str(2);
    } else {
        return str;
    }
}


const std::regex &inp::comm::iPattern()
{
//	static const std::regex I_PAT(R"(^[0-9]*[iI]$)");
//	return I_PAT;
    static const std::regex ipat(R"(^ *([0-9]*)i(log)*$)", std::regex_constants::icase);
	return ipat;
}
const std::regex &inp::comm::jPattern()
{
	static const std::regex J_PAT(R"(^[0-9]*[jJ]$)");
	return J_PAT;
}
const std::regex &inp::comm::mPattern()
{
	static const std::regex M_PAT(R"(^[0-9][0-9.eE+-]*[mM]$)");
	return M_PAT;
}
const std::regex &inp::comm::rPattern()
{
	static const std::regex R_PAT(R"(^[0-9]*[rR]$)");
	return R_PAT;
}

bool inp::comm::removeMcnpPostComment(std::string *str)
{
	// MCNPの後置コメント $以降のコメント部分を削除する。
	auto pos = str->find_first_of("$");
	if(pos != std::string::npos) {
		*str = str->substr(0, pos);
		return true;
	}
	return false;
}

// ＃以外のphits後置コメント($!%)を処理する。
bool inp::comm::removePhitsPostCommentNotSharp(std::string *str)
{
	auto pos = str->find_first_of("!$%");
	if(pos != std::string::npos) {
		*str = str->substr(0, pos);
		return true;
	}
	return false;
}

bool inp::comm::removePhitsPostCommentSharp(std::string *str)
{
	auto pos = str->find_first_of("#");
	if(pos != std::string::npos) {
		*str = str->substr(0, pos);
		return true;
	}
	return false;
}





std::string inp::comm::GetMaterialIdStr(const std::string &str)
{
	// M1 とか MAT[1] など。
    std::smatch sm1, sm2;
    std::regex pattern1(R"(^ {0,4}[mM][aA][tT]\[ *(\S+) *\])");  // NMTC style
    std::regex pattern2(R"(^ {0,4}[mM](\S+))");  // MCNP Style
    // マテリアル番号に文字列を使えるようにすることを想定しているため
    // MAT[n]はもれなくMnに該当して AT[n]がマテリアル番号と見なされてしまう。従って先ににMAT[n]から検索する
    if(std::regex_search(str, sm1, pattern1)) {  // for NMTC style
		assert(sm1.size() >= 2);
		return sm1.str(1);
    } else if (std::regex_search(str, sm2, pattern2)) {  // for MCNP style
		assert(sm2.size() >= 2);
        return sm2.str(1);
    } else {
        return std::string();
    }
}

