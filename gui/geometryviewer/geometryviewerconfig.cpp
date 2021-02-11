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
#include "geometryviewerconfig.hpp"

#include <vtkImplicitBoolean.h>
#include <vtkPlane.h>


#include "../../core/utils/numeric_utils.hpp"
#include "../../core/utils/message.hpp"

namespace {

}  // end anonymous namespace



bool operator == (const PlaneInfo &pi1, const PlaneInfo &pi2)
{
	if(pi1.visible != pi2.visible) return false;
	if(!math::isSamePoint(pi1.normal, pi2.normal)) return false;
	if(std::abs(pi1.pos - pi2.pos) > 0.0001) return false;
	if(pi1.cutting*pi2.cutting < 0) return false;
	if(pi1.cutting*pi2.cutting == 0 && (pi1.cutting != 0 || pi2.cutting != 0)) return false;
	return true;
}

bool operator==(const GeometryViewerConfig &c1, const GeometryViewerConfig &c2)
{
	if(c1.numPoints_ != c2.numPoints_) return false;
	if(!utils::isSameDouble(c1.smoothingFactor_, c2.smoothingFactor_)) return false;

	for(size_t i = 0; i < c1.region_.size(); ++i) {
		if(!utils::isSameDouble(c1.region_.at(i), c2.region_.at(i))) return false;
	}

	if(c1.xyzPlaneInfo_ != c2.xyzPlaneInfo_) return false;

	return true;
}

bool operator!=(const GeometryViewerConfig &c1, const GeometryViewerConfig &c2)
{
	return !(c1 == c2);
}

void GeometryViewerConfig::clear()
{
	*this = GeometryViewerConfig();
}

bool GeometryViewerConfig::hasChangedExceptAuxPlanes(const GeometryViewerConfig &gconf) const
{
	if(numPoints_ != gconf.numPoints_) return true;
	if(!utils::isSameDouble(smoothingFactor_, gconf.smoothingFactor_)) return true;

	for(size_t i = 0; i < region_.size(); ++i) {
		if(!utils::isSameDouble(region_.at(i), gconf.region_.at(i))) return true;
	}
	return  false;
}


bool GeometryViewerConfig::hasChangedCuttingPlanes(const GeometryViewerConfig &gconf) const
{
	// cutありplaneの変更が行われていたらtrueを返す。
	for(size_t i = 0; i < xyzPlaneInfo_.size(); ++i) {
        const PlaneInfo &pinfo1=xyzPlaneInfo_.at(i), &pinfo2 = gconf.xyzPlaneInfo().at(i);
		// (visible, cut) が(false, any) と (any, nocut)は同等と解釈してfalse扱いでcontinue
        if((!pinfo1.visible && pinfo2.cutting == 0) || (!pinfo2.visible && pinfo1.cutting == 0)) continue;

		// それ以外はvisible, cutting, pos比較。但し非カット面に関してはposは比較しない。
		if(pinfo1.cutting == 0 && pinfo2.cutting == 0) {
			if((pinfo1.visible != pinfo2.visible) || (pinfo1.cutting != pinfo2.cutting)) return true;
		} else {
			if((pinfo1.visible != pinfo2.visible)
				|| (pinfo1.cutting != pinfo2.cutting)
				|| (!utils::isSameDouble(pinfo1.pos, pinfo2.pos))) return true;
		}
	}
	return false;
}

geom::BoundingBox GeometryViewerConfig::getAuxPlanesBoundingBox() const
{
	double MAX_EXTENT = geom::BoundingBox::MAX_EXTENT;
	std::array<std::pair<double, double>, 3> regions{
		std::make_pair(-MAX_EXTENT, MAX_EXTENT),
		std::make_pair(-MAX_EXTENT, MAX_EXTENT),
		std::make_pair(-MAX_EXTENT, MAX_EXTENT)
	};
    // 各planeの方向ベクトルは自明でxyzとする。将来的には拡張の可能性はある。
	for(size_t i = 0; i < 3; ++i) {
		if(xyzPlaneInfo_[i].cutting > 0 && xyzPlaneInfo_[i].visible) {
			regions[i].second = xyzPlaneInfo_[i].pos;
		} else if (xyzPlaneInfo_[i].cutting < 0 && xyzPlaneInfo_[i].visible) {
			regions[i].first = xyzPlaneInfo_[i].pos;
		}
	}
	return geom::BoundingBox(
			regions[0].first, regions[0].second,
			regions[1].first, regions[1].second,
			regions[2].first, regions[2].second);
}


vtkSmartPointer<vtkImplicitFunction> GeometryViewerConfig::cutImplicitFunction(vtkSmartPointer<vtkImplicitFunction> iFunc) const
{
	std::vector<vtkSmartPointer<vtkImplicitFunction>> funcs;
	for(size_t i = 0; i < 3; ++i) {
		if(xyzPlaneInfo_[i].cutting != 0 && xyzPlaneInfo_[i].visible) {
			double sign = (xyzPlaneInfo_[i].cutting > 0) ? 1 : -1;
			//mDebug() << "visible=" << xyzPlaneInfo_[i].visible << "cutting=" << xyzPlaneInfo_[i].cutting;
			math::Point center = xyzPlaneInfo_[i].pos*xyzPlaneInfo_[i].normal;
			math::Vector<3> normal = sign*xyzPlaneInfo_[i].normal;
			auto plane = vtkSmartPointer<vtkPlane>::New();
			plane->SetOrigin(center.x(), center.y(), center.z());
			plane->SetNormal(normal.x(), normal.y(), normal.z());
			funcs.emplace_back(plane);
		}
	}
	if(funcs.empty()) {
		return iFunc;
	} else {
		auto bop = vtkSmartPointer<vtkImplicitBoolean>::New();
		for(auto func: funcs) {
			bop->AddFunction(func);
		}
		bop->AddFunction(iFunc);
		bop->SetOperationTypeToIntersection();
		return bop;
	}
}


