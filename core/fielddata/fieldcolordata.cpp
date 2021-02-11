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
#include "fieldcolordata.hpp"

#include <sstream>
#include "xyzmeshtallydata.hpp"

fd::FieldColorData::FieldColorData(const std::shared_ptr<const fd::XyzMeshTallyData> &meshData, bool isLog, bool ip)
	: meshData_(meshData), isLog_(isLog), interpolation_(ip)
//    : meshData_(meshData), range_(range), isLog_(isLog), interpolation_(ip)
{}


std::string fd::FieldColorData::toString() const
{
	std::stringstream ss;
	ss << "FieldColorData: mesh =\n" << meshData_->info();
	return ss.str();
}
