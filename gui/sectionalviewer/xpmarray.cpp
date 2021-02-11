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
#include "xpmarray.hpp"

#include <algorithm>
#include <vector>
#include "../../core/utils/container_utils.hpp"
#include "../../core/utils/string_utils.hpp"
#include "../../core/utils/message.hpp"

XpmArray::XpmArray(std::string xpmStr)
{
	// まず改行は削除する。
	auto it = remove_if(xpmStr.begin(), xpmStr.end(), [](const char& c) {return c=='\n';});
	xpmStr = std::string(xpmStr.begin(), it);

	std::vector<std::string> strVec = utils::splitString(",", xpmStr, false);
	for(auto &str: strVec) {
		str = utils::dequote('"', str, false);
	}
	numLines_ = strVec.size();
	xpm_ = new char*[numLines_];
	for(size_t i = 0; i < numLines_; ++i) {
		*(xpm_+i) = new char[strVec.at(i).size()+1];
		std::strcpy(*(xpm_+i), strVec.at(i).c_str());
	}
}

XpmArray::~XpmArray() {
	if(xpm_ != nullptr) {
		for(size_t i = 0; i < numLines_; ++i) {
			delete [] xpm_[i];
		}
	}
}
