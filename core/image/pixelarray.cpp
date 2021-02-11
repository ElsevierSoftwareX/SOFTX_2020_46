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
#include "pixelarray.hpp"

#include <cassert>
#include "core/utils/utils.hpp"
#include "color.hpp"
#include "pixelmergingworker.hpp"
#include "tracingraydata.hpp"

// pArray(row,l col)の周囲widピクセルをpatternで塗りつぶす
void FillSurroundings(img::PixelArray::pixel_type pattern, size_t row, size_t col, size_t wid, img::PixelArray *pArray)
{
	assert(row < pArray->verticalSize() && col < pArray->horizontalSize());
	/*
	 * row, col がpArrayの端だった時が面倒
	 * row < wid,  row > pArray->vSize()-1 - wid の場合。
	 * col < wid,  col > pArray->hSize()-1 - wid の場合。
	 */
	size_t ylower_bound = (row < wid) ? 0 : row - wid;  // 気をつけないとrow-widはoverflowする
	size_t xlower_bound = (col < wid) ? 0 : col - wid;

	for(size_t yindex = ylower_bound; yindex < std::min(row+wid, pArray->verticalSize()-1); ++yindex) {
		for(size_t xindex = xlower_bound ; xindex < std::min(col+wid, pArray->horizontalSize()-1); ++xindex) {
			(*pArray)(xindex, yindex) = pattern;
		}
	}


}

// targetPatternに該当する画素は周囲expandWidth分の画素も同じパターンで塗りつぶす
void img::PixelArray::expandPixel(pixel_type targetPattern, int expandWidth)
{
	if (expandWidth == 0) return;
	PixelArray tmpArray = *this;
	// どうせ1msecくらいしかかからないからマルチスレッド意味なし。
	for(size_t yindex = 0; yindex < verticalSize_; ++yindex) {
		for(size_t xindex = 0; xindex < horizontalSize_; ++xindex) {
			// ターゲットを探す。
			if(this->operator()(xindex, yindex) == targetPattern)
				FillSurroundings(targetPattern, yindex, xindex, static_cast<size_t>(expandWidth), &tmpArray);
		}
	}
	*this = tmpArray;
}


// operator(i, j)はcolumn-majorなので column方向に連結するのは簡単。
img::PixelArray::pixel_type &img::PixelArray::operator()(size_t hindex, size_t vindex) {
	assert(hindex < horizontalSize_);
	assert(vindex < verticalSize_);
	return dataArray_.at(hindex*verticalSize_ + vindex);
}

void img::PixelArray::hMoveConcat(img::PixelArray *arr)
{
    if(arr->horizontalSize() == 0) return;
    horizontalSize_ += arr->horizontalSize_;
    dataArray_.insert(dataArray_.end(),
							std::make_move_iterator(arr->dataArray_.begin()),
							std::make_move_iterator(arr->dataArray_.end()));
    verticalSize_ = dataArray_.size()/horizontalSize_;
}

const img::PixelArray::pixel_type &img::PixelArray::operator()(size_t hindex, size_t vindex) const {
	assert(hindex < horizontalSize_);
	assert(vindex < verticalSize_);
	return dataArray_.at(hindex*verticalSize_ + vindex);
}

std::string img::PixelArray::toXpmString(std::function<char(pixel_type)> pixToXpmCharFunc) const {
	std::stringstream ss;
	for(size_t yindex = 0; yindex < verticalSize_; ++yindex) {
		ss << "\"";
		for(size_t xindex = 0; xindex < horizontalSize_; ++xindex) {
			try {
				ss << pixToXpmCharFunc(this->operator()(xindex, yindex));
			} catch (std::exception &e) {
                (void) e;
                throw std::invalid_argument(std::string("Pixel data  (")
                                            + std::to_string(xindex) + "," + std::to_string(yindex) + ") = "
                                            + std::to_string(this->operator()(xindex, yindex))
                                            + " conversion to xpm char failed.");
			}
		}
		ss << "\"";
		if(yindex != verticalSize_-1) ss << ",";
		ss << "\n";
	}
	return ss.str();
}

