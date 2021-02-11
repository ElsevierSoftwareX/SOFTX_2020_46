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
#include "cylinder.hpp"

#include <sstream>
#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surface/sphere.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/math/nmatrix.hpp"

geom::Cylinder::Cylinder(const std::string name, const math::Point &point,
						 const math::Vector<3> &direction, double rad)
    :Surface("CYLINDER", name), refPoint_(point),
      refDirection_(direction.normalized()), radius_(rad)
{
    boundingPlaneVectors_ = boundingPlanes();
}

std::string geom::Cylinder::toInputString() const
{
	//
	std::stringstream ss;
	ss <<  name() << " ca" << " " <<  refPoint_.x() << " " << refPoint_.y() << " " << refPoint_.z()
	   << " "  << refDirection_.x() << " " << refDirection_.y() << " " << refDirection_.z()
	   << " " << radius_;
	return ss.str();
}

std::unique_ptr<geom::Surface> geom::Cylinder::createReverse() const
{
	std::unique_ptr<Surface> reversedCylinder(new Cylinder(Surface::reverseName(name_), refPoint_, refDirection_, radius_));
	reversedCylinder->setID(ID_*-1);
	return reversedCylinder;
}

bool geom::Cylinder::isForward(const math::Point &p) const
{
	math::Vector<3> perpVec = p - refPoint_
							  - math::dotProd((p - refPoint_), refDirection_)*refDirection_;
	return (!reversed_) ? perpVec.abs() >= radius_ : perpVec.abs() < radius_;
}


math::Point geom::Cylinder::getIntersection(const math::Point &p, const math::Vector<3> &d) const
{
	// 円筒とdが平行になった場合 明白に解なし
	if(math::isDependent(d, refDirection_)) {
		return math::Vector<3>::INVALID_VECTOR();
	}
	// まずrefDirection_に垂直でpointを通る面(alphaPlane)への射影を求める
	geom::Plane alphaPlane("", refDirection_, math::dotProd<3>(p, refDirection_));
	math::Point p_a = geom::Plane::projection(alphaPlane, p);
	math::Point p0_a = geom::Plane::projection(alphaPlane, refPoint_);
	math::Vector<3> d_a = geom::Plane::projection(alphaPlane, d + p) - p_a;

	// 円筒とdが平行になった場合を避ければd_aは0ベクトルにならないはず
	// …だが演算精度の限界で0になることもある
	if(d_a.abs() < math::Vector<3>::EPS) {
		return math::Vector<3>::INVALID_VECTOR();
	}

	geom::Sphere sp("", p0_a, radius_);
	auto section_a = sp.getIntersection(p, d_a);

//	mDebug() << "proj plane =" << alphaPlane.toString();
//	mDebug() << "p=" << p << "p_a=" << p_a;
//	mDebug() << "d=" << d << ", d_a =" << d_a;
//	mDebug() << "p0=" << refPoint_ << ", p0_a=" << p0_a;
//	mDebug() << "sphere = " << sp.toString();
//	mDebug() << "projected intersection = " << section_a;

	if(!section_a.isValid()) return math::Vector<3>::INVALID_VECTOR();

	return p + (section_a-p).abs() * 1.0/(math::cosine(d, d_a))*d.normalized();
}

std::string geom::Cylinder::toString() const
{
	std::stringstream ss;
	ss << Surface::toString() << ", ref point = " << refPoint_ << ", ref dir = " << refDirection_
	   << ", radius = " << radius_;
	return ss.str();
}



