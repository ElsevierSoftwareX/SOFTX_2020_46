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
#ifndef META_HPP
#define META_HPP

#include <regex>
#include <string>
#include <utility>

namespace inp {
namespace mcnp {

const std::regex &getReadCardPattern();

const std::regex &getFilePattern();
const std::regex &getNoechoPattern();

// READファイルの引数文字列を渡して READするファイルとechoフラグのペアを返す
std::pair<std::string, bool> procReadCard(std::string readArgStr);

}  // end namespace mcnp
}  // end namespace inp
#endif // META_HPP
