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
#include "geom_utils.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

#include "../geometry/cell/cell.hpp"
#include "../utils/message.hpp"
#include "surface/surfacemap.hpp"

// FXIME このcpp はSurface関連とCell関連に分ける必要が有る。
// そうしないとSurfaceのテストの時にCellまで必要としてしまう。

void utils::addReverseSurfaces(geom::SurfaceMap *surfaceMap)
{
	geom::Surface::map_type tmpMap;
	for(auto &frontSurfPair: surfaceMap->frontSurfaces()) {
		// frontSurfPair はindexとsurfaceを継承した面のスマポのペア
		//mDebug() << "Creating reverse surface for surfName=" << frontSurf->name();
		// 表裏逆面がmapに無いので生成・追加する
		std::shared_ptr<geom::Surface> revSurf(frontSurfPair.second->createReverse());
		surfaceMap->registerSurface(revSurf->getID(), revSurf);
	}
}


// ※この関数ではconst_castで隣接するsurfaceのインスタンスの変更操作を行っている
void utils::updateCellSurfaceConnection(const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cellMap)
{
	/*
	 * セルの接触しているを探し、面にそのセルを登録する。
	 * 問題は Cell::contactSurfaceMapは shared_ptr<const Surface>を保持しているので直接書き換えることはできない。
	 * 方法1：const_castする。→ shared<Surface>とshared<const Surface>は別の型なのでconst_castできない。
	 * 方法2：なので一旦生ポとってからconst_cast
	 * 方法3：Cell::contactSurfaceMapからsurface名を取得して、この関数の引数で与えるsurfaceMapを書き換える。
	 *
	 *  NOTE const_castは無くしたい。暫定的に方法2をとっているが方法3に切り替えたい。
	 */
	// cellPair.secondがshared_ptr<Cell>
	for(const auto& cellPair: cellMap) {
		// contactSurfacesMapに含まれる全surfaceに対して registerContactCellを実行する必要がある。
		auto surfaceMap = cellPair.second->contactSurfacesMap();
		for(const auto &frontSurfPair: surfaceMap.frontSurfaces()) {
//			const_cast<geom::Surface*>(frontSurfPair.second.get())->registerContactCell(cellPair.second);
            const_cast<geom::Surface*>(frontSurfPair.second.get())->registerContactCell(cellPair.first, cellPair.second.get());
		}
		for(const auto &backSurfPair: surfaceMap.backSurfaces()) {
//			const_cast<geom::Surface*>(backSurfPair.second.get())->registerContactCell(cellPair.second);
            const_cast<geom::Surface*>(backSurfPair.second.get())->registerContactCell(cellPair.first, cellPair.second.get());
		}

//		for(auto it = surfaceMap.frontSideBegin(); it != surfaceMap.frontSideEnd(); ++it) {
//			const_cast<geom::Surface*>(it->get())->registerContactCell(cellPair.second);
//		}
//		for(auto it = surfaceMap.backSideBegin(); it != surfaceMap.backSideEnd(); ++it) {
//			const_cast<geom::Surface*>(it->get())->registerContactCell(cellPair.second);
//		}

//		for(auto &surfPair: cellPair.second->contactSurfacesMap()) {
//			// surfPair.firstがstring(Surface名)
//			// surfPair.secondはshared_ptr<const Surface>
//			//surfPair.second->registerContactCell(cellPair.second);
//			const_cast<geom::Surface*>(surfPair.second.get())->registerContactCell(cellPair.second);
//		}
	}
}


void utils::dumpSurfaceMap(const geom::SurfaceMap &surfMap)
{
	for(const auto &surfPair: surfMap.frontSurfaces()) {
		std::cout << "Contact cells of surface " << surfPair.second->name() << " = ";
		for(const auto &p: surfPair.second->contactCellsMap()) {
			//std::cout << p.second.lock()->cellName() << " ";  // weak_ptr
			std::cout << p.second->cellName();// ナマポ版
		}
		std::cout << std::endl;
	}
}

void utils::removeUnusedSurfaces(geom::SurfaceMap *surfaceMap, bool warn)
{
	std::vector<int> targetIndexes;
	/*
	 *  いずれのセルににも隣接していないsurfaceは削除対象としてリストアップする。
	 * ただし、削除したところで実行速度は変わらずメモリ使用量が僅かに減るだけと思われる。
	 */
	for(const auto &surfPair: surfaceMap->frontSurfaces()) {
		if(surfPair.second->contactCellsMap().empty()) {
			targetIndexes.push_back(surfPair.second->getID()); // 削除対象のキーを追加
		}
	}
	for(const auto &surfPair: surfaceMap->backSurfaces()) {
		if(surfPair.second->contactCellsMap().empty()) {
			targetIndexes.push_back(surfPair.second->getID()); // 裏面も対象削除対象を追加
		}
	}

	// 削除するsurface名は辞書順に並べたほうが警告出力がわかりやすい。
	std::sort(targetIndexes.begin(), targetIndexes.end());
	for(auto& index: targetIndexes) {

		// 不使用セルの警告の目的は入力データのチェックなので、
		// 明示的に生成したSurfaceを不使用の時のみ警告する。
		/*
		 * Surface: S1 S 0 0 0 10
		 *          S2 S 1 1 0  1
		 * Cell:    C1 -S1
		 *
		 * の場合S1から暗黙に-S1が、S2から-S2が生成される。実際に使われるのは±S1のため
		 * ±S2は削除される。この場合明示的に生成していない-S2削除の警告メッセージは出したくない。
		 *
		 * 故に警告する条件はおもて面のみが対象となる。
		 */
		if(warn && surfaceMap->at(index)->getID() > 0) {
			mWarning() << "Surface \"" << surfaceMap->at(index)->name()
					  << "\" is defined but not used. Removed.";
		}
		// 未定義セルは全サーフェイスに隣接することにしているので
		// ここで隣接セルを持たない面は削除しておかないと
		// 未定義セルへ粒子が入射した場合交点が求まらなくなる。
		surfaceMap->erase(index);

	}
}

std::string utils::toString(const std::unordered_map<std::string, std::shared_ptr<geom::Cell> > &cellMap)
{
	std::stringstream ss;
	for(auto &cellPair: cellMap) {
		ss << cellPair.second->cellName() << " ";
	}
	return ss.str();
}
