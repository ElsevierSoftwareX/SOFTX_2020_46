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
#include "xpmcolors.hpp"

const char img::XpmColor::asciichars[]
	= "!.ilI+-:;=@_$0123456789<ABCDEFGHJKLMNOPQRSTUVWXYZ^abcdefghjkmnopqrstuvwxyz~";

int img::XpmColor::maxColorNumber() {return static_cast<int>(sizeof(asciichars));}

char img::XpmColor::colorChar(int index) {
    if(index < static_cast<int>(sizeof(asciichars))) {
        return asciichars[index];
    } else {
        return asciichars[COLOR_INDEX_OFFSET + index%(sizeof(asciichars) - COLOR_INDEX_OFFSET)];
    }

}
