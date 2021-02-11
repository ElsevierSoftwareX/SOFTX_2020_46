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
#ifndef GEOMETRYVIEWERCONFIG_HPP
#define GEOMETRYVIEWERCONFIG_HPP

#include <array>
#include <vtkImplicitFunction.h>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include "../../core/geometry/cell/boundingbox.hpp"
#include "../../core/math/nvector.hpp"


struct PlaneInfo{
	PlaneInfo(){;}
	PlaneInfo(bool v, math::Vector<3> n, double p, int c):visible(v), normal(n), pos(p), cutting(c){;}
	bool visible;
	math::Vector<3> normal;
	double pos;
	int cutting; // 0:カットしない、 >0：+側を除去、<0:-側を除去

	std::string toString() const {
		std::stringstream ss;
		ss << "visible=" << visible << ", pos=" << pos << ", normal=" << normal << ", cutting=" << cutting;
		return ss.str();
	}
	void clear() {*this = PlaneInfo();}

	// 補助平面表示(カットしない場合は面を表示する)→これはsettingpaneへ移したから廃止
	//vtkSmartPointer<vtkActor> createActor(const geom::BoundingBox &bb) const;
};

bool operator == (const PlaneInfo &pi1, const PlaneInfo &pi2);


class GeometryViewerConfig
{
friend 	bool operator==(const GeometryViewerConfig &c1, const GeometryViewerConfig &c2);
friend 	bool operator!=(const GeometryViewerConfig &c1, const GeometryViewerConfig &c2);

static constexpr double DEFAULT_NUM_PTS_CELL = 10000;
static constexpr double DEFAULT_SMOOTHING_FACTOR = 0.4;
// DEBUG
static constexpr double DEFAULT_MEMORY_SAFETY_FACTOR = 90;

public:
//	GeometryViewerConfig()
//		:numPoints_(DEFAULT_NUM_PTS_CELL), rate_(0), region_(std::array<double, 6>{0, 0, 0, 0, 0, 0}),
//		  smoothingFactor_(DEFAULT_SMOOTHING_FACTOR)
//	{;}

	GeometryViewerConfig()
		: GeometryViewerConfig(DEFAULT_NUM_PTS_CELL,
							   std::array<double, 6>{0, 0, 0, 0, 0, 0},
							   std::array<PlaneInfo, 3>(),
							   DEFAULT_SMOOTHING_FACTOR,
							   DEFAULT_MEMORY_SAFETY_FACTOR)
	{;}


	GeometryViewerConfig(size_t pts,
						 const std::array<double, 6> region,
						 const std::array<PlaneInfo, 3> &info,
						 double smoothFactor,
						 int memoryFactor)
		:numPoints_(pts), region_(region), xyzPlaneInfo_(info),
		  smoothingFactor_(smoothFactor), memorySafetyFactor_(memoryFactor)
	{;}
	void clear();
	size_t numPoints() const {return numPoints_;}
	const std::array<double, 6> &region() const {return region_;}
	//AuxPlaneInfo planeInfo() const {return planeInfo_;}
	const std::array<PlaneInfo, 3> &xyzPlaneInfo() const {return xyzPlaneInfo_;}
	double smoothingFactor() const {return smoothingFactor_;}
	int memoryUsagePercentLimit() const {return memorySafetyFactor_;}

	// 補助平面以外で変更点がある場合trueを返す。
	bool hasChangedExceptAuxPlanes(const GeometryViewerConfig &gconf) const;
	// カットあり補助平面の変更がある場合trueを返す。
	bool hasChangedCuttingPlanes(const GeometryViewerConfig &gconf) const;
	// 補助平面3個の作る(半無限)BoundingBox
	geom::BoundingBox getAuxPlanesBoundingBox() const;

	// 補助平面で陰関数をカットする
	vtkSmartPointer<vtkImplicitFunction> cutImplicitFunction(vtkSmartPointer<vtkImplicitFunction> iFunc) const;


private:
	// セルあたりのサンプリング点数
	size_t numPoints_;
	// 描画領域(xmin, xmax, ymin, ymax ...
	std::array<double, 6> region_;
	// 補助平面情報の管理auxPlaneinfo_[0]が補助x平面に対応
	std::array<PlaneInfo, 3> xyzPlaneInfo_;
	// 体積の大きいセルはpow(vol, smoothingFactor_)で点数を増やす。
	double smoothingFactor_;
	// メモリ安全係数(0-100）。メモリ確保の限界をメモリ空き容量×この係数%分だけにする。
	int memorySafetyFactor_;
};

bool operator==(const GeometryViewerConfig &c1, const GeometryViewerConfig &c2);
bool operator!=(const GeometryViewerConfig &c1, const GeometryViewerConfig &c2);

#endif // GEOMETRYVIEWERCONFIG_HPP
