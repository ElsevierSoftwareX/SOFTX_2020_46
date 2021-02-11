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
#include "modecard.hpp"


const std::regex &inp::mcnp::ModeCard::regex()
{
	static const std::regex reg(R"(^ {0,4}()(mode)() +([-+nqpef|!u<>vhglb_~xcywo@/*zk?%^dtsa# ]*) *$)", std::regex_constants::icase);
	return reg;
}

inp::mcnp::ModeCard::ModeCard(const std::string &str)
	: DataCard(str, inp::mcnp::ModeCard::regex())
{

}
