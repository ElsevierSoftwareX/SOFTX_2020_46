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
#include "bitmapimage.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>

#include "xpmcolors.hpp"
#include "tracingraydata.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/numeric_utils.hpp"

#include "core/utils/utils.hpp"
#include "core/material/material.hpp"

namespace {


// 文字列から整数へ変換。失敗したら第二引数の値を返す
template <class T>
T convertOrValue(const std::string &str,  T failedValue)
{
	try{
		return utils::stringTo<T>(str);
	} catch (...) {
		return failedValue;
	}
}

}  // end anonymous namespace

// PixelArrayにはMaterialに対応した整数、材料インデックスを保存する。
img::BitmapImage::BitmapImage(img::DIR raydir, size_t hReso, size_t vReso, double hSize, double vSize,
								const std::vector<img::TracingRayData> &rays,
								const img::CellColorPalette &pal)
	:widthCm_(hSize), heightCm_(vSize), palette_(pal)
{
    //std::vector<std::string> namevector = palette_.getCellNames();
	setRayData(raydir, rays, hReso, vReso);
}


// 材料名ベースの色分けでイメージを作成
void img::BitmapImage::setRayData(img::DIR raydir,
									const std::vector<img::TracingRayData> &rays,
									size_t hReso, size_t vReso
							  )
{
	// 色テーブルが事前に定義されていない場合データから自動生成する。
	// しかし自動生成だとデータ中の色数が変わるとセルの色配分が変わってわかりにくいから望ましくない。
	// BitmapImage()は空のビットマップイメージで、().numColors()はデフォルトで予約済みの色数返す。
	if(palette_.empty()) {
		std::cerr << "ProgramError: No color palette defined. Call genColorTable before." << std::endl;
		std::exit(EXIT_FAILURE);
	}

	/*
	 * 2方向走査線でまともな画像を得るには
	 * ・水平方向走査線の数と鉛直解像度が一致(厳密には整数倍)
	 * ・垂直方向走査線の数と水平解像度が一致(厳密には整数倍)
	 */
	// 優先インデックスは先頭の方がより優先される。ここでは1.未定義境界、2．通常境界の順で優先される。
	// indexが競合した時は未定義領域インデックスを適用する。

	//結局 pixelArrayには pixel = palette.getIndexByCellName(cellname)のように
	// パレットでセル名をキーにしてセルに対応した一意のインデックスを保存している。
	switch(raydir) {
	case img::DIR::H:
		pixelArray_ = PixelArray::renderingFromRayData(PixelArray::RayDir::HORIZONTAL,
													   hReso, vReso, widthCm_, heightCm_,
													   rays, palette_);
		break;
	case img::DIR::V:
		pixelArray_ = PixelArray::renderingFromRayData(PixelArray::RayDir::VERTICAL,
													   hReso, vReso, widthCm_, heightCm_,
													   rays, palette_);
		break;
	default:
		abort();
	}
}

//void img::BitmapImage::expandLineWidth(int linewidth)
//{
//	if(linewidth <= 1) return;
////	for (auto &linePixelName: lineRegionNames) {
////		pixelArray_.expandPixel(palette_.getIndexByCellName(linePixelName), linewidth-1);
////	}
//	pixelArray_.expandPixel(palette_.getIndexByCellName(geom::Cell::UBOUND_CELL_NAME), linewidth-1);
//	pixelArray_.expandPixel(palette_.getIndexByCellName(geom::Cell::BOUND_CELL_NAME), linewidth-1);
//}

void img::BitmapImage::expandRegion(int width, const std::string &regionName)
{
	if(width <= 1) return;
	pixelArray_.expandPixel(palette_.getIndexByCellName(regionName), width-1);
}


