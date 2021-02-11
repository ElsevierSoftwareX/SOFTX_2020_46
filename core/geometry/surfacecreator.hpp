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
#ifndef SURFACEOBJECTS_HPP
#define SURFACEOBJECTS_HPP

#include <iostream>
#include <list>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "core/math/nmatrix.hpp"
#include "core/io/input/dataline.hpp"
#include "core/geometry/surface/surface.hpp"

namespace geom {

/*
 * surfaceカードを解釈し、surfaceオブジェクトを生成する。
 *
 * 10 S 0 0 0 5 $コメント
 *
 * 但しコメントは入力ファイル読み取り時に除去し、複数行データも連結済みとする。
 */
class SurfaceCreator
{
	friend class CellCreator;
	typedef std::unordered_map<size_t, math::Matrix<4>> matrix_map_type;
	template <class KEY, class T>	using map_const_type = std::unordered_map<KEY, std::shared_ptr<const T>>;
public:

	/*
	 * コンストラクタは
	 * ・Datalistとmatrixmapから構築
	 * ・SurfaceオブジェクトのMapから直接構築のどちらか。
	 */
	SurfaceCreator(){;}
	SurfaceCreator(const std::list<inp::DataLine>& surfInputList,
				   const std::unordered_map<size_t, math::Matrix<4>>& trMatrixes,
				   bool warnPhitsCompat);
    explicit SurfaceCreator(const Surface::map_type& surfMap);

    const Surface::map_type& map() const {return surfaceMap_;}
    Surface::map_type *mapPointer() {return &surfaceMap_;}
	void removeUnusedSurfaces(bool warn = true);  // 使われていないSurfaceを削除
	const matrix_map_type &transformationMatrixMap() const { return trMap_;}

	static std::string getSurfaceSymbol(const std::string &str);
private:
    Surface::map_type surfaceMap_;
	matrix_map_type trMap_;

};

std::ostream& operator <<(std::ostream& os, const SurfaceCreator &surfObjs);



}  // end namespace geom
#endif // SURFACEOBJECTS_HPP
