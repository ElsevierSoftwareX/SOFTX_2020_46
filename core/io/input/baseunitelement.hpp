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
#ifndef BASEUNITELEMENT_HPP
#define BASEUNITELEMENT_HPP

#include <memory>
#include <vector>

#include "core/math/nvector.hpp"

namespace geom {
//class CellCreator;
class SurfaceMap;
class BoundingBox;
class Plane;
}

class BaseUnitElement{
public:

	static std::unique_ptr<BaseUnitElement> createBaseUnitElement(int latvalue,
																  const std::vector<std::string> &surfaceNames,
																  const geom::SurfaceMap &smap);

	BaseUnitElement(int dimension,
					const math::Point &center,
					const std::vector<math::Vector<3>> &indexVecs,
					const std::vector<std::pair<geom::Plane, geom::Plane> > &planePairs);

	// 基本単位要素の中心
	const math::Point &center() const {return center_;}
	// 格子の次元
	int dimension() const {return dimension_;}
	// 基本単位要素を構成する面のペアfirstがindexの正側、secondが負側
	const std::vector<std::pair<geom::Plane, geom::Plane>> &planePairs() const {return planePairs_;}
	// indexが増加する方向のベクトル
	const std::vector<math::Vector<3>> &indexVectors() const {return indexVectors_;}
	// 基本単位要素のBoundingBox
	geom::BoundingBox unitBB() const;


private:
	int dimension_;
	math::Point center_;
	std::vector<math::Vector<3>> indexVectors_;
	// 要素を構成する面
	std::vector<std::pair<geom::Plane, geom::Plane>> planePairs_;


	// 基本単位要素を構成する角の点(BB計算用だからpublicには必要ない)
	std::vector<math::Point> cornerPoints() const;

};

#endif // BASEUNITELEMENT_HPP