void img::BitmapImage::drawEdge(img::PixelArray::pixel_type target, img::PixelArray::pixel_type edge)
{
	auto hsize = hResolution(), vsize = vResolution();
	for(size_t vindex = 0; vindex < vsize; ++vindex) {
		for(size_t hindex = 0; hindex < hsize; ++hindex) {
			if(pixelArray_(hindex, vindex) == target) {
				if(hindex > 1        && pixelArray_(hindex - 1, vindex) != target) pixelArray_(hindex - 1, vindex) = edge;
				if(hindex != hsize-1 && pixelArray_(hindex + 1, vindex) != target) pixelArray_(hindex + 1, vindex) = edge;
				if(vindex > 1        && pixelArray_(hindex, vindex - 1) != target) pixelArray_(hindex, vindex - 1) = edge;
				if(vindex != vsize-1 && pixelArray_(hindex, vindex + 1) != target) pixelArray_(hindex, vindex + 1) = edge;
			}
		}
	}
}


std::string img::BitmapImage::exportToXpmString() const
{
	if(this->empty()) return "";
	std::stringstream ofs;
    // xpmのヘッダ部分
    ofs	<< "\"" << hResolution() << " " << vResolution() << " " << palette_.size()<< " 1\"," << std::endl;

    // 重複のない材料・色データ一覧を作成する。cellTocolorIndexは多対1なのでユニークなindexの一覧が欲しい。
//    std::vector<int> indexList = palette_.getIndexes();
//    std::sort(indexList.begin(), indexList.end());
    // material名がキー、MaterialColorDataが値のマップ
    auto matColorDataVec = palette_.materialColorDataList();

	// 色定義部分
	// 表示を揃えるために色の説明文字列で一番長いものを取得
	size_t nameStrLength = 0;
//	for(auto &matName: palette_.getMaterialNames()) {
//		// ユーザー定義色はindex < 0 なのでXpmColor::colorName()では求まらない。"user_defined"決め打ちだから無視。
//		nameStrLength = std::max(matName.size(), nameStrLength);
//	}
    for(const auto& matColData: matColorDataVec) {
        nameStrLength = std::max(matColData->matName().size(), nameStrLength);
    }


    int maxNumColor = XpmColor::maxColorNumber();
	int numColor = 0;
    for(size_t i = 0; i < matColorDataVec.size(); ++i) {
        if(++numColor > maxNumColor) {
            std::cerr << "Warning: Number of colors exceeds max number(" << maxNumColor << "),"
                      << "some colors are duplicated." << std::endl;
        }
        auto materialName = matColorDataVec.at(i)->matName();
        ofs << "\"" << XpmColor::colorChar(static_cast<int>(i))
            << " s " << std::left << std::setw(static_cast<int>(nameStrLength+1)) << materialName
            << " c ";
        auto currentColor = matColorDataVec.at(i)->color();
        if(currentColor->a <= 0) {
            ofs << "none" << "\",\n";   // xpmでは透明色は"none"で表される
        } else {
            ofs << currentColor->toRgbString() << "\",\n";
        }
    }


//	for(auto &index: indexList) {
//        if(++numColor > maxNumColor) {
//            std::cerr << "Warning: Number of colors exceeds max number(" << maxNumColor << "),"
//                      << "some colors are duplicated." << std::endl;
//        }

//        auto materialName = palette_.getMaterialNameByIndex(index);



//		ofs << "\"" << XpmColor::colorChar(index)
//            << " s " << std::left << std::setw(nameStrLength+1) << materialName
//			<< " c ";
//		auto tmpColor = palette_.getColorByIndex(index);
//		if(tmpColor.a <= 0) {
//			ofs << "none" << "\",\n";   // xpmでは透明色は"none"で表される
//		} else {
//			ofs << palette_.getColorByIndex(index).toRgbString() << "\",\n";
//		}
//	}

    // toXpmStringメソッドは int→char変換関数オブジェクトを引数に取る。
    // これはxpmファイルのヘッダ部分の文字-色対応部分と矛盾しては行けない。
	ofs << pixelArray_.toXpmString(&img::XpmColor::colorChar);
	return ofs.str();
}

