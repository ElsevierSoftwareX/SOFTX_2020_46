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
#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <list>
#include <map>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "core/io/input/inputdata.hpp"
#include "core/io/input/mcmode.hpp"
#include "core/image/bitmapimage.hpp"
#include "core/math/nmatrix.hpp"
#include "core/option/config.hpp"
#include "core/physics/physconstants.hpp"

namespace inp {
class DataLine;
}

namespace img {
class MaterialColorData;
}

namespace math {
template <unsigned int M> class Matrix;
template <unsigned int M> class Vector;
}

namespace mat {
class Material;
class Materials;
}


namespace geom {

class Surface;
class SurfaceMap;
class Cell;

class Geometry
{


public:
	static std::shared_ptr<Geometry> createGeometry(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
													const std::list<inp::DataLine> &cellCards,
													const std::list<inp::DataLine> &surfaceCards,
													const std::shared_ptr<const mat::Materials> &materials,
													const conf::Config &config);

	Geometry() {;}
	// MCNPに対応しやすいようにPhitsInputSectionではなくlist<Dataline>から構築する。
	// list<Dataline>で入力ファイルから構築することを想定。
	Geometry(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
			 std::list<inp::DataLine> surfaceInput,
			 std::list<inp::DataLine> cellInput,
			 const std::shared_ptr<const mat::Materials> &materials,
			 bool verbose, bool warnPhitsCompat, int numThread);

	// testで使いやすいようにunorderedMapから構築
	Geometry(const geom::SurfaceMap &surfMap,
			 const std::unordered_map<std::string, std::shared_ptr<const Cell> > &cellMap);

	/*
	 * 断面画像出力
	 * 原点originから
	 * dir1を横軸に、
	 * dir2を縦軸にして
	 * 断面をファイルfilenameに出力する。
	 *
	 * 描画はoriginを左下にして、右上が0.5*(dir1+dir2)の範囲となる。
	 * 解像度は hReso×vResoとなる。
	 *
	 */
	img::BitmapImage getSectionalImage(const math::Vector<3> &origin,
								const math::Vector<3> &hdir, const math::Vector<3> &vdir,
                                size_t hReso, size_t vReso, int numThread = 1, bool verbose = false, bool quiet = false) const;

	// ptからdir方向に進んだ時に次にぶつかるセルのスマポを返す。副作用でptは交点+deltaでセル内に入った点まで進む。
	const Cell *getNextCell(const geom::Cell* startCell, const math::Vector<3> &dir, math::Point *pt) const;

	const std::unordered_map<std::string, std::shared_ptr<const Cell>> & cells() const {return cells_;}
	const std::unordered_map<size_t, math::Matrix<4>> &trMap() const {return trMap_;}

	// 色関係
    void clearUserDefinedPalette();
	void setDefaultPalette();
	// ユーザー定義の色設定を適用したパレットを作成する。
    void createModifiedPalette(const std::map<std::string, img::MaterialColorData> &matMap);
	const img::CellColorPalette &palette() const {return palette_;}
    const std::unordered_map<std::string, int> & surfaceNameIndexMap() const {return surfaceIndexNameMap_;}


	//! メタカード展開後の最終的な入力ファイルデータを返す。（未完）
	const std::string toFinalInputString() const;
private:
	/*
	 * このクラスが保持するデータはcellの集合だけで良い。面、材料データはcellが保持している。
	 */
	// shared<"const" Cell> であることはデータ競合を防ぐために必要。
	std::unordered_map<std::string, std::shared_ptr<const Cell>> cells_;
    std::unordered_map<std::string, int> surfaceIndexNameMap_; // surface名、surfaceIDのマップ
    std::unordered_map<size_t, math::Matrix<4>> trMap_;
	img::CellColorPalette palette_;
	// 予約セルのパレットを適用
	void setReservedPalette();

    static void expandMacroBody(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
        std::list<inp::DataLine> *surfInputList,
        std::list<inp::DataLine> *cellInputList);

};


}  // namespace geom


#endif // GEOMETRY_HPP