img::PixelArray img::PixelArray::renderingFromRayData(img::PixelArray::RayDir dir,
													  size_t hReso, size_t vReso, double xCm, double yCm,
													  const std::vector<img::TracingRayData> &rays,
													  const CellColorPalette &palette)
{
//	mDebug() << "dir=" << ((dir==RayDir::HORIZONTAL)? "horizontal" : "vertical") << "x,yReso=" << xReso << yReso
//			 << ", xyCm=" << xCm << yCm << "raySize =" << rays.size();

	//ray数が足りない場合警告。resolutionはrays.size()に変更される。
	if(dir == RayDir::VERTICAL && rays.size() < hReso) {
		mDebug() << "numrays=" << rays.size() << "reso=" << hReso << vReso;
		std::cerr << "Warning: Number of tracing data is fewer than horizontal resolution.\n"
				  << "y-resolution was set to be " << rays.size() << std::endl;
		vReso = rays.size();
	} else if(dir == RayDir::HORIZONTAL && rays.size() < vReso) {
		mDebug() << "numrays=" << rays.size() << "reso=" << hReso << vReso;
		std::cerr << "Warning: Number of tracing data is fewer than vertical resolution.\n"
				  << "x-resolution was set to be " << rays.size() << std::endl;
		vReso = rays.size();
	}

	PixelArray pArray(hReso, vReso);

	// ここからデータ

	const double pixLengthCm = (RayDir::HORIZONTAL == dir) ? xCm/hReso : yCm/vReso;
	double pixelPos = 0;

	// 2d描画で並列化するならここ。r=5000で1.5秒→1秒くらいの差。しかし単なるomp parallel forでは結果がバグる
	// palleteでcell名からindexを取得
	// ・#pragma omp parallel for
	for(size_t yindex = 0; yindex < vReso; ++yindex) {
		for(size_t xindex = 0; xindex < hReso; ++xindex){
			if(RayDir::HORIZONTAL == dir) {
				pixelPos = xindex*pixLengthCm + 0.5*pixLengthCm;  // 画素の中心位置(cm)
				// horizontaRayの並び順（原点側が先頭）とpixellArrayのｙ並び順（原点が上）は逆なので反転させる
				std::string cellname = rays.at(rays.size() - 1 - yindex).getCellName(pixelPos, pixLengthCm);
				//mDebug() << "centerpos=" << pixelPos << "cell=" << cellname << "pixelSize=" << pixLengthCm;
				pArray(xindex, yindex) =  palette.getIndexByCellName(cellname);
				//mDebug() << "cellname=" << cellname << "pixel=" << palette.getIndexByCellName(cellname);

			} else {
				pixelPos = yindex*pixLengthCm + 0.5*pixLengthCm;  // 画素の中心位置(cm)
				std::string cellname = rays.at(xindex).getCellName(pixelPos, pixLengthCm);
				// pixelArrayのy原点は上。で下向きが正なので反転させる。
				pArray(xindex, vReso - 1 - yindex) = palette.getIndexByCellName(cellname);
				//mDebug() << "cellname=" << cellname << "pixel=" << palette.getIndexByCellName(cellname);
			}
		}
	}
	return pArray;
}