void img::BitmapImage::exportToXpmFile(const std::string &filename)
{
	std::ofstream ofs(filename.c_str());
	if(ofs.fail()) {
		mWarning() << "File=" << filename << " could not be opened.";
		return;
	}
	// ヘッダ
	ofs << "/* XPM */\n"
		<< "static char * roundb_xpm[] = {\n"
		<< "/* width height ncolors chars_per_pixel */\n";
	ofs << exportToXpmString();
	ofs << "};" << std::endl;
}



img::BitmapImage img::BitmapImage::flipHorizontally(const img::BitmapImage &img)
{
	BitmapImage bitmap;
	bitmap.widthCm_ = img.widthCm_;
	bitmap.heightCm_ = img.heightCm_;
	bitmap.palette_ = img.palette_;
	bitmap.pixelArray_ = PixelArray::hFlip(img.pixelArray_);
	return bitmap;
}

img::BitmapImage img::BitmapImage::flipVertically(const img::BitmapImage &img)
{
	BitmapImage bitmap;
	bitmap.widthCm_ = img.widthCm_;
	bitmap.heightCm_ = img.heightCm_;
	bitmap.palette_ = img.palette_;
	bitmap.pixelArray_ = PixelArray::vFlip(img.pixelArray_);
	return bitmap;
}



img::BitmapImage img::BitmapImage::concatHorizontally(const img::BitmapImage &img1, const img::BitmapImage &img2)
{
	std::stringstream ss;
	// 連結方向と直交する方向のサイズ、解像度が同じでなければ話にならない。
	if(img1.vResolution() != img2.vResolution()) {
		ss << "Vertical resolutions are different. v1="
		   << img1.vResolution() << ", v2=" << img2.vResolution();
		throw std::invalid_argument(ss.str());
	} else if (!utils::isSameDouble(img1.heightCm(), img2.heightCm())) {
		ss << "Vertical size(cm) are different. v1="
		   << img1.heightCm() << ", v2=" << img2.heightCm();
		throw std::invalid_argument(ss.str());
	} else if(img1.palette_.empty() || img2.palette_.empty()) {
		throw std::invalid_argument("Color palette of concatenating img is empty");
	}
	/*
	 * 色パレットをマージする必要がある。
	 * 色パレットはコンストラクタ引数の、pair<cell名,material名>リストが同じなら同じ。
	 * 利用シチュエーションを考えれば空か同じ値が入っているはず。
	 * ・cell-material関係は違っていたらエラー
	 * ・material-index関係は違っていたら統一
	 * ・index-color関係は違っていたら統一
	 * 等の作業が本来必要だが、空でなければ同じと見做す。
	 */
	BitmapImage image;
	image.widthCm_ = img1.widthCm_ + img2.widthCm_;
	image.heightCm_ = img1.heightCm_;
	image.palette_ = img1.palette_;
	image.pixelArray_ = PixelArray::hConcat(img1.pixelArray_, img2.pixelArray_);
	return image;
}

img::BitmapImage img::BitmapImage::concatVertically(const BitmapImage &img1, const img::BitmapImage &img2)
{
	std::stringstream ss;
	if(img1.hResolution() != img2.hResolution()) {
		ss << "Horizontal resolutions are different. h1="
		   << img1.hResolution() << ", h2=" << img2.hResolution();
		throw std::invalid_argument(ss.str());
	} else if (!utils::isSameDouble(img1.widthCm(), img2.widthCm())) {
		ss << "Horizontal size(cm) are different. h1="
		   << img1.widthCm() << ", h2=" << img2.widthCm();
		throw std::invalid_argument(ss.str());
	} else if(img1.palette_.empty() || img2.palette_.empty()) {
		throw std::invalid_argument("Color palette of concatenating img is empty");
	}
	BitmapImage image;
	image.widthCm_ = img1.widthCm_;
	image.heightCm_ = img1.heightCm_ + img2.heightCm_;
	image.palette_ = img1.palette_;
	image.pixelArray_ = PixelArray::vConcat(img1.pixelArray_, img2.pixelArray_);
	return image;
}

