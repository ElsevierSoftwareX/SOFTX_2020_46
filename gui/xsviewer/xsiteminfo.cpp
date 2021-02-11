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
#include "xsiteminfo.hpp"

#include <sstream>
#include <cmath>
#include "../../core/utils/message.hpp"
#include "../../core/material/nuclide.hpp"

namespace {
const double FACTOR_EPS = 1e-14;
}

//// factor"だけ"が異なるTargetInfoは同値扱いにする。
//bool operator<(const XsItemInfo &t1, const XsItemInfo &t2)
//{
//	if(t1.ptype != t2.ptype) return static_cast<int>(t1.ptype) < static_cast<int>(t2.ptype);
//	if(t1.zaid != t2.zaid) return t1.zaid < t2.zaid;
//	return t1.mt < t2.mt;
//}

bool isSameInfo(const XsItemInfo &t1, const XsItemInfo &t2)
{
	return isSameInfoWithoutFactor(t1, t2) && std::abs(t1.factor - t2.factor) < FACTOR_EPS;
}

bool isSameInfoWithoutFactor(const XsItemInfo &t1, const XsItemInfo &t2)
{
	return t1.mt == t2.mt && t1.ptype == t2.ptype && t1.zaid == t2.zaid;
}


bool xsInfoLess(const XsItemInfo &t1, const XsItemInfo &t2)
{
	if(t1.ptype != t2.ptype) return static_cast<int>(t1.ptype) < static_cast<int>(t2.ptype);
//	if(t1.zaid != t2.zaid) return t1.zaid < t2.zaid;
	if(t1.zaid != t2.zaid) return mat::Nuclide::ZaidLess(t1.zaid.toStdString(), t2.zaid.toStdString());
	if(t1.mt != t2.mt) return t1.mt < t2.mt;
	return t1.factor < t2.factor;
}

bool xsInfoLessWithoutFactor(const XsItemInfo &t1, const XsItemInfo &t2)
{
	if(t1.ptype != t2.ptype) return static_cast<int>(t1.ptype) < static_cast<int>(t2.ptype);
//	if(t1.zaid != t2.zaid) return t1.zaid < t2.zaid;
	if(t1.zaid != t2.zaid) return mat::Nuclide::ZaidLess(t1.zaid.toStdString(), t2.zaid.toStdString());
	return t1.mt < t2.mt;
}


std::string XsItemInfo::toString() const
{
	std::stringstream ss;
	ss << "incident particle=" << phys::particleTypeTostr(ptype) << ", ZAID=" << zaid << ", MT=" << mt << ", factor=" << factor;
	return ss.str();
}

