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
#ifndef CELLOBJECTS_HPP
#define CELLOBJECTS_HPP

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <map>
#include "core/io/input/dataline.hpp"
#include "core/io/input/cellcard.hpp"
#include "surface/surfacemap.hpp"

namespace mat {
class Material;
}
namespace inp {
struct CellCard;
}

namespace geom {
class Surface;
class SurfaceCreator;
class Cell;



/*
 * cell カードを解釈し、cellオブジェクトを生成する。
 * 1 MAT RHO  EQUATION
 */
//#ifdef ENABLE_GUI
//#include <QObject>
//class CellCreator : public QObject
//{
//	Q_OBJECT

//signals:
//	// ファイルのオープンに成功したら親ファイルとのペアをemitする。
//	void fileOpenSucceeded(std::pair<std::string, std::string> files);

//#else
//class CellCreator
//{
//#endif
class CellCreator
{
	typedef std::unordered_map<std::string, std::shared_ptr<const Cell>> cell_list_type;
	typedef std::unordered_map<std::string, inp::CellCard> CardMap;
public:

    CellCreator(){}
	// CellカードとSurfaceオブジェクトから構築
    CellCreator(const std::list<inp::DataLine> &cellInputs,
                SurfaceCreator *sCreator,
                const std::unordered_map<std::string, std::shared_ptr<const mat::Material>> &mmap,
                bool warnPhitsCompat, int numThread, bool verbose);
	// Cellスマポのmapから構築
	explicit CellCreator(const std::unordered_map<std::string, std::shared_ptr<const Cell>> &cellMap);

	// セルリストを返す
	const cell_list_type & cells() const {return cells_;}

	// Undefined セルの初期化。全てのsurfaceを隣接surfaceとして登録する。
	void initUndefinedCell(const SurfaceMap &surfMap);

	// セルを作成
	std::shared_ptr<const Cell> createCell(inp::CellCard card) const;

	//const SurfaceCreator* surfaceCreator() const {return surfCreator_;}
	SurfaceCreator* surfaceCreator() const {return surfCreator_;}
private:

	void warnUnusedSurface(const CardMap &solvedCards) const;
	// 補集合、like-butによるセル間依存関係を解決する。
	void solveCellDependency(bool warnPhitsCompat, const std::list<inp::DataLine> &cellInput, CardMap* solvedCards) const;
	// 明示的同階層のTRCLを適用する。(TRCL後のsurfaceを作成し、多項式中のsurface名を変更する)
	void applyExplicitTRCL(CardMap* solvedCards);  // surfMapを変更するのでconstではない
	// Lattice定義セルを展開して各要素セルを生成する。(セル内部は未充填)
	void appendLatticeElements(CardMap * solvedCards);  // surfMapを変更するのでconstではない
	// filledセルにuniverseを充填して個別のセルを生成する
    void fillUniverse(int numThread, const CardMap& solvedCards, std::vector<inp::CellCard> *cellcards, bool verbose);


	// Cellの重複定義をチェックするために cell名をキーにsする。
	cell_list_type cells_;
	SurfaceCreator *surfCreator_;
	std::unordered_map<std::string, std::shared_ptr<const mat::Material>> materialMap_;
};

std::ostream& operator << (std::ostream& os, const CellCreator& cellObjs);

}  // end namespace geom
#endif // CELLOBJECTS_HPP
