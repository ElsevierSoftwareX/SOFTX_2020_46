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
#ifndef LOGICALFUNC_HPP
#define LOGICALFUNC_HPP

#include <string>

namespace lg{


// equationの補集合の文字列を返す。
std::string complimentedString(const std::string &equation);



} // end namespace lg
#endif // LOGICALFUNC_HPP
