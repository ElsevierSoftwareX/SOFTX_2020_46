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
#ifndef QUADRICBS_HPP
#define QUADRICBS_HPP

#include <memory>
#include <vector>

#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"

namespace geom{
class Plane;
class BoundingBox;
}


// 二次曲面のbounding surfaceを求める関数。
// 行列のランクごとに分けている。

void CalcBoundingPlaneVectors_rank3(const std::string &name_,  //
                                    bool isInside,
                                    int rankMatrix,
                                    const math::Vector<3> &translation,
                                    const math::Matrix<3> &rotation,
                                    std::vector<double> eigenValues,
                                    double d,
                                    std::vector<std::vector<geom::Plane>> &boundingPlaneVecs,
                                    geom::BoundingBox & bb);

void CalcBoundingPlaneVectors_rank2(const std::string &name_,  //
                                    bool isInside,
                                    int rankMatrix,
                                    const math::Vector<3> &pqr,
                                    const math::Matrix<3> &rotation,
                                    std::vector<double> eigenValues,
                                    double d,
                                    std::vector<std::vector<geom::Plane>> &boundingPlaneVecs);

void CalcBoundingPlaneVectors_rank1(const std::string &name_,  //
                                    bool isInside,
                                    int rankMatrix,
                                    math::Vector<3> pqr,
                                    math::Matrix<3> rotation,
                                    std::vector<double> eigenValues,
                                    double d,
                                    std::vector<std::vector<geom::Plane>> &boundingPlaneVecs);

#endif // QUADRICBS_HPP
