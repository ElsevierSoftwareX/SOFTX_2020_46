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
#include "surf_utils.hpp"

#include <memory>

#include "surface/surface.hpp"
#include "surface/surfacemap.hpp"


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
