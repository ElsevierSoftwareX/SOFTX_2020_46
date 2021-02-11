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
#ifndef GEOM_UTILS_HPP
#define GEOM_UTILS_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include "../geometry/surface/surface.hpp"


namespace geom {
class SurfaceMap;
class Cell;
}

namespace utils {

void dumpSurfaceMap(const geom::SurfaceMap &surfMap);

// "-s" と"s"を表裏逆の別の面として扱うことにしたので、
// "s"を生成した時は"-s"を、
// "-s"を生成した時は"s"を追加しなければならない
void addReverseSurfaces(geom::SurfaceMap *surfaceMap);

/*
 * CellとSurfaceの接続関係を作成する。
 * cellにはCell::contactSurfaceMap_に隣接Surfaceが登録済みなので、
 * それを手繰ってsurfaceにCellを登録する。
 * あるsurfaceを追加する場合、その表裏逆surfaceも登録する必要がある。
 *
 * ※cellMapは変更されない(const)が、cellMapのメンバを通じてSurfaceには変更が生じることに注意。
 */
void updateCellSurfaceConnection(const std::unordered_map<std::string, std::shared_ptr<const geom::Cell> > &cellMap);


// contact Cellを持たないsurfaceを削除。
void removeUnusedSurfaces(geom::SurfaceMap *surfaceMap, bool warn = true);

std::string toString(const std::unordered_map<std::string, std::shared_ptr<geom::Cell>> &cellMap);
}

#endif // GEOM_UTILS_HPP
