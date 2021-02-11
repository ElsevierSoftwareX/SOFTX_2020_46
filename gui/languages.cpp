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
#include "languages.hpp"

#include <iostream>
#include <unordered_map>


const char BASENAME[] = "gui_";
const char DEFAULT_LANGUAGE[] = "English";

std::string fileToLangName(std::string fileName)
{
//	const static std::unordered_map<std::string, std::string> langTable
//	{
//		{"en", "English"},
//		{"ja", "Japanese"}
//	};

	auto seppos = fileName.rfind("/");
	if(seppos != std::string::npos) fileName = fileName.substr(seppos);

	auto pos = fileName.find("gui_");
	auto dotpos = fileName.rfind(".");
	if(pos == std::string::npos) {
		return std::string(DEFAULT_LANGUAGE);
	} else {
		return fileName.substr(pos+sizeof(BASENAME)-1, dotpos - (pos+sizeof(BASENAME)-1));
//		try {
//			return langTable.at(fileName.substr(pos+sizeof(BASENAME)-1, dotpos - (pos+sizeof(BASENAME)-1)));
//		} catch (...) {
//			return std::string(DEFAULT_LANGUAGE);
//		}
	}
}
