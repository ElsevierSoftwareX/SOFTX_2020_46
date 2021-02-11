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
#include "tracingraydata.hpp"

#include <iostream>

#include "core/utils/message.hpp"
#include "core/math/constants.hpp"



// Traceしたデータを元にpos位置の幅pixWidxのピクセルのデータ名を決定する。
img::TracingRayData::TracingRayData(const math::Point &startPos,
                                    const size_t &i,
                                    const std::vector<std::string> &cells,
                                    const std::vector<double> &tlengths,
                                    const std::string &undefRegName,
                                    const std::string &undefBoundRegName,
                                    const std::string &boundRegName)
    :startPos_(startPos), index_(i), undefinedRegionName_(undefRegName),
      undefinedBoundRegionName_(undefBoundRegName), boundRegionName_(boundRegName)
{
	//assert(cells.size() == tlengths.size());
	if(cells.size() != tlengths.size()) {
		// FXIME何故かここに入ってかつ下のthrowが無視されることがある。さらにその時以後plotterがコマンドを受け付けなくなる。上でcatchして無視しているっぽい
		mDebug() << "i=" << index_;
		mDebug() << "cells=" << cells;
		mDebug() << "tlengths=" << tlengths;
		abort();
		throw std::invalid_argument("TracingRayData() size of cells and tlengths are different");
	}
	assert(!cells.empty());

	// 同じcellBoundPositionをまたいで同じセルがあった場合一つにまとめる。
	cellNames_.push_back(cells.at(0));
	lengths_.push_back(tlengths.at(0));
	for(size_t i = 1; i < cells.size(); ++i) {
		if(i == 0 || cells.at(i) != cellNames_.back()) {
		// 最初の要素か、前回追加したセル名とcells.at(i)が同じ場合はデータを追加。
			cellNames_.push_back(cells.at(i));
			lengths_.push_back(tlengths.at(i));
		} else {
		// 同じセルが連続している場合 track lengthを加算して要素は増やさない。
			lengths_.back() += tlengths.at(i);
		}
	}

	// オブジェクト構築時にセル境界位置を絶対座標化しておく。
	double pos = 0;
	for(size_t i = 0; i < lengths_.size(); ++i) {
		pos +=lengths_.at(i);
		cellBoundPositions_.push_back(pos);
	}
}



// 境界画素(境界ピクセル)はここのルーチンで生成される。
std::string img::TracingRayData::getCellName(const double &pos, const double &pixWidth) const
{
	// コンストラクタでまとめたので、同名セルが境界をまたいで連続することはない。
	//mDebug() << "pos=" << pos << "pwid=" << pixWidth;
	// 一番上のcellBoundPositionはセル境界ではなくtrack lengthの終わりなのでcell境界として扱わない。
	for(size_t i = 0; i < cellBoundPositions_.size()-1; ++i) {
		double distance = cellBoundPositions_.at(i) - pos;
		if(std::abs(distance) <= 0.5*pixWidth   // 「セル境界がpixel境界上かpixel内部にあり」
		&& ( (std::abs(distance) != 0.5*pixWidth)  // かつ「セル境界がpixel境界上には無いか
			|| (std::abs(distance) == 0.5*pixWidth && distance > 0) )) { // 上側pixel境界上にある」。

			// ここまで来たらcell境界が最低一つはpixelの内部(かpix上側境界上)にあると言える。
			if(i == cellBoundPositions_.size() - 1) {
				// 一番上のcellBoundPositionの判定では誤差が積もって
				// 本来pixel上側とcell最上側境界がcell内に含まれてしまうケースがある。
				mDebug() << "pos=" << pos << ",Cboundpos=" << cellBoundPositions_.at(i) << "distance=" << distance;
//				return cellNames_.at(i) == geom::Cell::UNDEF_CELL_NAME ? geom::Cell::UBOUND_CELL_NAME : geom::Cell::BOUND_CELL_NAME;
				return cellNames_.at(i) == undefinedRegionName_ ? undefinedBoundRegionName_ : boundRegionName_;
			// 現在セルか次のセルがundefならundefbound。
//			} else if(cellNames_.at(i) == geom::Cell::UNDEF_CELL_NAME
//					|| cellNames_.at(i+1) == geom::Cell::UNDEF_CELL_NAME) {
			} else if(cellNames_.at(i) == undefinedRegionName_
			        || cellNames_.at(i+1) == undefinedRegionName_) {
//					ここで二重定義領域の境界を見つけようとしても無駄。tracingdataに重複領域が出てくることはないから。
//					|| cellNames_.at(i) == geom::Cell::DOUBLE_NAME
//					|| cellNames_.at(i+1) == geom::Cell::DOUBLE_NAME) {
//				return geom::Cell::UBOUND_CELL_NAME;
				return undefinedBoundRegionName_;
			} else {
//				return geom::Cell::BOUND_CELL_NAME;
				return boundRegionName_;

			}

		} else {
			// ここに来たらセル境界はpixel内あるいは上側pixel境界にはない
			// pixel上側を超えるcell境界が見つかればそのcell名を返す。
			if(cellBoundPositions_.at(i) > pos + 0.5*pixWidth) {
				return cellNames_.at(i);
			}
		}
	}
	// ここまででデータが見つからなければ最後のcellを適用する。
	// pixelがtrack lengthからはみ出ていた場合警告する。
	if(pos > cellBoundPositions_.back()) {
		std::cerr << "Warning: x=" << pos + 0.5*pixWidth << "is out of track length(="
				  << cellBoundPositions_.back() << "). last cell data was used." << std::endl;
		//mDebug() << pos + 0.5*pixWidth - cellBoundPositions_.back();
	}
	return cellNames_.back();
}

const std::string img::TracingRayData::toString() const
{
	std::stringstream ss;
	ss << "index=" << index_ << ", cells=";
	for(size_t i = 0; i < cellNames_.size(); ++i) {
		if(i != 0) ss << " ";
		ss << cellNames_.at(i);
	}
	ss << ", lengths=";
	for(size_t i = 0; i < lengths_.size(); ++i) {
		if(i != 0) ss << " ";
		ss << lengths_.at(i);
	}
	return ss.str();
}
