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
#include "triangle.hpp"
#include <sstream>

#include "plane.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/utils/stream_utils.hpp"
#include "core/utils/numeric_utils.hpp"
#include "core/utils/message.hpp"


// 自分自身もneighborに判定されてしまうので要注意
geom::Triangle::Triangle(const std::string &name, const std::array<std::shared_ptr<math::Point>, 3> &v, double torelance, bool ccw)
    : Surface("TRIANGLE", name), vertices_(v)
{
    if(math::isSamePoint(v[0], v[1], torelance) || math::isSamePoint(v[0], v[2], torelance)) {
        std::stringstream ss;
        ss << "Triangle degenerated, points ={";
        for(size_t i = 0; i < v.size(); ++i) {
            ss << *v[i].get();
            if(i != v.size() - 1) ss << ", ";
        }
        ss << "}";
        throw std::invalid_argument(ss.str());
    }
    std::shared_ptr<math::Vector<3>> tmp1 = v[2] - v[0];
    std::shared_ptr<math::Vector<3>> tmp2 = v[1] - v[0];
    std::shared_ptr<math::Vector<3>> pt = math::crossProd(tmp1, tmp2);
    // ccw: counter clock wise. デフォルトではvtkと同様cwを表にしているが、stl等からデータを読み込む時は逆にする。
    normal_ = ccw ? -1*pt->normalized() : pt->normalized();
    boundingPlaneVectors_ = boundingPlanes();
}

geom::Triangle::Triangle(const std::string &name, const std::array<math::Point, 3> &v, bool ccw)
    :Surface("TRIANGLE", name),
      normal_(math::crossProd(v[2] - v[0], v[1] - v[0]).normalized())
{
    for(size_t i = 0; i < v.size(); ++i) {
        vertices_[i] = std::make_shared<math::Point>(v[i]);
    }
    if(ccw) normal_ = -1*normal_;
    boundingPlaneVectors_ = boundingPlanes();
}

std::shared_ptr<geom::Surface> geom::Triangle::makeDeepCopy(const std::string &newName) const
{
    std::array<math::Point, 3> vertices{*vertices_.at(0).get(), *vertices_.at(1).get(), *vertices_.at(2).get()};
    return std::make_shared<Triangle>(newName, vertices);
}


bool geom::Triangle::isNeighbor(const geom::Triangle &tri)
{
    int count = 0;
    for(size_t i = 0; i < tri.vertices_.size(); ++i) {
        if(this->hasVertex(tri.vertices_[i])) {
            ++count;
            if(count == 2) return true;
        }
    }
    return false;
}

std::string geom::Triangle::toString() const
{
    std::stringstream ss;
    ss << Surface::toString() << ", vertexes = {";
    for(size_t i = 0; i < vertices_.size(); ++i) {
        ss << "{" << vertices_.at(i) << "}";
    }
    ss << "}, normal=" << normal_;
    return ss.str();
}

std::string geom::Triangle::toInputString() const
{
    std::stringstream ss;
    ss << name() << " tri ";
    for(size_t i = 0; i < 3; ++i) {
        ss << vertices_.at(i)->at(0) << " " << vertices_.at(i)->at(1) << " " << vertices_.at(i)->at(2);
        if(i <= 1) ss << "  ";
    }
    return ss.str();
}

std::unique_ptr<geom::Surface> geom::Triangle::createReverse() const
{
    std::unique_ptr<Surface> reversedSurface(new Triangle(Surface::reverseName(name_), vertices_, -1*normal_));
	reversedSurface->setID(-1*ID_);
	mDebug() << "三角形の裏面名は=" << Surface::reverseName(name_) << "IDは" << reversedSurface->getID();
	return reversedSurface;
}

void geom::Triangle::transform(const math::Matrix<4> &matrix)
{
    Surface::transform(matrix);
	normal_ = normal_*matrix.rotationMatrix();
	for(auto &v: vertices_) math::affineTransform(v.get(), matrix);
}

bool geom::Triangle::isForward(const math::Point &point) const
{
	// 平面の式は n1*X + n2*Y + n3Z = d
	double d = math::dotProd(normal_, *vertices_.at(0).get());
	return math::dotProd(point, normal_) - d >= 0;  // 面の直上も表判定
}

