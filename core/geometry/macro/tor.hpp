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
#ifndef TOR_HPP
#define TOR_HPP


#include "macro.hpp"

namespace geom {
namespace macro {


// MARS-CGのTORは１トーラス２平面で定義できるが、マルチピースになる。
// 論理式を単純化するため、常に1トーラス3平面の2ピース構成とする。
// 通常のMCNPマクロボディと異なり全てマイナス符号、や全てプラス符号ではないので
// replace関数内で独自の論理式文字列を作成する。
class Tor
{
public:
    static const char mnemonic[];// = "tor";
    static constexpr int numSurfaces = 3;
    static std::pair<std::string, std::string>
                expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
                        std::list<inp::DataLine>::iterator &it,
                        std::list<inp::DataLine> *surfInputList);
//ここから。
    static void replace (const std::string &macroBodyName,
                         const std::list<inp::DataLine>::iterator &it);
};


}
}

#endif // TOR_HPP
