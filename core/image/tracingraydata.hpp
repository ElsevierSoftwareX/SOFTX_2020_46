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
#ifndef TRACINGRAYDATA_HPP
#define TRACINGRAYDATA_HPP

#include <cassert>
#include <string>
#include <vector>

#include "core/math/nvector.hpp"


namespace img {

class TracingRayData {
public:
	TracingRayData(){;}
	TracingRayData(const math::Point &startPos,
					const size_t &i,
				   const std::vector<std::string> &cells,
	               const std::vector<double> &tlengths,
	               const std::string &undefRegName,
	               const std::string &undefBoundRegName,
	               const std::string &boundRegName);

	// pos(cm)の位置のcell名を返す posは走査開始点を0cmとした座標系。
	std::string getCellName(const double &pos, const double &pixWidth) const;
	const std::vector<double> &cellBoundPositions() const {return cellBoundPositions_;}
	const std::vector<std::string> &cellNames() const {return cellNames_;}
	const math::Point start() const {return startPos_;}
	const std::string toString() const;

private:
	math::Point startPos_;
	size_t index_;  // 何番目のデータか保持している。(PEND本来外部で持つべきか。)
	std::vector<std::string> cellNames_;
	std::vector<double> lengths_;
	std::vector<double> cellBoundPositions_; // Ray内でのcell境界の位置。始点が0
	// 境界、未定義境界の判定をするために以下の領域名が必要
	std::string undefinedRegionName_;  // 未定義領域の領域名(セル名)
	std::string undefinedBoundRegionName_;  // 未定義領域境界の領域名
	std::string boundRegionName_;  // 通常境界の領域名
};


}  // namespace img
#endif // TRACINGRAYDATA_HPP
