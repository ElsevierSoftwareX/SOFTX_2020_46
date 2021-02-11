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
#include "cardcommon.hpp"
#include <regex>

#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"

namespace {
std::regex surfaceNameRegex("^" + inp::surfaceNameRegexStr() + "$");
//std::regex acceptableUserInputNameRegex("^" + inp::acceptableUserInputNameRegexStr() + "$");
}

// 第一引数がTRCLを行っているセル名、 第二引数がTR対象surface名
// TRしているセルが階層セルの場合一番深い部分のセル名のみを使う
std::string inp::getTransformedSurfaceName(const std::string &tredCellName, const std::string &oldSurfaceName) {
	std::string signStr{oldSurfaceName.front()};
	std::string::size_type spos = 1;
	if(signStr != "+" && signStr != "-") 	{
		signStr.clear();
		spos = 0;
	}
	/*
	 * fillによる外側セルのTRを適用する場合、univ内の同一セルに別々の名前がつかないように
	 * TR後の名前は surface名_t"lattice外側セル名"にする。
	 */
	// AA<BB<CC の AAと BB<CCを分けて後者をTR後のセル名に適用する。
	//	static std::regex latticePattern(R"(([-+ ._0-9a-zA-Z]+)(<\w\[\w+ \w+ \w+\].*))");
	static std::regex outerCellPattern(R"(([-+ ._0-9a-zA-Z\[\]]+)(<.*))");std::smatch sm;
	std::string retStr;
	if(std::regex_search(tredCellName, sm, outerCellPattern)) {
		//		for(size_t i = 0; i < sm.size(); ++i) {
		//			mDebug() << "i=" << i << sm.str(i);
		//		}
		retStr = signStr + oldSurfaceName.substr(spos) + "_t" + sm.str(2).substr(1);
//		retStr = signStr + "_" + sm.str(2) + "_" + oldSurfaceName.substr(spos);
	} else {
		retStr = signStr + oldSurfaceName.substr(spos) + "_t" +tredCellName;
//		retStr = signStr + "_" + tredCellName + "_" + oldSurfaceName.substr(spos);
	}
//	mDebug() << "TR前の名前=" << oldSurfaceName << ", TRセル名=" << tredCellName << " TRCL後の新しい面名=" << retStr;
	return retStr;
}


const std::string &inp::cellNameRegexStr()
{
    static const std::string cellRegexStr = "([-+.,_@<\\[\\]\\w]+)";
	return cellRegexStr;
}

const std::string &inp::surfaceNameRegexStr()
{
	// TRした場合surface名にまるまるcell名が含まれるのでセル名使用可能文字に加えて
	// *(反射), +(ホワイト) がsurface名で使用可能な文字になる。
	//
	static const std::string surfRegexStr = "([-+*]*)" + cellNameRegexStr();
	return surfRegexStr;
}



const std::string &inp::acceptableUserInputNameRegexStr()
{
	static const std::string surfRegexStr = "[\\w][\\w_]*";
    return surfRegexStr;

}


std::string inp::indexToElementName(const std::string &baseName, int i, int j, int k)
{
	return baseName + "[" + std::to_string(i) + "," + std::to_string(j) + "," + std::to_string(k) + "]";
}

void inp::checkNameCharacters(const std::string &name, bool asUserInput)
{
	static const std::regex acceptableUserInputNameRegex("^ *" + inp::acceptableUserInputNameRegexStr() + " *$");
//	static const std::regex acceptableUserInputNameRegex(R"(^ *[\w][\w_]* *$)");
	if(asUserInput) {
        if(name.empty()) {
            throw std::invalid_argument("user-input name is empty");
        } else if(!std::regex_match(name, acceptableUserInputNameRegex)) {
			throw std::invalid_argument("user-input characters should be [0-9a-zA-Z_], input name=" + name);
        } else if(name.front() == '_') {
            throw std::invalid_argument("user-input characters should not start with \"_\", input=" + name);
        }
    } else {
        if(!std::regex_match(name, surfaceNameRegex)) {
            throw std::invalid_argument("name contains invalid char(s), input=" + name);
        }
    }
}


std::regex trPattern(R"((\*?)([tT][rR][sS][fF]|[tT][rR][cC][lL]) *= *\(([-+"(){}*/%.,0-9a-zA-Z ]+)\))"); // {}はfortran式が使われる可能性があるため

bool inp::appendCanonicalTrStr(const std::string &srcStr, std::string *trStr)
{
	std::smatch trMatch;
	if(std::regex_search(srcStr, trMatch, trPattern)) {
		//mDebug() << "TR detected!! name=" << trMatch.str(2) << "TRarg=" << trMatch.str(3);
		bool isDegree = !trMatch.str(1).empty();
		auto trArgVec = utils::splitString(std::make_pair('{', '}'), ",", trMatch.str(3), true);
		for(auto trArg: trArgVec) {
			trArg = utils::dequote(std::make_pair('(', ')'), trArg);
			if(isDegree && trArg.front() != '*') trArg = "*" + trArg;
			*trStr = (trStr->empty()) ? trArg : *trStr + "," + trArg;
		}
		return true;
	} else {
		return false;
	}
}


