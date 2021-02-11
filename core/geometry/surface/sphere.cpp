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
#include "sphere.hpp"

#include <cmath>
#include <sstream>

#include "plane.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/numeric_utils.hpp"


geom::Sphere::Sphere(const std::string &name, const math::Point &center, const double &radius):
    Surface("SPHERE", name), center_(center), radius_(radius)
{
    assert(radius_ > 0);
    boundingPlaneVectors_ = boundingPlanes();
}

std::shared_ptr<geom::Surface> geom::Sphere::makeDeepCopy(const std::string &newName) const
{
    return std::make_shared<Sphere>(newName, center_, radius_);
}



std::string geom::Sphere::toInputString() const
{
    std::stringstream ss;
    ss << name() << " s " << center_.x() << " "
       << center_.y() << " " << center_.z() << " " << radius_;
    return ss.str();
}

std::unique_ptr<geom::Surface> geom::Sphere::createReverse() const
{
	std::unique_ptr<Surface> reversedSphere(new Sphere(Surface::reverseName(name_), center_, radius_));
	reversedSphere->setID(-1*ID_);
	return reversedSphere;
}



bool geom::Sphere::isForward(const math::Point &point) const
{
	return (!reversed_) ? radius_ - math::distance(point, center_) <= 0
						: radius_ - math::distance(point, center_) > 0;
}

std::string geom::Sphere::toString() const
{
	std::stringstream ss;
	ss << Surface::toString() << ", center = " << center_ << ", radius = " << radius_;
	return ss.str();
}

// 直線と球の交点を返す
math::Point geom::Sphere::getIntersection(const math::Point& point, const math::Vector<3> &direction) const
{
	// pointは球の内部とは限らない
	//mDebug() << "############## In getInttersection, point=" << point << "dir=" << direction;
	math::Vector<3> dir = direction.normalized();	// 方向は単位ベクトルであることが前提。
	math::Vector<3> s = point - center_;
	double d_dot_s = math::dotProd(dir,s);
	// 判別式
	double discriminan = d_dot_s*d_dot_s - math::dotProd(s,s) + radius_*radius_;
	//mDebug() << "距離= " << std::sqrt(std::abs(d_dot_s*d_dot_s - math::dotProd(s,s)));
	if(discriminan < math::Point::eps()) return math::Point::INVALID_VECTOR();
	/*
	 * ここでプラスの解は方向ベクトルの先側の解なので、
	 * ・pointが球内の場合は+の解を、
	 * ・pointが球外の場合は-の解を採用する
	 * ・また、球と線が接する場合は交差なしとして扱う
	 */


	// プラスの解は		point + (-d_dot_s + std::sqrt(discriminan))*direction;
	// マイナスの解は	point + (-d_dot_s - std::sqrt(discriminan))*direction;

	double plusCoef = (-d_dot_s + std::sqrt(discriminan));
	double minusCoef = (-d_dot_s - std::sqrt(discriminan));

//	mDebug() << "プラスの解=" << point + plusCoef*dir << ", coef=" << plusCoef;
//	mDebug() << "マイナスの解=" << point + minusCoef*dir << ", coef=" << minusCoef;

	// 係数が正→その解がdir前方にある。
	// 計数が負→その解がdir後方にある
	// 計数が0→その解の上にいる

	// 点が解の上にある場合その点を交点として返す。
    if(utils::isSameDouble(plusCoef, 0)) {
//		mDebug() << "解は" << point + plusCoef*dir;
		return point + plusCoef*dir;
    } else if (utils::isSameDouble(minusCoef, 0)) {
//		mDebug() << "解は" << point + minusCoef*dir;
		return point + minusCoef*dir;
	}

	if(plusCoef > 0) {
		if(minusCoef > 0){
			// ＋-両方の解が前方 ＝ 近い方を返す。
			return (plusCoef > minusCoef) ? point + minusCoef*dir : point + plusCoef*dir;
		} else {
			// +の解が前方、-の解が後方 = プラス解採用
			return point + plusCoef*dir;
		}
	} else {
		if(minusCoef > 0){
			// +の解が後方、-の解が前方 ＝ マイナス解採用
			return point + minusCoef*dir;
		} else {
			// +-両方の解が後方 ＝ 交点なし
			return math::Point::INVALID_VECTOR();
		}
	}
}