// 2方向からのpixelデータをマージする。
img::PixelArray img::PixelArray::merge(const img::PixelArray &parray1, const img::PixelArray &parray2,
									   const std::vector<pixel_type> &priorPattern,
									   pixel_type conflicted)
{
	// 優先度はundef境界、通常境界、未定義領域、第一引数通常データの順。
	// サイズチェック
	if(parray1.horizontalSize() != parray2.horizontalSize()) {
		std::cerr << "ProgramErro: Horizontal numbers of pixels are different, arg1="
				  << parray1.horizontalSize() << ", arg2=" << parray2.horizontalSize();
		std::exit(EXIT_FAILURE);
	} else if(parray1.verticalSize() != parray2.verticalSize()) {
		std::cerr << "ProgramErro: Vertical numbers of pixels are different, arg1="
				  << parray1.verticalSize() << ", arg2=" << parray2.verticalSize();
		std::exit(EXIT_FAILURE);
	}

//	// thread数の指定
//	auto info = PixelMergingWorker::info();
//	const int NTHREADS = 2;
//	info.numThreads = NTHREADS;
//	info.numTargets =parray1.verticalSize()*parray1.horizontalSize();
//	PixelArray retArray = ProceedOperation<PixelMergingWorker>(info, parray1, parray2, priorPattern, conflicted);

  // シングルスレッド版。そんなに変わらないからマルチスレッド無駄っぽい
	auto hsize = parray1.horizontalSize(), vsize = parray1.verticalSize();
	pixel_type val1, val2;
	PixelArray retArray = parray1;
	for(size_t vindex = 0; vindex < vsize; ++vindex) {
		for(size_t hindex = 0; hindex < hsize; ++hindex) {
			//mDebug() << "arraysize=" << parray1.horizontalSize() << parray1.verticalSize() << "i,j=" <<i << j;
			val1 = parray1(hindex, vindex);
			val2 = parray2(hindex, vindex);
			/*
			 * 領域データが矛盾する→未定義領域
			 * 境界データと領域データが食い違う → 境界を優先
			 * 領域データと領域データが食い違う → 未定義セル
			 */
			if(val1 != val2) {
				bool isPriorPattern = false;
				// priorPattern内では先頭に近いほうが優先されるのでループは後ろから回す。
				for(auto it = priorPattern.rbegin(); it != priorPattern.rend(); ++it) {
					if(val1 == *it || val2 == *it) {
						retArray(hindex, vindex) = *it;
						isPriorPattern = true;
						break;
					}
				}
				if(!isPriorPattern)  {
					retArray(hindex, vindex) = conflicted;
				}
			}
		}
	}
	return retArray;
}

img::PixelArray img::PixelArray::hConcat(const img::PixelArray &parray1, const img::PixelArray &parray2)
{
	if(parray1.verticalSize_ != parray2.verticalSize_) {
		throw std::invalid_argument("Array vertical sizes are different");
	}
	/*
	 * pixelデータはverticalインデックスの変化に対して連続、v方向連続なので
	 * 水平方向に連結する場合は単にarrayをつなげれば良い。
	 */
	PixelArray parray(parray1.horizontalSize_ + parray2.horizontalSize_, parray1.verticalSize_);
	// この時点で領域は確保されているのでreserveやemplace_backは不要
	for(size_t i = 0; i < parray1.dataArray_.size(); ++i) {
		parray.dataArray_[i] = parray1.dataArray_[i];
	}
	size_t a1size = parray1.dataArray_.size();
	for(size_t i = 0; i < parray2.dataArray_.size(); ++i) {
		parray.dataArray_[i+a1size] = parray2.dataArray_[i];
	}
	return parray;
}

img::PixelArray img::PixelArray::vConcat(const img::PixelArray &parray1, const img::PixelArray &parray2)
{
	if(parray1.horizontalSize_ != parray2.horizontalSize_) {
		throw std::invalid_argument("Array horizontal sizes are different");
	}
	PixelArray parray(parray1.horizontalSize_, parray1.verticalSize_ + parray2.verticalSize_);
	for(size_t i = 0; i < parray.horizontalSize_; ++i) {
		for(size_t j = 0; j < parray.verticalSize_; ++j) {
			if(j < parray1.verticalSize_) {
				parray(i, j) = parray1(i,j);
			} else {
				parray(i, j) = parray2(i, j-parray1.verticalSize_);
			}
		}
	}
	return parray;
}

img::PixelArray img::PixelArray::hFlip(const img::PixelArray &parray1)
{
	PixelArray retArray = img::PixelArray(parray1.horizontalSize(), parray1.verticalSize());
	size_t hSize = retArray.horizontalSize();
	size_t vSize = retArray.verticalSize();
	for(size_t i = 0; i < hSize; ++i) {
		for(size_t j = 0; j < vSize; ++j) {
			retArray(i, j) = parray1(hSize-1-i, j);
		}
	}
	return retArray;
}

img::PixelArray img::PixelArray::vFlip(const img::PixelArray &parray1)
{
	PixelArray retArray = img::PixelArray(parray1.horizontalSize(), parray1.verticalSize());
	size_t vSize = retArray.verticalSize();
	size_t hSize = retArray.horizontalSize();
	for(size_t i = 0; i < hSize; ++i) {
		for(size_t j = 0; j < vSize; ++j) {
			retArray(i, j) = parray1(i, vSize-1-j);
		}
	}
	return retArray;
}