// priorIndexは優先させる領域の名前、conflictedIndexは2つの画像でコンフリクトした場合の置き換える領域名
img::BitmapImage img::BitmapImage::merge(const img::BitmapImage &img1, const img::BitmapImage &img2,
										 const std::vector<std::string> &priorRegionNames,
										 const std::string &conflictedRegionName)
{
	std::stringstream ss;
	if(img1.hResolution() != img2.hResolution() || img1.vResolution() != img2.vResolution()) {
		ss << "Resolutions are different."
		   << "(" << img1.hResolution() << ", " << img1.vResolution() << "),"
		   <<" (" << img2.hResolution() << ", " << img2.vResolution() << ")";
	} else if (!utils::isSameDouble(img1.widthCm(), img2.widthCm())
			|| !utils::isSameDouble(img1.heightCm(), img2.heightCm())) {
		ss << "Sizes are different. "
		   << "(" << img1.widthCm() << ", " << img1.heightCm() << "), "
		   << "(" << img2.widthCm() << ", " << img2.heightCm() << ")";
	} else if(img1.palette_.empty() || img2.palette_.empty()) {
		ss << "Color palette is empty";
	}
	if(!ss.str().empty()) throw std::invalid_argument(ss.str());

	BitmapImage image;
	image.widthCm_ = img1.widthCm_;
	image.heightCm_ = img1.heightCm_;;
	image.palette_ = img1.palette_;
	std::vector<PixelArray::pixel_type> priorIndexes;
	for(auto &priRegName: priorRegionNames) {
		priorIndexes.emplace_back(image.palette_.getIndexByCellName(priRegName));
	}
	PixelArray::pixel_type conflictedIndex = image.palette_.getIndexByCellName(conflictedRegionName);
//	std::vector<PixelArray::pixel_type> priorIndexes{image.palette_.getIndexByCellName(geom::Cell::UBOUND_CELL_NAME),
//													  image.palette_.getIndexByCellName(geom::Cell::BOUND_CELL_NAME)};
//	PixelArray::pixel_type conflictedIndex = image.palette_.getIndexByCellName(geom::Cell::DOUBLE_CELL_NAME);

	image.pixelArray_ = PixelArray::merge(img1.pixelArray_, img2.pixelArray_, priorIndexes, conflictedIndex);
	return image;
}

// 「"800 800 14 1",  "  s *M_undef*    c none",」のようなstd文字列から 文字列配列を作成する。
void img::BitmapImage::XpmStrToXpmData(const std::string &sourceStr, char **&arr)
{
	std::vector<std::string> strVec = utils::splitString(",", sourceStr, true);
	for(auto &str: strVec) {
		str = utils::dequote('"', str);
	}

	for(size_t i = 0; i < strVec.size(); ++i) {
		*(arr+i) = new char[strVec.at(i).size()+1];
		std::strcpy(*(arr+i), strVec.at(i).c_str());
	}
}









