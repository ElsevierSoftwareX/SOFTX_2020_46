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
#ifndef CELL_HPP
#define CELL_HPP

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "boundingbox.hpp"
#include "core/geometry/surface/surface.hpp"
#include "core/geometry/surface/surfacemap.hpp"
#include "core/formula/logical/lpolynomial.hpp"
#include "core/math/nvector.hpp"
#include "core/physics/physconstants.hpp"

#ifdef ENABLE_GUI
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#endif



namespace mat {
class Material;
}

namespace geom {

class Surface;

/*
 * vtkImplicitFunction* を返せば、
 * vtkSampleFunction::SetImplicitFunctionで陰関数を定義でき、
 * vtkContourFilter::SetInputConnectionで等高面を生成して、
 * vtkPolyData::DeepCopy(surface->GetOutput())で面データを生成し、
 * vtkPolyDataMapper::SetInputDataでマップにセットして
 * vtkActor::SetMapperでアクターにセットできる。
 *
 */

//  セル構築時に多項式treeを辿って陰関数を作成できれば、
// isForwardごとにtreeをたどる必要がなくなる... ように思えるがそれは多項式を展開してしまうことになるので
// 項数が爆発的に増えうる

class Cell
{
public:
	typedef std::unordered_map<std::string, std::shared_ptr<const Cell>> const_map_type;
    Cell(){}
	/*
	 * セルを構築する時、使ったsurfaceオブジェクトには、cellへのスマポを登録したいが、
	 * 「コンストラクタ内」かつ「thisへのスマポ」を作成する方法は無いので作成後に登録する必要がある。
	 * コンストラクタ外ならinterface継承でそのようなことをするクラスが用意されているが。
	 */
	// 材料データを省いた場合はvoidセルか最外殻セル
	Cell(const std::string& cellName,
		 const Surface::map_type &globalSurfaceMap,
		 const lg::LogicalExpression<int>& poly,
		 double imp);
	// 非voidセルのコンストラクタ
	Cell(const std::string& cellName,
		 const Surface::map_type &globalSurfaceMap,
		 const lg::LogicalExpression<int>& poly,
		 const std::shared_ptr<const mat::Material> &mat,
		 double dens, double imp);


	std::string cellName()const;
	std::string cellMaterialName() const;
	double importance() const {return importance_;}
	double density() const {return density_;}
    const lg::LogicalExpression<int> &polynomial() const {return polynomial_;}
    const SurfaceMap &contactSurfacesMap() const { return contactSurfacesMap_;}



    bool isInside(const math::Point& pos) const;
	bool isHeavierThanAir() const;
    bool isVoid() const;
	double macroTotalXs(phys::ParticleType ptype, double energy) const;

    BoundingBox boundingBox(size_t timeoutMsec) const;

	std::pair<const Surface *, math::Point> getNextIntersection(const math::Point &point, const math::Vector<3>& direction) const;
	std::pair<std::vector<const Surface *>, math::Point> getNextIntersections(const math::Point &point, const math::Vector<3>& direction) const;
	std::pair<std::shared_ptr<const Surface>, math::Point> getFarestIntersection(const math::Point &point, const math::Vector<3>& direction) const;


	std::string toString() const;
	std::string polynomialString() const;
	std::string toFinalInputString() const;

	// setter
	void setInitBB(const BoundingBox &bb);
	bool isUndefined() const {return cellName_ == undefindeCellName();}

private:
	std::string cellName_;          // セル名
	SurfaceMap contactSurfacesMap_;  // 隣接surfaceのマップ
	lg::LogicalExpression<int> polynomial_;
	double importance_;
	std::shared_ptr<const mat::Material> material_;  // 物質へのスマポ
	double density_;								  // 密度 g/cc
	std::shared_ptr<geom::BoundingBox> initialBB_;  // セルカードにBBオプションがあった場合初期BBを適用する。


	// 詳細なBBと簡易版のBBの計算ルーチン
	BoundingBox getRoughBB() const;
	BoundingBox getMediumBB(bool acceptMultiPiece, size_t timeoutMsec) const;
	BoundingBox getDetailedBB(size_t timeoutMsec) const;


// static
public:
	static const std::string & undefindeCellName();
	static void initUndefinedCell(const Surface::map_type& surfMap);
	static const std::shared_ptr<const Cell> &UNDEFINED_CELL_PTR();
	static const std::shared_ptr<const Cell> &guessCell(const const_map_type &cellList, const math::Point& pos, bool strict, bool enableCache);
    // セル名から親子関係にあるセルのvectorを作成する。
    static std::vector<std::string> getHierarchialCellNames(const std::string &cellName);

	// セル名の順序定義関数
	static bool cellNameLess(const std::string &str1, const std::string &str2);

	// 特殊領域で使われるセル名。通常セルはセル名文字列で表現される
    static const char UNDEF_CELL_NAME[];
    static const char VOID_CELL_NAME[];
    static const char UBOUND_CELL_NAME[];
    static const char BOUND_CELL_NAME[];
    static const char DOUBLE_CELL_NAME[];
    static const char OMITTED_CELL_NAME[];

private:
	static std::shared_ptr<const Cell> undefinedCell_;



#ifdef ENABLE_GUI
public:
	// 論理式に応じてboole演算を実行し、vtkSmartPointer<vtkImplicitFunction>を返す
    vtkSmartPointer<vtkImplicitFunction> createImplicitFunction() const;
#endif

};


// cellの順序比較関数
class CellLess{
public:
    bool operator()(const std::shared_ptr<const geom::Cell> &c1, const std::shared_ptr<const geom::Cell> &c2) const
	{
		return geom::Cell::cellNameLess(c1->cellName(), c2->cellName());
	}
};


} // end namespace geom

#endif // CELL_HPP
