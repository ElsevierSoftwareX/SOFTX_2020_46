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
#ifndef FIELDCOLORDATA_HPP
#define FIELDCOLORDATA_HPP

#include <memory>
#include <string>
#include <utility>

namespace fd{

class XyzMeshTallyData;

struct FieldColorData{
public:
	FieldColorData(const std::shared_ptr<const fd::XyzMeshTallyData> &meshData,
				   bool isLog, bool ip);
	std::string toString() const;

	std::shared_ptr<const fd::XyzMeshTallyData> meshData_;
	bool isLog_;
	bool interpolation_;
	static constexpr double SMALL_FIELD_DATA = 1e-30;
};

}
#endif // FIELDCOLORDATA_HPP
