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
#ifndef XPMCOLORS_HPP
#define XPMCOLORS_HPP

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace img {


class XpmColor
{
	static constexpr size_t COLOR_INDEX_OFFSET = 5;
	// XPMで使う文字のカタログ
	// 最初が半角空白ならあとは何でも良い.視認性の良い文字を先に使いたい。
	// 並びの最初は "未定義セル" "voidセル" "未定義境界" "境界" "省略"に使われることを想定。
    static const char asciichars[];// = " .!|I+-:;=?@_$%&0123456789<ABCDEFGHJKLMNOPQRSTUVWXYZ^abcdefghijklmnopqrstuvwxyz~";

public:
	static int maxColorNumber();
	// インデックス番号からxpm色文字へ変換する関数
	// 色数が限界を超えると特殊色5色を除いて使いまわす。
    static char colorChar(int index);
};



}  // end namespace img

#endif // XPMCOLORS_HPP
