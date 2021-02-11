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
#ifndef FILLDATA_HPP
#define FILLDATA_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "baseunitelement.hpp"
#include "core/math/nvector.hpp"
#include "core/geometry/surface/plane.hpp"

namespace geom {
class CellCreator;
//class SurfaceMap;
}


namespace inp {

struct CellCard;




// FILLパラメータのデータを保持するクラス。 主にfill=...の文字列から生成する。
class FillingData {
public:
	// fillデータをfill=以降の部分から生成する。
	static FillingData fromString(const std::string &fillParamStr);

	// 初期化ルーチン。要素データを計算する。これを実行しないとcenterが正しい値にならない
	void init(const geom::CellCreator *creator,
							 const std::unordered_map<std::string, CellCard> *solvedCards,
							 const inp::CellCard &latticeCard,
							 const std::vector<std::string> &surfaceNames,
							 int latvalue,
							 bool *isSelfFilled);

	bool isDegree() const {return isDegree_;}
	std::size_t readCount() const {return hasReadCount_;}

	std::pair<int, int> irange() const {return irange_;}
	std::pair<int, int> jrange() const {return jrange_;}
	std::pair<int, int> krange() const {return krange_;}

	const std::vector<std::string> &universes() {return universes_;}

	int isize() const {return irange_.second - irange_.first + 1;}
	int jsize() const {return jrange_.second - jrange_.first + 1;}
	int ksize() const {return krange_.second - krange_.first + 1;}
	int size(std::size_t index) const;
	const std::pair<int, int> &range(std::size_t index) const;

	// latticeの次元 1, 2 or 3
	int dimension() const {return baseUnitElement_->dimension();}
	// 要素セルを定義するのに必要な面のペア数 3次元矩形格子なら3, 2次元六角格子なら3, 3次元六角格子なら4
	int numIndex() const {return baseUnitElement_->planePairs().size();}
	std::shared_ptr<BaseUnitElement> baseUnitElement() const {return baseUnitElement_;}

	std::string toInputString() const;


private:
	bool isDegree_;            // *FILLL=で角度をdegree単位に指定の時true
	bool hasDimDeclarator_;    // Dimension Declaratorが明示的に与えられている時はtrue
	std::size_t hasReadCount_; // このデータ作成時に何文字読み込んだかを保持しておく。
	std::pair<int, int> irange_, jrange_, krange_;  // DimDeclaratorの下限、上限それぞれ3方向.[xyz]rangeとしているけど本当は[ijk]rangeが正しい。
	std::vector<std::string> universes_;

	std::shared_ptr<BaseUnitElement> baseUnitElement_;
};





}  // end namespace geom
#endif // FILLDATA_HPP
