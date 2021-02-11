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
#ifndef SURFACECARDPARSER_HPP
#define SURFACECARDPARSER_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include "surface/surface.hpp"
#include "../math/nmatrix.hpp"

namespace geom {


/*
 * surfaceカードを解釈し、surfaceオブジェクトを生成する。
 *
 * 10 S 0 0 0 5 $コメント
 *
 * 但しコメントは入力ファイル読み取り時に除去し、複数行データも連結済みとする。
 */
class SurfaceCardParser
{
	typedef std::unordered_map<size_t, math::Matrix<4>> matrix_map_type;
public:
	explicit SurfaceCardParser(const matrix_map_type& trmap):trMap_(trmap){;}

	std::shared_ptr<Surface> createSurface(const std::string& str);
	std::pair<std::string, std::shared_ptr<Surface> > createSurfacePair(const std::string& str);
private:
	matrix_map_type trMap_;
};


}  // end namespace geom
#endif // SURFACECARDPARSER_HPP
