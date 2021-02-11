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
#ifndef ORIGINAL_METACARD_H
#define ORIGINAL_METACARD_H

#include <regex>

namespace inp {
namespace org {

/*
 *  オリジナルのメタ入力パターンを返す。
 *
 */

const std::regex &getOriginalCommandPattern();


}  // end namespace org
}  // end namespace inp
#endif // ORIGINAL_METACARD_H
