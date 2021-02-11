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
#ifndef QUA_HPP
#define QUA_HPP

#include "macro.hpp"

namespace geom {
namespace macro {


// MARS-CGのTORは１トーラス２平面で定義できるが、マルチピースになる。
// TZ + RPP で展開するか、 →　　マクロ展開をマクロがなくなるまで繰り返す必要が出る。
// OR演算子込で置換するか。　→　独自replace関数を定義する。
// のどちらかを採用する必要があるので後者を採用。
class Qua
{
public:
	static const char mnemonic[];// = "qua";
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

#endif // QUA_HPP
