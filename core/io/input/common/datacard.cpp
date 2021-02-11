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
#include "datacard.hpp"

#include <cassert>
#include <sstream>
#include <stdexcept>

#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"

inp::comm::DataCard::DataCard(const std::string &str, const std::regex &reg)
	: regex_(reg)
{
	assert(regex_.mark_count() == 4);
    assert((regex_.flags() && std::regex_constants::icase) == true);

	std::smatch sm;
	// regex_ は4つのサブマッチを持つ必要がある。
	if(std::regex_search(str, sm, regex_)) {
//		for(size_t i = 0; i < sm.size(); ++i) {
//			mDebug() << "i=" << i << sm.str(i);
//		}
		modifier_ = sm.str(1);
		name_ = sm.str(2);
		id_ = sm.str(3);
		argument_ = sm.str(4);
		utils::trim(&argument_);
	} else {
		throw std::invalid_argument(str + " is not a valid data card string.");
	}
}

std::string inp::comm::DataCard::toString() const
{
	std::stringstream ss;
	ss << "Card name = " << modifier_ << name_ << ", id = " << id_ << ", argument = " << argument_;
	return ss.str();
}
