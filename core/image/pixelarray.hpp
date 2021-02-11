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
#ifndef XPMPIXELARRAY_H
#define XPMPIXELARRAY_H

#include <cassert>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>


#include "cellcolorpalette.hpp"

namespace img {
class TracingRayData;
/*
 * サイズの2次元ピクセルアレイ
 *
 * 内部的には1次元vectorで領域を確保し、
 * operator(xindex, yindex)でアクセスする。
 *
 * Array(0, 0)が画面左上の点。その右隣りが(1, 0)となる。
 * メモリ上でデータはy方向に連続して配置する。
 */
class PixelArray{
public:
	typedef int pixel_type;
	enum class RayDir {HORIZONTAL, VERTICAL};

    PixelArray()
        : horizontalSize_(0), verticalSize_(0){}
	PixelArray(size_t hsize, size_t vsize)
		:horizontalSize_(hsize), verticalSize_(vsize)
	{
		dataArray_.resize(horizontalSize_*verticalSize_, 0);
	}
	void clear()
	{
		horizontalSize_ = 0;
		verticalSize_ = 0;
		dataArray_.clear();
	}

	void resize(size_t hsize, size_t vsize)
	{
		horizontalSize_ = hsize;
		verticalSize_ = vsize;
		dataArray_.resize(horizontalSize_*verticalSize_, 0);
	}
	bool empty() const {return dataArray_.empty();}
	size_t horizontalSize() const {return horizontalSize_;}
	size_t verticalSize() const {return verticalSize_;}

    // 一次元配列としてアクセス
	const pixel_type &at(size_t index) const {return dataArray_.at(index);}
	pixel_type &at(size_t index) {return dataArray_.at(index);}

	// 位置xindex,yindexの文字を返す
	const pixel_type& operator()(size_t xindex, size_t yindex) const;
	pixel_type &operator()(size_t xindex, size_t yindex);

	// 水平方向に連結する。引数の中身はmoveされるのでこのルーチン以後はnullptrとなる。
	void hMoveConcat(PixelArray *arr);
	// targetPatternに該当するpixelをexpandWidth分だけ太らせる。
	void expandPixel(PixelArray::pixel_type targetPattern, int expandWidth);
	std::string toXpmString(std::function<char(pixel_type)> pixToXpmCharFunc) const;

	// 2つの画像をマージする。
	// 第一引数アレイか第二引数アレイにpriorPatternが現れた場合その位置はpriorPatternに設定する。
	// priorPattern以外で第1引数アレイと第2引数アレイが異なる場合conflictedを適用する。
	static PixelArray merge(const PixelArray& parray1, const PixelArray& parray2,
							const std::vector<pixel_type> &priorPattern,
							PixelArray::pixel_type conflicted);
	// 走査データからpixelアレイを構築
	static PixelArray renderingFromRayData(RayDir dir,
										   size_t hReso, size_t vReso, double xCm, double yCm,
										   const std::vector<TracingRayData> &rays,
										   const CellColorPalette &palette
										   );
	// 水平/垂直方向に連結
	static PixelArray hConcat(const PixelArray &parray1, const PixelArray &parray2);
	static PixelArray vConcat(const PixelArray &parray1, const PixelArray &parray2);
	// 水平/垂直方向に反転
	static PixelArray hFlip(const PixelArray &parray1);
	static PixelArray vFlip(const PixelArray &parray1);

private:
	std::vector<pixel_type> dataArray_;
	size_t horizontalSize_;
	size_t verticalSize_;
};






}  // end namespace img

#endif // XPMPIXELARRAY_H