void img::BitmapImage::drawSquareMark(int xindex, int yindex, int sz, const Color &color, bool filled)
{
	if(sz == 0 || !checkInside(xindex, yindex, "")) return;
	static int numSquareMarks = 0;

	/*
	 * マークごとに個別にパレットへ色登録していたら点の数だけエントリが増えて、xpmファイルへ
	 * エクスポートするときに画素文字を使い果たしてしまう可能性がでてしまう。
	 * そこで同色のマークはまとめてしまう。
	 */
	int colorIndex = palette_.getIndexByColor(color);
	if(colorIndex == CellColorPalette::NOT_INDEX) {
		std::string markName = std::string("squareMark") + std::to_string(numSquareMarks);
		palette_.registerColor(markName, markName, color);
		++numSquareMarks;
		colorIndex = palette_.getIndexByColor(color);
	}

	int xmin_index = std::max(0, xindex - sz);
	int ymin_index = std::max(0, yindex - sz);
	int xmax_index = std::min(static_cast<int>(pixelArray_.horizontalSize()), xindex + sz);
	int ymax_index = std::min(static_cast<int>(pixelArray_.verticalSize()), yindex + sz);
	if(filled) {
		for(int i = xmin_index; i < xmax_index; ++i) {
			for(int j = ymin_index; j < ymax_index; ++j) {
				pixelArray_(i, j) = colorIndex;
			}
		}
	} else {
		for(int i = xmin_index; i < xmax_index; ++i) {
			for(int j = ymin_index; j < ymax_index; ++j) {
				if(i == xmin_index || i == xmax_index - 1 || j == ymin_index || j == ymax_index - 1)
					pixelArray_(i, j) = colorIndex;
			}
		}
	}
}

void img::BitmapImage::drawCrossMark(int xindex, int yindex, int sz, const Color &color)
{
	if(sz == 0) return;
	static int numCrossMarks = 0;
	/*
	 * crossmarkもパレットに同色がない場合のみパレットに追加する。
	 */
	int colorIndex = palette_.getIndexByColor(color);
	if(colorIndex == CellColorPalette::NOT_INDEX) {
		std::string markName = std::string("crossMark") + std::to_string(numCrossMarks);
		palette_.registerColor(markName, markName, color);
		numCrossMarks++;
		colorIndex = palette_.getIndexByColor(color);
	}



    // NOTE roundで丸めるべきか?
    const int lw = static_cast<int>(sz*0.2);
	// 横棒
	const int xmin_index = std::max(0, xindex - sz);
	const int xmax_index = std::min(xindex + sz , static_cast<int>(pixelArray_.horizontalSize())-1);
	for(int i = xmin_index; i < xmax_index; ++i) {
		for(int j = -lw; j <= lw; ++j){
			int yid = yindex + j;
			if(yid > 0&& yid < static_cast<int>(pixelArray_.verticalSize())-1)
				pixelArray_(i, yid) = colorIndex;
		}
	}
	// 縦棒
	const int ymin_index = std::max(0, yindex - sz);
	const int ymax_index = std::min(yindex + sz, static_cast<int>(pixelArray_.verticalSize())-1);
	for(int j = ymin_index; j < ymax_index; j++) {
		for(int i = -lw; i <= lw; ++i) {
			int xid = xindex + i;
			if(xid > 0 && xid < static_cast<int>(pixelArray_.horizontalSize())-1)
				pixelArray_(xid, j) = colorIndex;
		}
	}

}

void img::BitmapImage::drawSquareCrossMark(int xindex, int yindex, int sz, const Color &color) {
	drawCrossMark(xindex, yindex, sz, color);
	drawSquareMark(xindex, yindex, sz, color, false);
}


bool img::BitmapImage::checkInside(int xpix, int ypix, const std::string &errorMessage)
{
	if(xpix < 0 || xpix > static_cast<int>(hResolution()) || ypix < 0 || ypix > static_cast<int>(vResolution())) {
//		std::cerr << "Warning: (" << xpix << ", " << ypix << ") is out of image("
//				 << hResolution() << ", " << vResolution() << ")." << std::endl;
		if(!errorMessage.empty()) std::cerr << errorMessage << std::endl;
		return false;
	} else {
		return true;
	}
}


void img::BitmapImage::setSizeCm(double wid, double hei) {
	widthCm_ = wid;
	heightCm_ = hei;
}

void img::BitmapImage::setWidthCm(const double &xwidth) {
	widthCm_ = xwidth;
	heightCm_ = vResolution()* widthCm_/hResolution();
}

void img::BitmapImage::setHeightCm(const double &ywidth) {
	heightCm_ = ywidth;
	widthCm_ = hResolution() * heightCm_/vResolution();
}