// 表裏区別せずに交点を求める
math::Point geom::Triangle::getIntersection(const math::Point &point, const math::Vector<3> &direction) const
{
	return getIntersection2(point, direction).second;
}

std::pair<bool, math::Point> geom::Triangle::getIntersection2(const math::Point &point,
															  const math::Vector<3> &direction) const
{
	// 参考：実例で学ぶゲーム3D数学 p.304
	auto dot = math::dotProd(normal_, direction);
	// 面と直線が平行なら常に交点なし
	if(utils::isSameDouble(dot, 0)) return std::make_pair(false, math::Point::INVALID_VECTOR());

	auto d = math::dotProd(normal_, *vertices_.at(0).get());
	auto t = d - math::dotProd(normal_, point);

	// 交点が前方にあるかは、本来t/dotがノンゼロ正かで判定するが、
	// tとdotが同符号でtが0以上であれば同じなので掛け算でも良い。(t!=0はここまででチェック済みなので)s
	if(t*dot <= 0) return std::make_pair(false, math::Point::INVALID_VECTOR());

	t /= dot;
	assert(t >= 0);

	math::Vector<3> p = point + t*direction;  // 直線の三角形平面への投影点これに内外判定をかける。


	double u0, u1, u2, v0, v1, v2;
	auto nx2 = normal_.x()*normal_.x(), ny2 = normal_.y()*normal_.y(), nz2 = normal_.z()*normal_.z();
	if(nx2 > ny2) {
		if(nx2 > nz2) {
			u0 = p.y() - vertices_[0]->y();
			u1 = vertices_[1]->y() - vertices_[0]->y();
			u2 = vertices_[2]->y() - vertices_[0]->y();
			v0 = p.z() - vertices_[0]->z();
			v1 = vertices_[1]->z() - vertices_[0]->z();
			v2 = vertices_[2]->z() - vertices_[0]->z();
		} else {
			u0 = p.x() - vertices_[0]->x();
			u1 = vertices_[1]->x() - vertices_[0]->x();
			u2 = vertices_[2]->x() - vertices_[0]->x();
			v0 = p.y() - vertices_[0]->y();
			v1 = vertices_[1]->y() - vertices_[0]->y();
			v2 = vertices_[2]->y() - vertices_[0]->y();
		}
	} else {
		if(ny2 > nz2) {
			u0 = p.x() - vertices_[0]->x();
			u1 = vertices_[1]->x() - vertices_[0]->x();
			u2 = vertices_[2]->x() - vertices_[0]->x();
			v0 = p.z() - vertices_[0]->z();
			v1 = vertices_[1]->z() - vertices_[0]->z();
			v2 = vertices_[2]->z() - vertices_[0]->z();
		} else {
			u0 = p.x() - vertices_[0]->x();
			u1 = vertices_[1]->x() - vertices_[0]->x();
			u2 = vertices_[2]->x() - vertices_[0]->x();
			v0 = p.y() - vertices_[0]->y();
			v1 = vertices_[1]->y() - vertices_[0]->y();
			v2 = vertices_[2]->y() - vertices_[0]->y();
		}
	}
	auto temp = u1*v2 - v1*u2;
	// 交点なし。分母が発散する
	if(!(!utils::isSameDouble(temp, 0))) return std::make_pair(false, math::Point::INVALID_VECTOR());
	temp = 1.0/temp;

	// alpha, beta, gammaのどれかが0なら辺の直上に交点が来る
	auto alpha = (u0*v2 - v0*u2)*temp;
	if(!(alpha >= 0)) return std::make_pair(false, math::Point::INVALID_VECTOR());

	auto beta = (u1*v0 - v1*u0)*temp;
	if(!(beta >= 0)) return std::make_pair(false, math::Point::INVALID_VECTOR());

	auto gamma = 1.0 - alpha - beta;
	if(!(gamma >= 0)) return std::make_pair(false, math::Point::INVALID_VECTOR());

	return std::make_pair(utils::isSameDouble(alpha, 0) ||
						  utils::isSameDouble(beta, 0)  ||
						  utils::isSameDouble(gamma, 0), p);
}

