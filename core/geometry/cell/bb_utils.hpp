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
#ifndef BB_UTILS_HPP
#define BB_UTILS_HPP


#include <atomic>
#include "core/formula/logical/lpolynomial.hpp"


#ifdef ENABLE_GUI
#include <vtkSmartPointer.h>
#include <vtkImplicitFunction.h>
#endif

namespace geom {
class BoundingBox;
class Plane;
class SurfaceMap;

namespace bb {

// 論理多項式ツリーを辿ってBoundingBox作成用planeを取得する
std::vector<std::vector<geom::Plane>> boundingSurfaces(const lg::LogicalExpression<int> &poly,
                                                       const std::atomic_bool *timeoutFlag,
                                                       const geom::SurfaceMap &smap);
// 多項式を辿って式のBBを作成する。
geom::BoundingBox createBoundingBox(const lg::LogicalExpression<int> &poly, const geom::SurfaceMap &smap);
// BB計算方法2 面のAND部分だけでとりあえずBBを作成し、あとはそれらBBのOR演算でBBを作成する。
// 第四引数がfalseならセルのマルチピースを生むような部分は無視して計算する。
geom::BoundingBox createBoundingBox2(const lg::LogicalExpression<int> &poly, const geom::SurfaceMap &smap,
                                     std::atomic_bool *stopFlag, bool acceptMultiPiece);
#ifdef ENABLE_GUI
    vtkSmartPointer<vtkImplicitFunction> createImplicitFunction(const lg::LogicalExpression<int> &poly, const geom::SurfaceMap &smap);
#endif

}  // end namespace bb
}  // end namespace geom

#endif // BB_UTILS_HPP
