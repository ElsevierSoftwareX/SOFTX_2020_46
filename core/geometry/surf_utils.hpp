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
#ifndef SURF_UTILS_HPP
#define SURF_UTILS_HPP


namespace geom{
class SurfaceMap;
}

namespace utils {


// "-s" と"s"を表裏逆の別の面として扱うことにしたので、
// "s"を生成した時は"-s"を、
// "-s"を生成した時は"s"を追加しなければならない
void addReverseSurfaces(geom::SurfaceMap *surfaceMap);

// contact Cellを持たないsurfaceを削除。
void removeUnusedSurfaces(geom::SurfaceMap *surfaceMap, bool warn = true);
}


#endif // SURF_UTILS_HPP