std::vector<std::vector<geom::Plane> > geom::Triangle::boundingPlanes() const
{
    return std::vector<std::vector<Plane>> { std::vector<Plane>{Plane("", normal_, center())}};
}


std::unique_ptr<geom::Triangle> geom::Triangle::createTriangle(const std::string &name,
															   const std::vector<double> &params,
															   const math::Matrix<4> &trMatrix,
															   bool warnPhitsCompat)
{
	if(warnPhitsCompat) mWarning() << "Triangle surface is not phits compatible";

    if(params.size() < 9) throw std::invalid_argument("Number of parameters should be 9 for TRI");
	std::array<math::Point, 3> ptArray{
		math::Point{params.at(0), params.at(1), params.at(2)},
		math::Point{params.at(3), params.at(4), params.at(5)},
		math::Point{params.at(6), params.at(7), params.at(8)}
	};
	std::unique_ptr<Triangle> triangle(new Triangle(name, ptArray));
	if(!math::isSameMatrix(trMatrix, math::Matrix<4>()) && !math::isSameMatrix(trMatrix, math::Matrix<4>::IDENTITY())) triangle->transform(trMatrix);
	return triangle;
}

geom::BoundingBox geom::Triangle::generateBoundingBox() const
{
	//mDebug() << "Creating BoundingBox for Plane =" << this->toInputString();
	double LARGE = geom::BoundingBox::MAX_EXTENT;
	math::Vector<3> xvec{1, 0, 0}, yvec{0, 1, 0}, zvec{0, 0, 1};
	double dist = math::dotProd(normal_, *vertices_[0].get());
	if(math::isDependent(normal_, xvec)) {
		// x軸平行なら
		if(math::dotProd(normal(), math::Vector<3>{1, 0, 0}) > 0){  // 法線が+x向き
			return geom::BoundingBox((dist*normal_).x(), LARGE, -LARGE, LARGE,-LARGE, LARGE);
		} else {
			return geom::BoundingBox(-LARGE, (dist*normal_).x(), -LARGE, LARGE,-LARGE, LARGE);
		}
	} else if(math::isDependent(normal_, yvec)) {
		if(math::dotProd(normal(), math::Vector<3>{0, 1, 0}) > 0){
			return geom::BoundingBox(-LARGE, LARGE, (dist*normal_).y(), LARGE,-LARGE, LARGE);
		} else {
			return geom::BoundingBox(-LARGE, LARGE, -LARGE, (dist*normal_).y(), -LARGE, LARGE);
		}
	} else if (math::isDependent(normal_, zvec)) {
		if(math::dotProd(normal(), math::Vector<3>{0, 0, 1}) > 0){
			return geom::BoundingBox(-LARGE, LARGE, -LARGE, LARGE, (dist*normal_).z(), LARGE);
		} else {
			return geom::BoundingBox(-LARGE, LARGE, -LARGE, LARGE, -LARGE, (dist*normal_).z());
		}
	} else {
		return BoundingBox::universalBox();
	}
}

#ifdef ENABLE_GUI
#include <vtkPlane.h>
// 基本的に扱いは交点を求める所以外はPlaneと同じ。
vtkSmartPointer<vtkImplicitFunction> geom::Triangle::generateImplicitFunction() const
{
	//mDebug() << "plane name=" << name() << "dist_=" << distance_ << "nor_=" << normal_ << "norm()=" << normal();
	auto c = math::dotProd(normal_, *vertices_[0].get())*normal_;
	auto n = -1*normal_;  // vtkとmcnpで法線の向きは逆っぽい
	auto plane = vtkSmartPointer<vtkPlane>::New();
	plane->SetOrigin(c.x(), c.y(), c.z());
	plane->SetNormal(n.x(), n.y(), n.z());
//	double *vtknorm = plane->GetNormal();
//	double *vtkorg = plane->GetOrigin();
	return plane;
}
#endif

// NOTE TracingParticleはtracing中に面とtrackが平行に面に乗りそうなら警告したほうがいいかも。

std::ostream& geom::operator << (std::ostream& os, const geom::Triangle& tri)
{
	return os << tri.toString();
}
