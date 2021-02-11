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
#ifndef FILLING_UTILS_HPP
#define FILLING_UTILS_HPP


#include <array>
#include <vector>

#include "core/math/nvector.hpp"

namespace geom{
class BoundingBox;
}





/*
 * 次元数
 * 基本格子要素[000]の中心
 * 格子インデックスの方向ベクトルのセット
 * 外側セルのBoundingBox
 *
 * を元に要素インデックス番号の最大最小(dimension declarator)ペアを計算する。
 * 次元数が2の場合返り値の第三要素はpair(0,0)である。
 *
 * BoundingBoxが無限大の場合、無限大の広がりを持つ方向と
 * 格子インデックスベクトルが直行している場合は有限の結果が返るが、
 * それ以外の場合はruntime_erorrが発生する。
 *
 * BBが無限大の広がりを持つ方向の数が"3-次元数"より多い場合は
 * 要素数が無限になることが確定的なのでこの場合もruntime_errorが発生する。
 */
std::array<std::pair<int, int>, 3>
	calcDimensionDeclarator(int dimension,
							const math::Point &baseUnitCenter,
							const std::vector<math::Vector<3>> &indexVectors,
							const geom::BoundingBox &outerCellBB);


#endif // FILLING_UTILS_HPP
