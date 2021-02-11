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
#ifndef MATRIX_UTILS_HPP
#define MATRIX_UTILS_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include "core/math/nmatrix.hpp"


namespace utils {

// TRCL文字列からTR行列を作成
math::Matrix<4> generateSingleTransformMatrix(const std::string &trstr, bool warnCompat);

// TRCL番号あるいはコンマ区切り複数TRCL文字列からTR行列を作成
math::Matrix<4> generateTransformMatrix(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
										const std::string &orgTrclStr);
// 上のtrMapを使わないoverload版
math::Matrix<4> generateTransformMatrix(const std::string &trstr);

// unique_ptr作成版。TR適用するかしないか不明な場合は全部unique_ptrを扱ってif(ptr)で判定して可能な限り変換を適用しないようにしたい
//std::unique_ptr<math::Matrix<4>> makeMatrixPtr(const std::unordered_map<size_t, math::Matrix<4>> &trMap, int trNumber);

// 4x4行列からMCNPのTR文字列を作成
std::string toTrclString(const math::Matrix<4> &matrix);
}  // end namespace utils

#endif // MATRIX_UTILS_HPP
