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
#include "mcmode.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>


#include "common/commoncards.hpp"
#include "phits/phits_metacards.hpp"
#include "core/utils/system_utils.hpp"

namespace {
const std::map<inp::phits::Section, std::string> &sectionNameMap()
{
	// ここでのセクション名は小文字化、空白除去が適用されている
	static const std::map<inp::phits::Section, std::string> sectionNameMap
	{
		{inp::phits::Section::NOTSET, ""},
		{inp::phits::Section::TITLE, "title"},
		{inp::phits::Section::PARAMETERS, "parameters"},
		{inp::phits::Section::CELL, "cell"},
		{inp::phits::Section::SURFACE, "surface"},
		{inp::phits::Section::TRANSFORM, "transform"},
		{inp::phits::Section::SOURCE, "source"},
		{inp::phits::Section::MATERIAL, "material"},
		{inp::phits::Section::T_POINT, "t-point"},
		{inp::phits::Section::MATNAMECOLOR, "matnamecolor"},
		{inp::phits::Section::UNCATEGORIZED, "uncategorized"}
	};
	return sectionNameMap;
}


const std::map<McMode, std::string> modeStrMap()
{
	static std::map<McMode, std::string> modeStringMap
	{
		{McMode::AUTO, std::string("auto")},
		{McMode::MCNP, std::string("mcnp")},
		{McMode::PHITS, std::string("phits")},
		{McMode::QAD, std::string("qad")},
		{McMode::MARS, std::string("mars")},
	};
	return modeStringMap;
}

const std::map<std::string, McMode> createModeStrMap()
{
	auto modeToStringMap = modeStrMap();
	std::map<std::string, McMode> strModeMap;
	for(auto it = modeToStringMap.cbegin(); it != modeToStringMap.cend(); ++it) {
		strModeMap.emplace(it->second, it->first);
	}
	return strModeMap;
}

const std::map<std::string, McMode> & strModeMap()
{
	static std::map<std::string, McMode> strModeMap = createModeStrMap();
	return strModeMap;
}

}  // end anonymous namespace




std::string inp::getModeString(const McMode &mode) noexcept
{
	return modeStrMap().at(mode);
}



McMode inp::stringToMcMode(std::string modeStr)
{
	std::transform(modeStr.begin(), modeStr.end(), modeStr.begin(), ::tolower);
	auto smMap = strModeMap();
	auto it = smMap.find(modeStr);
	return (it != smMap.end()) ? it->second : McMode::AUTO;
}


McMode inp::guessMcModeFromFile(const std::string &filename)
{
	// 空白、コメント行を除いた最初のデータ行がセクション([section])かfile=, infl: set:ならphits
	std::ifstream ifs(utils::utf8ToSystemEncoding(filename).c_str());
	if(ifs.fail()) {
        throw std::invalid_argument(std::string("Error: No such a file ") + filename);
	}
	std::string buff;
	std::smatch sm;
	while(buff.empty() || std::regex_search(buff, sm, comm::getPreCommentPattern())) {
		getline(ifs, buff);
	}
	/*
	 *  要するにコメント以外で最初に出てきた文字列が"file="かphitsセクションならphits、
	 * それ以外ならmcnpと判定する。
	 */
	static std::regex fileReg(R"(^ *file *=)", std::regex_constants::icase);
	if(std::regex_search(buff, phits::getSectionPattern()) || std::regex_search(buff, phits::getIncludeCardPattern())
	|| std::regex_search(buff, phits::getSetConstantPattern()) || std::regex_search(buff, fileReg)) {
		return McMode::PHITS;
	} else {
		return McMode::MCNP;
	}
}

McMode inp::guessMcModeFromSuffix(const std::string &filename)
{
	auto dotpos = filename.rfind(".");
	std::string suffix = (dotpos == std::string::npos) ? filename : filename.substr(dotpos+1);

	if(suffix == "mcn" || suffix == "MCN") {
		return McMode::MCNP;
	} else if(suffix == "qad" || suffix == "QAD") {
		return McMode::QAD;
	} else if(suffix == "mars" || suffix == "MARS") {
		return McMode::MARS;
    } else if(suffix == "phi" || suffix == "PHI") {
        return McMode::PHITS;
    } else if (suffix == "inp" || suffix == "INP") {
#ifdef INP_AS_PHITS
        return McMode::PHITS;
#elif INP_AS_MCNP
        return McMode::MCNP;
#else
        return McMode::AUTO;
#endif
	} else {
		return McMode::AUTO;
	}
}


// PhitsSection列挙子から対応する文字列を返す。
std::string inp::phits::toSectionString(const inp::phits::Section &sect)
{
	return sectionNameMap().at(sect);
}

inp::phits::Section inp::phits::toEnumSection(const std::string &str)
{
	for(const auto &p: sectionNameMap()) {
		if(str == p.second) return p.first;
	}
	// 未対応のセクションタイトルはUNCATEGORIZEDとする。
	return inp::phits::Section::UNCATEGORIZED;
	//throw std::out_of_range(std::string("str=") + str + " is not a section title.");
}


