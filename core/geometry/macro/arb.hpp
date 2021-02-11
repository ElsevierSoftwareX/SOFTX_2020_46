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
#ifndef ARB_HPP
#define ARB_HPP

#include "macro.hpp"

namespace inp{
class DataLine;
}

namespace geom {
namespace macro{

class Arb {
public:
	static const char mnemonic[];// = "arb";
    static constexpr int numSurfaces = 6;  // max 6 planes　min 4 planes
	static std::pair<std::string, std::string>
				expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
						std::list<inp::DataLine>::iterator &it,
						std::list<inp::DataLine> *surfInputList);

	static void replace (const std::string &surfName,
						 const std::list<inp::DataLine>::iterator &it);
};

}  // end namespace macro
}  // end namespace geom

#endif // ARB_HPP