std::unique_ptr<geom::Cylinder> geom::Cylinder::createCylinder(const std::string &name,
										 const std::vector<double> &params,
										 const math::Matrix<4> &trMatrix,
										 TYPE type, bool warnPhitsCompat)
{
	(void) warnPhitsCompat;
	math::Point refPoint;
	math::Vector<3> refDir;
	double radius;

	switch(type) {
	case TYPE::CX:
		Surface::CheckParamSize(3, "C/X", params);
		refPoint = math::Point{0, params.at(0), params.at(1)};
		refDir = math::Vector<3>{1, 0, 0};
		radius = params.at(2);
		break;
	case TYPE::CY:
		Surface::CheckParamSize(3, "C/Y", params);
		refPoint = math::Point{params.at(0), 0, params.at(1)};
		refDir = math::Vector<3>{0, 1, 0};
		radius = params.at(2);
		break;
	case TYPE::CZ:
		Surface::CheckParamSize(3, "C/Z", params);
		refPoint = math::Point{params.at(0), params.at(1), 0};
		refDir = math::Vector<3>{0, 0, 1};
		radius = params.at(2);
		break;
	case TYPE::CXO:
		Surface::CheckParamSize(1, "CX", params);
		refPoint = math::Point{0, 0, 0};
		refDir = math::Vector<3>{1, 0, 0};
		radius = params.at(0);
		break;
	case TYPE::CYO:
		Surface::CheckParamSize(1, "CY", params);
		refPoint = math::Point{0, 0, 0};
		refDir = math::Vector<3>{0, 1, 0};
		radius = params.at(0);
		break;
	case TYPE::CZO:
		Surface::CheckParamSize(1, "CZ", params);
		refPoint = math::Point{0, 0, 0};
		refDir = math::Vector<3>{0, 0, 1};
		radius = params.at(0);
		break;
	case TYPE::CA:
		Surface::CheckParamSize(7, "CA", params);
		refPoint = math::Point{params.at(0), params.at(1), params.at(2)};
		refDir = math::Vector<3>{params.at(3), params.at(4), params.at(5)};
		radius = params.at(6);
		break;
	default:
		std::cerr << "ProgramError: Invalid Cylinder type." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	std::unique_ptr<Cylinder> cyl(new Cylinder(name, refPoint, refDir, radius));
	cyl->transform(trMatrix);
	return cyl;
}

geom::BoundingBox geom::Cylinder::generateBoundingBox() const
{
	// 円筒外側領域のBBは全空間
	if(!reversed_) return BoundingBox::universalBox();

	// BoundingBoxはAABBなのでaxisが軸方向でなければ無限大にする。
	const double LARGE = BoundingBox::MAX_EXTENT;
	if(math::isDependent(refDirection_, math::Vector<3>{1, 0, 0})) {
		return BoundingBox(-LARGE, LARGE,
						   refPoint_.y()-radius_, refPoint_.y()+radius_,
						   refPoint_.z()-radius_, refPoint_.z()+radius_);
	} else if(math::isDependent(refDirection_, math::Vector<3>{0, 1, 0})) {
		return BoundingBox(refPoint_.x()-radius_, refPoint_.x()+radius_,
							-LARGE, LARGE,
						   refPoint_.z()-radius_, refPoint_.z()+radius_);
	} else if(math::isDependent(refDirection_, math::Vector<3>{0, 0, 1}))  {
		return BoundingBox(refPoint_.x()-radius_, refPoint_.x()+radius_,
						   refPoint_.y()-radius_, refPoint_.y()+radius_,
							-LARGE, LARGE);
	} else {
		return BoundingBox::universalBox();
	}
}

void geom::Cylinder::transform(const math::Matrix<4> &matrix)
{
    Surface::transform(matrix);
//	if(matrix != math::Matrix<4>::ZERO()) {
	if(!math::isSameMatrix(matrix, math::Matrix<4>::ZERO())) {
		math::affineTransform<3>(&refPoint_, matrix);
		refDirection_ = refDirection_*matrix.rotationMatrix();
	}
}

std::vector<std::vector<geom::Plane> > geom::Cylinder::boundingPlanes() const
{
	// 円筒の4側面を返す。方向はrefDirection_, 半径radius_ 軸上の1点 refPoint_
	auto vPair = math::get2OrthogonalUnitVectors(refDirection_);
	auto v1 = vPair.first, v2 = vPair.second;
	// 面の法線はaxis_をv1, v2周りに±90度回転させて作成する。
	double t1 = std::tan(0.5*math::PI);
    double rad05pi = std::acos(std::sqrt(1.0/(1+t1*t1)));
    math::Vector<3> n1p = refDirection_*math::generateRotationMatrix2(-v2, -rad05pi);
    math::Vector<3> n1m = refDirection_*math::generateRotationMatrix2( v2, -rad05pi);
    math::Vector<3> n2p = refDirection_*math::generateRotationMatrix2(-v1, -rad05pi);
    math::Vector<3> n2m = refDirection_*math::generateRotationMatrix2( v1, -rad05pi);


	// 裏面の場合は面をAND連結なので4要素ベクトル1つに、表面はOR連結なので1要素ベクトル4つに
	if(reversed_) {
		std::vector<Plane> ret;
        ret.emplace_back("", n1p, refPoint_ + radius_*(-n1p), false);
        ret.emplace_back("", n1m, refPoint_ + radius_*(-n1m), false);
        ret.emplace_back("", n2p, refPoint_ + radius_*(-n2p), false);
        ret.emplace_back("", n2m, refPoint_ + radius_*(-n2m), false);
		return std::vector<std::vector<Plane>>{std::move(ret)};
	} else {
		double f = 0.5*std::sqrt(2);  // 内接面はr/√2の位置にある。
		return std::vector<std::vector<Plane>> {
           std::vector<Plane>{Plane("", n1p, refPoint_ + f*radius_*(n1p), false)},
           std::vector<Plane>{Plane("", n1m, refPoint_ + f*radius_*(n1m), false)},
           std::vector<Plane>{Plane("", n2p, refPoint_ + f*radius_*(n2p), false)},
           std::vector<Plane>{Plane("", n2m, refPoint_ + f*radius_*(n2m), false)},
		};
    }
}

std::shared_ptr<geom::Surface> geom::Cylinder::makeDeepCopy(const std::string &newName) const
{
    return std::make_shared<Cylinder>(newName, refPoint_, refDirection_, radius_);
}


#ifdef ENABLE_GUI
#include <vtkCylinder.h>
#include <vtkImplicitBoolean.h>
vtkSmartPointer<vtkImplicitFunction> geom::Cylinder::generateImplicitFunction() const
{
	auto cylinder = vtkSmartPointer<vtkCylinder>::New();
	cylinder->SetRadius(radius_);
	cylinder->SetCenter(refPoint_.x(), refPoint_.y(), refPoint_.z());
	cylinder->SetAxis(refDirection_.x(), refDirection_.y(), refDirection_.z());

	if(reversed_) {
		return cylinder;
	} else {
		return Surface::getVtkCompliment(cylinder);
	}

//	// 表面の場合はvtkでは陰関数が逆になるので大体積から引いて反転させる
//	auto bop = vtkSmartPointer<vtkImplicitBoolean>::New();
//	auto cylinder2 = vtkSmartPointer<vtkCylinder>::New();
//	cylinder2->SetCenter(refPoint_.x(), refPoint_.y(), refPoint_.z());
//	cylinder2->SetAxis(refDirection_.x(), refDirection_.y(), refDirection_.z());
//	cylinder2->SetRadius(LARGE_DOUBLE);

//	bop->AddFunction(cylinder2);
//	bop->AddFunction(cylinder);
//	bop->SetOperationTypeToDifference();
//	return bop;
}
#endif









