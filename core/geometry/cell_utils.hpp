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
#ifndef CELL_UTILS_HPP
#define CELL_UTILS_HPP

#include <memory>
#include <string>
#include <unordered_map>

namespace geom{
class Cell;
class SurfaceMap;
}

namespace utils{


void dumpSurfaceMap(const geom::SurfaceMap &surfMap);
/*
 * CellとSurfaceの接続関係を作成する。
 * cellにはCell::contactSurfaceMap_に隣接Surfaceが登録済みなので、
 * それを手繰ってsurfaceにCellを登録する。
 * あるsurfaceを追加する場合、その表裏逆surfaceも登録する必要がある。
 *
 * ※cellMapは変更されない(const)が、cellMapのメンバを通じてSurfaceには変更が生じることに注意。
 */
void updateCellSurfaceConnection(const std::unordered_map<std::string, std::shared_ptr<const geom::Cell> > &cellMap);



std::string toString(const std::unordered_map<std::string, std::shared_ptr<geom::Cell>> &cellMap);

}

#endif // CELL_UTILS_HPP