std::vector<std::vector<geom::Plane> > geom::Sphere::boundingPlanes() const
{
	using Vec = math::Vector<3>;
	std::vector<std::vector<Plane>> planeVectors;

	if(reversed_) {
		// 球面の法線が内向きの場合 面はAND連結とするので1つのvectorにまとめて追加
		std::vector<Plane> planes;
        planes.emplace_back("", Vec{ 1,  0,  0},   center_.x() - radius_, false);
        planes.emplace_back("", Vec{-1,  0,  0}, -(center_.x() + radius_), false);
        planes.emplace_back("", Vec{ 0,  1,  0},   center_.y() - radius_, false);
        planes.emplace_back("", Vec{ 0, -1,  0}, -(center_.y() + radius_), false);
        planes.emplace_back("", Vec{ 0,  0,  1},   center_.z() - radius_, false);
        planes.emplace_back("", Vec{ 0,  0, -1}, -(center_.z() + radius_), false);
		planeVectors.emplace_back(std::move(planes));
	} else {
		double f = 0.5*std::sqrt(2);
		// 外向きの場合1面が1要素ベクトルとして追加していく。BBには内接面を使う。
        planeVectors.emplace_back(std::vector<Plane>{Plane("", Vec{ 1, 0, 0},    center_.x() + f*radius_, false)});
        planeVectors.emplace_back(std::vector<Plane>{Plane("", Vec{-1, 0, 0}, -(center_.x() - f*radius_), false)});
        planeVectors.emplace_back(std::vector<Plane>{Plane("", Vec{ 0, 1, 0},    center_.y() + f*radius_, false)});
        planeVectors.emplace_back(std::vector<Plane>{Plane("", Vec{ 0,-1, 0}, -(center_.y() - f*radius_), false)});
        planeVectors.emplace_back(std::vector<Plane>{Plane("", Vec{ 0, 0, 1},    center_.z() + f*radius_, false)});
        planeVectors.emplace_back(std::vector<Plane>{Plane("", Vec{ 0, 0,-1}, -(center_.z() - f*radius_), false)});
	}
    return planeVectors;
}




void geom::Sphere::transform(const math::Matrix<4> &matrix)
{
    Surface::transform(matrix);
//	if(matrix != math::Matrix<4>::ZERO()) {
	if(!math::isSameMatrix(matrix, math::Matrix<4>::ZERO())) {
		math::affineTransform<3>(&center_, matrix);
	}
}

std::unique_ptr<geom::Sphere> geom::Sphere::createSphere(const std::string &name,
														  const std::vector<double> &params,
														  const math::Matrix<4> &trMatrix,
														  geom::Sphere::TYPE type,
														 bool warnPhitsCompat)
{
	(void) warnPhitsCompat;
	math::Point center;
	double radius = -1;

	switch(type) {
	case TYPE::S:
		Surface::CheckParamSize(4, "S", params);
		center = math::Point{params.at(0), params.at(1), params.at(2)};
		radius = params.at(3);
		break;
	case TYPE::SO:
		Surface::CheckParamSize(1, "SO", params);
		center = math::Point{0, 0, 0};
		radius = params.at(0);
		break;
	case TYPE::SX:
		Surface::CheckParamSize(2, "SX", params);
		center = math::Point{params.at(0), 0, 0};
		radius = params.at(1);
		break;
	case TYPE::SY:
		Surface::CheckParamSize(2, "SY", params);
		center = math::Point{0, params.at(0), 0};
		radius = params.at(1);
		break;
	case TYPE::SZ:
		Surface::CheckParamSize(2, "SZ", params);
		center = math::Point{0, 0, params.at(0)};
		radius = params.at(1);
		break;
//	default:
//		abort();
//		break;
	}
	std::unique_ptr<Sphere> sphere(new Sphere(name, center, radius));
	sphere->transform(trMatrix);
	return sphere;
}

std::ostream &geom::operator <<(std::ostream &os, const geom::Sphere &sph)
{
	os << sph.toString();
	return os;
}


geom::BoundingBox geom::Sphere::generateBoundingBox() const
{
//	mDebug() << "\nCreating BoundingBox for sphere =" << name();
//	mDebug() << "center=" << center_ << "rad=" << radius_<< "corner=" << center_ - radius_*math::Vector<3>{1, 1, 1};

	if(reversed_) {
		return BoundingBox(center_.x() - radius_, center_.x() + radius_,
						   center_.y() - radius_, center_.y() + radius_,
						   center_.z() - radius_, center_.z() + radius_);
	} else {
		return BoundingBox::universalBox();
	}
}

#ifdef ENABLE_GUI
#include <vtkSphere.h>
#include <vtkImplicitBoolean.h>
#include <vtkPlane.h>
vtkSmartPointer<vtkImplicitFunction> geom::Sphere::generateImplicitFunction() const
{
	vtkSmartPointer<vtkSphere> sphere = vtkSmartPointer<vtkSphere>::New();
	sphere->SetCenter(center_.x(), center_.y(), center_.z());
	sphere->SetRadius(radius_); // DEBUG for debug

	if(reversed_) {
		return sphere;
	} else {
		return getVtkCompliment(sphere);
	}

//	if(reversed_) {
//		return sphere;
//	} else{
//		// 表面の場合はvtkでは陰関数が逆になるので大体積から引いて反転させる
//		vtkSmartPointer<vtkImplicitBoolean> bop = vtkSmartPointer<vtkImplicitBoolean>::New();
//		vtkSmartPointer<vtkSphere> sphere2 = vtkSmartPointer<vtkSphere>::New();
//		sphere2->SetCenter(center_.x(), center_.y(), center_.z());
//		sphere2->SetRadius(LARGE_DOUBLE);

//		bop->AddFunction(sphere2);
//		bop->AddFunction(sphere);
//		bop->SetOperationTypeToDifference();
//		return bop;
//	}
}
#endif
