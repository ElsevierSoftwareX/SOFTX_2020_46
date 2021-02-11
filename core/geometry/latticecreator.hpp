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
#ifndef LATTICECREATOR_HPP
#define LATTICECREATOR_HPP

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "core/io/input/cellcard.hpp"
#include "core/io/input/filldata.hpp"
#include "cell/boundingbox.hpp"


namespace geom{

class Plane;
class SurfaceMap;
class CellCreator;


class LatticeCreator
{
public:
	LatticeCreator(const CellCreator *creator, int latvalue, const inp::CellCard &latticeCard,
				   const std::unordered_map<std::string, inp::CellCard> *solvedCards);
	bool isSelfFilled() const {return isSelfFilled_;}

	std::vector<inp::CellCard> createElementCards(const std::string &selfUnivName,
												  const std::unordered_map<size_t, math::Matrix<4>> &trMap,
												  SurfaceMap *smap);


	std::vector<std::string> getLatticeSurfaceNames(int i, int j, int k) const;
	std::string getLatticeBBString(int i, int j, int k, const BoundingBox &unitBB) const;


private:
	inp::CellCard latticeCard_;
	inp::FillingData fillingData_;
	bool isSelfFilled_;

	int latticeArg_;  // LAT=X のX

	// 内部ではvector<map<int, string>>でsurface名を保持する。
	// latticeSurfaces_.at(direction).at(index) でdirectionとindexに応じたsurface名を保存する。
	std::vector<std::map<int, std::string>> latticeSurfaceNameMap_;
	// 六角格子用のマップ
	// hexSurfaceNameMap[s][i][j][1]が格子要素[i,j]のs方向面のプラス側面を表す。
	// hexSurfaceNameMap[s][i][j][0]が格子要素[i,j]のs方向面のマイナス側面を表す。
	std::vector<std::map<int, std::map<int, std::map<int, std::string>>>> hexSurfaceNameMap_;

	std::pair<int, int> iPlaneIndexPair(int i, int j, int k) const;
	std::pair<int, int> jPlaneIndexPair(int i, int j, int k) const;
	std::pair<int, int> kPlaneIndexPair(int i, int j, int k) const;

	void generatePlanes(const std::pair<int, int> &irange,
						const std::pair<int, int> &jrange,
						const std::pair<int, int> &krange,
						SurfaceMap *smap);

};



}  // end namespace geom
#endif // LATTICECREATOR_HPP
