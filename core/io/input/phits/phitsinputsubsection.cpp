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
#include "phitsinputsubsection.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include "core/io/input/dataline.hpp"
#include "core/utils/utils.hpp"

namespace {
const char NOT_USE[] = "non";
}

inp::phits::PhitsInputSubsection::PhitsInputSubsection() {;}

size_t inp::phits::PhitsInputSubsection::numberOfParameters() const
{
	if(parameterVectorsMap_.empty()) {
		return 0;
	} else {
		return parameterVectorsMap_.begin()->second.size();
	}
}



std::vector<std::string> inp::phits::PhitsInputSubsection::getValueVector(const std::string &title) const
{
	return parameterVectorsMap_.at(title);
}

bool inp::phits::PhitsInputSubsection::hasTitle(const std::string &str) const
{
	for(auto &title: indexToTitle_) {
		if(str == title) return true;
	}
	return false;
}

std::string inp::phits::PhitsInputSubsection::toString() const
{
	std::stringstream ss;
	size_t maxWidth = 0;
	for(auto &elem: indexToTitle_) {
		if(maxWidth <  elem.size()) maxWidth = elem.size();
	}
	for(auto &elem: indexToTitle_) {
		if(elem != NOT_USE)
			ss << std::setw(maxWidth) << elem << " ";
	}
	ss << std::endl;

//	mDebug() << indexToTitle_;
	for(size_t j = 0; j < parameterVectorsMap_.begin()->second.size(); ++j) {
		for(size_t i = 0 ; i < indexToTitle_.size(); ++i) {
			auto indexString = indexToTitle_.at(i);
			if(indexString != NOT_USE) {
				ss << std::setw(maxWidth) << parameterVectorsMap_.at(indexString).at(j) << " ";
			}
		}
		ss << std::endl;
	}
	return ss.str();
}


inp::phits::PhitsInputSubsection::PhitsInputSubsection(std::list<inp::DataLine>::const_iterator &it,
														 std::list<inp::DataLine>::const_iterator endIt, size_t n)
{
	if(n == 0) return;
	// title行を読み取ってparameterVecMapの空データと出現順序-文字列対応vectorを作成。
	if(it == endIt) throw std::invalid_argument("Unexpected eof, while reading subsection");
	for(auto &elem: utils::splitString(" ", it->data, true)) {
//		utils::tolower(&elem);
		indexToTitle_.emplace_back(elem);
		if(elem != NOT_USE) {
			parameterVectorsMap_[elem] = std::vector<std::string>();
		}
	}


	size_t num_read = 0;
	while(num_read != n) {
		++it;
		if(it == endIt) throw std::invalid_argument("Unexpected eof, while reading subsection");
		auto paramstr = it->data;
		auto parameters = utils::splitString(" ", paramstr, true);
		if(parameters.size() != indexToTitle_.size()) {
			throw std::invalid_argument("Number of title data(=" + utils::toString(indexToTitle_.size())
										+ ") does not equal to that of parameter(=" + utils::toString(parameters.size()) + ")\n"
										+ "title = \"" + utils::concat(indexToTitle_, " ")
										+ "\", parameter = \"" + paramstr + "\"");
		}
		for(size_t i = 0; i < parameters.size(); ++i) {
			std::string title = indexToTitle_.at(i);
			if(title != NOT_USE) {
				parameterVectorsMap_.at(title).emplace_back(parameters.at(i));
			}

		}
		++num_read;
	}
	// 読み取り終了なのでitはひとつ進める
	++it;

	// データ整合性チェック
	const size_t sz = parameterVectorsMap_.begin()->second.size();
	for(auto beg = parameterVectorsMap_.begin(); beg != parameterVectorsMap_.end(); ++beg){
		if(beg->second.size() != sz) {
			throw std::invalid_argument(std::string("Subsection parameter length titled\""
													+ beg->first + "\" should be " + utils::toString(sz)));
		}
	}
}
