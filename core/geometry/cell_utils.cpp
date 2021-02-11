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
#include "cell_utils.hpp"

#include <iostream>
#include "surface/surfacemap.hpp"
#include "cell/cell.hpp"

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

std::string utils::toString(const std::unordered_map<std::string, std::shared_ptr<geom::Cell> > &cellMap)
{
    std::stringstream ss;
    for(auto &cellPair: cellMap) {
        ss << cellPair.second->cellName() << " ";
    }
    return ss.str();
}
