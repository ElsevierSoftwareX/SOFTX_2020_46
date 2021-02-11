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
#ifndef MACRO_HPP
#define MACRO_HPP

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>
#include "core/math/nmatrix.hpp"
#include "core/math/nvector.hpp"
#include "core/geometry/surface/surface.hpp"

namespace inp {
class DataLine;
}


namespace geom{
namespace macro {

using expander_type = std::function<std::pair<std::string, std::string>(
										const std::unordered_map<size_t, math::Matrix<4>> &,
										std::list<inp::DataLine>::iterator &,
										std::list<inp::DataLine> *)>;
using replacer_type = std::function<void(const std::string &,
										const std::list<inp::DataLine>::iterator &)>;


// パラメータ長チェック
void checkParamLength(const std::vector<double> &params,
					  const std::vector<std::size_t> &validCounts, const std::string &macroName);


// equation中のmacroBodyNameをreplacingStringで置換する
std::string makeReplacedEquation(const std::string &macroBodyName,
                                  const std::string &replacingString,
                                  const std::string &equation);


// surfaceカード中のマクロボディを置換する。
void replaceSurfaceInput(const std::vector<std::shared_ptr<Surface>> &exSurfaces, // マクロ展開結果のサーフェイス群
						 const std::unique_ptr<math::Matrix<4> > &matrix, // 展開結果の面に適用するTR行列
                         std::list<inp::DataLine>::iterator &it,   // サーフェイスカードの中のマクロボディの行を指すiterator
                         std::list<inp::DataLine> *surfInputList  // サーフェイスカードリスト
						 );

// cellカード中のマクロボディを置換する。
std::string replacCelInput(const std::string &surfName, int numSurfaces, const std::string &cellInputString);



}  // end namespace geom
}  // end namespace macro
#endif // MACRO_HPP
