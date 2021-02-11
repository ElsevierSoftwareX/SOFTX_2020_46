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
#ifndef MODECARD_HPP
#define MODECARD_HPP

#include "core/io/input/common/datacard.hpp"
#include <regex>
#include <string>

namespace inp {
namespace mcnp {

class ModeCard : public inp::comm::DataCard
{
public:
	static const std::regex &regex();
	ModeCard(const std::string &str);
};

}  // end namespace mcnp
}  // end namespace inp
#endif // MODECARD_HPP
