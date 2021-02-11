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
#ifndef XSITEMINFO_HPP
#define XSITEMINFO_HPP

#include <string>
#include <QString>
#include "../core/physics/physconstants.hpp"

struct XsItemInfo {
	XsItemInfo(phys::ParticleType pt, const QString &zstr, int m, double f)
		:ptype(pt), zaid(zstr), mt(m), factor(f)
	{;}
	std::string toString() const;
	phys::ParticleType ptype;
	QString zaid;
	int mt;
	double factor;
};
// operator<はfactorを無視した比較
//bool operator<(const XsItemInfo &t1, const XsItemInfo &t2); // FIXMEわかりづらいから消す
bool xsInfoLess(const XsItemInfo &t1, const XsItemInfo &t2);
bool xsInfoLessWithoutFactor(const XsItemInfo &t1, const XsItemInfo &t2);
bool isSameInfo(const XsItemInfo &t1, const XsItemInfo &t2); // factorも含めて同じならtrueを返す。
bool isSameInfoWithoutFactor(const XsItemInfo &t1, const XsItemInfo &t2); // factor以外が同じならtrueを返す。



#endif // XSITEMINFO_HPP
