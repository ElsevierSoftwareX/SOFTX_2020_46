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
#ifndef IMAGE2D_HPP
#define IMAGE2D_HPP

#include <cassert>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pixelarray.hpp"
#include "core/math/nvector.hpp"
#include "color.hpp"

namespace mat {
class Material;
}

namespace img {

enum class DIR {H, V};


class BitmapImage
{
public:
	typedef std::tuple<std::string, std::string, img::Color> color_tuple;
	typedef std::vector<img::TracingRayData> raydata_type;
	BitmapImage():widthCm_(0), heightCm_(0){;}
	BitmapImage(DIR raydir, size_t hReso, size_t vReso, double hSize, double vSize,
				const std::vector<TracingRayData> &rays,
				const CellColorPalette &pal);

	// getter
	size_t hResolution() const {return pixelArray_.horizontalSize();}
	size_t vResolution() const {return pixelArray_.verticalSize();}
	double widthCm() const {return widthCm_;}
	double heightCm() const {return heightCm_;}
	const CellColorPalette& palette() const {return palette_;}
	const PixelArray &pixelArray() const {return pixelArray_;}
	bool empty() const {return pixelArray_.empty();}

	// setter
	// hRaysが空なら鉛直走査画像、vRaysが空なら水平走査画像になる。
	void setRayData(DIR raydir, const std::vector<TracingRayData> &rays, size_t hReso, size_t vReso);
	void setSizeCm(double wid, double hei);
	void setWidthCm(const double &xwidth);
	void setHeightCm(const double &ywidth);

	// 描画ルーチン。
	// インデックスがpindexの部分を幅widthだけ拡大。
//	void expandRegion(int linewidth, std::vector<std::string> lineRegionNames);
	void expandRegion(int width, const std::string &regionName);
	//void expandLineWidth(int linewidth);
	// インデックスがtargetの画素の輪郭をedgeで塗る
	void drawEdge(PixelArray::pixel_type target, PixelArray::pixel_type edge);
	// 位置xyz(piexel)に色(#00ff00式のstring)、大きさsz(pixel)の点を描画
	void drawSquareMark(int xindex, int yindex, int sz, const Color &color, bool filled);
	void drawCrossMark(int xindex, int yindex, int sz, const Color &color);
	void drawSquareCrossMark(int xindex, int yindex, int sz, const Color &color);
	// xpmファイルに保存
	void exportToXpmFile(const std::string &filename);
	// xpmデータ文字列(ファイル書き出し用)
	std::string exportToXpmString() const;

	static BitmapImage flipHorizontally(const BitmapImage &img);
	static BitmapImage flipVertically(const BitmapImage &img);
	static BitmapImage concatHorizontally(const BitmapImage &img1, const BitmapImage &img2);
	static BitmapImage concatVertically(const BitmapImage &img1, const BitmapImage &img2);
	static BitmapImage merge(const BitmapImage &img1, const BitmapImage &img2,
							 const std::vector<std::string> &priorRegionNames,
							 const std::string &conflictedRegionName);

	// xpm文字列(std::string)から適当にsplitと型変換して文字列配列(const char* [])に変換する関数
	static void XpmStrToXpmData(const std::string &sourceStr, char **&arr);
private:
	double widthCm_;
	double heightCm_;
	PixelArray pixelArray_;

	// 色パレットは cell名--材料名--インデックス番号--RGB文字列を保持する。
	CellColorPalette palette_;
	// privateメソッド
	// パラメータチェック
	bool checkInside(int xpix, int ypix, const std::string &errorMessage);


};


}  // end namespace img
#endif // IMAGE2D_HPP
