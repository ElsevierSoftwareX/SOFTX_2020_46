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
#ifndef MCMODE_HPP
#define MCMODE_HPP

#include <string>

enum class McMode: int {AUTO, PHITS, MCNP, QAD, MARS};

// POD型だから非局所staticでも初期化は確実。
static constexpr McMode McModeList[] = {McMode::AUTO, McMode::PHITS, McMode::MCNP, McMode::QAD, McMode::MARS};



namespace inp{

std::string getModeString(const McMode &mode) noexcept;
McMode stringToMcMode(std::string modeStr);
// ファイルの中身からMcModeを判定
McMode guessMcModeFromFile(const std::string &filename);
// ファイルの拡張子からMcModeを判定
McMode guessMcModeFromSuffix(const std::string &filename);

namespace phits{

enum class Section: int{NOTSET, TITLE, PARAMETERS, CELL, SURFACE, TRANSFORM, MATERIAL, SOURCE, T_POINT, MATNAMECOLOR, UNCATEGORIZED};
// PhitsSection列挙子を与えて対応する文字列を返す。
// PhitsSection::CELLを引数に与えると"CELL"を返す
std::string toSectionString(const Section &sect);
Section toEnumSection(const std::string &str);


}  // end namespace phits
}  // end namespace inp
#endif // MCMODE_HPP
