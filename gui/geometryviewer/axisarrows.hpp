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
#ifndef AXISARROWS_HPP
#define AXISARROWS_HPP

#include <array>
#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include "../../core/math/nmatrix.hpp"

class AxisArrows
{
public:
	AxisArrows();
	// radius, length はスケーリングなし状態のarrowに対する設定値
	void setTipLength(double length) {tipLength_ = length;}
	void setTipRadius(double radius) {tipRadius_ = radius;}
	void setShaftRadius(double radius) {shaftRadius_ = radius;}
	// xmin, xmax, ymin, ymax, zmin,zmaxでarrowをフィットさせる領域を与える
	void setRange(const std::array<double,6> ranges);
	void clear();
	std::array<vtkSmartPointer<vtkActor>, 3> actors();

private:
	template <class T>
	using Arr3 = std::array<T, 3>;


	Arr3<vtkSmartPointer<vtkActor>> actors_;
	Arr3<vtkSmartPointer<vtkArrowSource>> arrows_;
	Arr3<vtkSmartPointer<vtkTransform>> transforms_;
	Arr3<vtkSmartPointer<vtkTransformPolyDataFilter>> transformPDs_;
	Arr3<vtkSmartPointer<vtkPolyDataMapper>> mappers_;
    std::array<math::Matrix<4>, 3> matrices_;
	// デフォルトのvtkArrowSource(大きさ1)からどれだけ拡大しているか。
	// この値に応じてtipRadiusなどを調節しないと巨大な矢印になってしまう。
	std::array<double, 3> scalingFactors_;
	math::Point origin_;
	double tipLength_;
	double tipRadius_;
	double shaftRadius_;





	vtkSmartPointer<vtkArrowSource> arrowSourceX_;
	vtkSmartPointer<vtkMatrix4x4> matrixX_ ;
	vtkSmartPointer<vtkTransform> transformX_;
	vtkSmartPointer<vtkTransformPolyDataFilter> transformPDX_;
	vtkSmartPointer<vtkPolyDataMapper> mapperX_;
	vtkSmartPointer<vtkActor>actorX_;
};

#endif // AXISARROWS_HPP
