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
#include "plane.hpp"

#include <cmath>
#include <stdexcept>
#include <sstream>

#include "core/geometry/cell/boundingbox.hpp"
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"
#include "core/utils/numeric_utils.hpp"


std::unique_ptr<geom::Plane> geom::Plane::createPlane(const std::string &name,
													   const std::vector<double> &params,
													   const math::Matrix<4> &trMatrix,
													   geom::Plane::TYPE type,
													   bool warnPhitsCompat
													  )
{
	math::Vector<3> normalVector;
	double dist;
	switch(type) {
	case TYPE::P:
		Surface::CheckParamSize(std::vector<size_t>{4, 9}, "P", params);
		if(params.size() == 4) {
			normalVector = math::Vector<3>{params.at(0), params.at(1), params.at(2)};
			dist = params.at(3);
		} else if (params.size() == 9) {
			if(warnPhitsCompat) {
				mWarning() << "9-input P card is not phits compatible.";
			}
			/*
			 *  3点(9パラメータ)入力面の向きはややこしいことに、以下の順で決定される
			 * 1．refPointを含む側が負
			 * 2．面がrefPointを通るとき(D=0)は (0, 0, +∞)を含む方が表
			 * 3．面がZ軸に平行な場合(D=C=0)は (0, +∞, 0)を含む方が表
			 * 4．面がZ軸Y軸に平行な場合(D=C=B=0)は(+∞, 0, 0)を含む方向が表
			 * 5．それでも決まらなければ（D=C=B=A=0）エラー
			 * となっている。面倒くさい。時計周りとかで判定してくれればいいものを。
			 * Ref MCNP5 manual vol.3 p.17
			 *
			 * refPointはデフォルトでは{0, 0, 0}
			 */

			math::Point p1{params.at(0), params.at(1), params.at(2)};
			math::Point p2{params.at(3), params.at(4), params.at(5)};
			math::Point p3{params.at(6), params.at(7), params.at(8)};
			auto v2 = p1 - p2;
			auto v3 = p1 - p3;
			/* 3点が同一直線にある場合、面は一意に決まらない。(がMCNPではエラーにならない。)
			* 例: 3点 (0 5 0)   (5 0 0)  (3.5355 3.5333 0)   は、
			* PZ 0
			* P 1 1 0 3.5355
			* どちらの面の上でもある。そこまでMCNPの挙動に合わせるのは面倒なのでエラーにする。
			*/
			if(math::isDependent(v2, v3)) {
				throw std::invalid_argument(std::string("Three points are on the same line. Plane cannot be determined.") +
											"pts = {" + p1.toString() + "}, {" + p2.toString() + "}, {" + p3.toString() + "}");
			}

			normalVector = math::crossProd(v2, v3).normalized();
			dist = math::dotProd(p1, normalVector);

			// normalベクトルの方向設定
			if(utils::isSameDouble(dist, 0)) {
				// 面が参照点点を通る場合の法線方向の設定。
//				mDebug() << "面=" << name << "は原点を通るので法線向きの調整が必要。normal=" << normalVector;
				if(utils::isSameDouble(normalVector.z(), 0)) {
					if(utils::isSameDouble(normalVector.y(), 0)) {
						if(utils::isSameDouble(normalVector.x(), 0)) {
							// A=B=C=D=0の場合定義しようがないのでエラー
							throw std::invalid_argument("Normal vector cannot be defined(=0 0 0).");
						} else {
						// 面がzy軸に平行な場合＋x方向を法線ベクトルになるように反転する
							if(normalVector.x() < 0) {
								normalVector *= -1.0;
							}
						}
					} else {
						// 面がz軸平行の場合＋y方向を法線方向にする
						if(normalVector.y() < 0) {
							normalVector *= -1.0;
						}

					}
				} else {
					// 面が原点を通る場合＋z方向を法線方向にとる。
					if(normalVector.z() < 0) {
						normalVector *= -1.0;
					}
				}
			} else if(dist < 0){
				// distが負なら原点は法線側なので逆方向にする。(ケース1．)
				normalVector = -1*normalVector;
			}
			// 参照点はあくまでオモテウラ決定のためだけなので、本来のdistを再度計算する。
			dist = math::dotProd(p1, normalVector);

		} else {
			throw std::invalid_argument("Number of parameters should be 4 or 9 for P input");
		}
		break;
	case TYPE::PX:
		Surface::CheckParamSize(1, "PX", params);
		normalVector = math::Vector<3>{1, 0, 0};
		dist = params.at(0);
		break;
	case TYPE::PY:
		Surface::CheckParamSize(1, "PY", params);
		normalVector = math::Vector<3>{0, 1, 0};
		dist = params.at(0);
		break;
	case TYPE::PZ:
		Surface::CheckParamSize(1, "PZ", params);
		normalVector = math::Vector<3>{0, 0, 1};
		dist = params.at(0);
		break;
	default:
		abort();
		break;
	}
	// 内部表現では法線ベクトル規格化はコンストラクタで実施するのでここでは不要。
	std::unique_ptr<Plane> pl(new Plane(name, normalVector, dist));
	pl->transform(trMatrix);
	return pl;
}

geom::Plane::TYPE geom::Plane::strToType(const std::string &str)
{
	static const std::unordered_map<std::string, TYPE> typeMap {
		{"p", TYPE::P},
		{"P", TYPE::P},
		{"px", TYPE::PX},
		{"PX", TYPE::PX},
		{"py", TYPE::PY},
		{"PY", TYPE::PY},
		{"pz", TYPE::PZ},
		{"PZ", TYPE::PZ},
	};
	return typeMap.at(str);
}


geom::Plane::Plane(const std::string &name, const math::Vector<3> &normal,
                   double distance, bool hasBoundingPlane)
	:Surface("PLANE", name)
{
	// normalは単位ベクトルでなくてもOKにしてコンストラクタで内部表現(法線ベクトルは長さ1)に合わせる。
	assert(normal.abs() >0);
	normal_ = normal.normalized();
	distance_ = distance/normal.abs();
	assert(std::abs(normal_.abs() - 1) < math::Vector<3>::EPS);
    if(hasBoundingPlane) boundingPlaneVectors_ = boundingPlanes();
}

geom::Plane::Plane(const std::string &name, const math::Vector<3> &normal, const math::Point &pt, bool hasBoundingPlane)
    :Plane(name, normal, math::dotProd(pt, normal), hasBoundingPlane)
{;}

// 3点方式による面の定義。時計回りに見えるほうが表。
geom::Plane::Plane(const std::string &name, const math::Point &p1, const math::Point &p2, const math::Point &p3,
                   NormalType flag, bool hasBoundingPlane)
	:Surface("PLANE", name)
{
	// MCNP独特の法線定義詳細はPカード＋9引数入力の項を参照のこと。
	if(flag == NormalType::MCNP) {
		std::shared_ptr<Plane> tmp = createPlane(name, {p1.x(), p1.y(), p1.z(), p2.x(), p2.y(), p2.z(), p3.x(), p3.y(), p3.z()},
												 math::Matrix<4>::IDENTITY(), TYPE::P, false);
		*this = *tmp.get();
	} else {
		auto v1 = p2 - p1, v2 = p3 - p2;
		auto norm = math::crossProd(v1, v2);
        if(norm.abs() < math::EPS) {
            std::stringstream ss;
            ss << "Constructing a plane, length of normal vector is zero, points="
                << p1 << p2 << p3;
            throw std::invalid_argument(ss.str());
		}
		normal_ = norm.normalized();

		if(flag == NormalType::CoutnerClock){
			normal_ = -1*normal_;
		}
		distance_ = math::dotProd(p1, normal_);
	}
    if(!hasBoundingPlane)boundingPlaneVectors_ = boundingPlanes();
}


// Pカードの最後のパラメータは法線に沿った距離。
std::string geom::Plane::toInputString() const
{
	std::stringstream ss;
	ss << name()
	   << " p " << normal_.at(0) << " " << normal_.at(1) << " " << normal_.at(2)
	   << " " << distance_;
	return ss.str();
}

double geom::Plane::distanceToPoint(const math::Point &pt) const
{
	//return math::dotProd((pt - distance_*normal_), normal_);
	return math::dotProd(pt, normal_) - distance_; // ↑とおなじ。
}

void geom::Plane::alignNormal(const math::Vector<3> &n)
{
	if(math::dotProd(n, normal_) < 0) {
		normal_ = -1*normal_;
		distance_ = -1*distance_;
	}
	return;
}


std::unique_ptr<geom::Surface> geom::Plane::createReverse() const
{
	if(reversed_) throw std::invalid_argument("cannot create reverse surface of reverse surface."
											  "source plane =" + this->toString());
	/*
	 * 裏面のnormal_も表面同様実際の法線に合わせるようにした。その結果distance_も-1倍になる。
	 */
	std::unique_ptr<geom::Surface> reversedPlane(new Plane(Surface::reverseName(name_), -1*normal_, -1*distance_));
	reversedPlane->setID(ID_*-1);
	return reversedPlane;
}


bool geom::Plane::isForward(const math::Point &p) const
{
	// valueが負なら原点側、正なら原点より遠い側
	double value = math::dotProd(normal_, p) - distance_;
	return (!reversed_) ? value >= 0: value > 0;
}

// 点pointからdirection方向の光線とPlaneの交点
math::Point geom::Plane::getIntersection(const math::Point &point, const math::Vector<3> &direction) const
{
	double denominator = math::dotProd(normal_, direction);
	if(std::abs(denominator) < math::Point::EPS) {
		return math::Point::INVALID_VECTOR();  // denominator==0 なdirectionと法線が直行していて面との交点なし
	} else {
		// delta < 0 なら進行方向逆に交点があるのでinvalid_vectorを返す。
		// delta == 0なら現在の点を返す。
		double delta = (distance_ - math::dotProd(normal_, point))/denominator;
		return (delta >= 0) ? point + delta*direction : math::Point::INVALID_VECTOR();
	}
}

std::string geom::Plane::toString() const
{
	std::stringstream ss;
	ss << Surface::toString() << ", normal = " << normal_ << ", distance = " <<distance_;
	return ss.str();
}

void geom::Plane::transform(const math::Matrix<4> &matrix)
{
    Surface::transform(matrix);
	if(!math::isSameMatrix(matrix, math::Matrix<4>::ZERO()) && !math::isSameMatrix(matrix, math::Matrix<4>::IDENTITY())) {
		// Tr変換は法線ベクトルの回転と、距離を平行移動ベクトルの法線への射影分増加させることで実現
		normal_ = normal_*matrix.rotationMatrix();
		distance_ = distance_ + math::dotProd(matrix.translationVector(), normal_);

	}
	assert(std::abs(normal_.abs() - 1) < math::Vector<3>::EPS);
}

std::vector<std::vector<geom::Plane>> geom::Plane::boundingPlanes() const
{
	// boundingBox計算時に適当にオフセットを就けるのでここではオフセットは付与しない。
	// 無名の等価な面を作成して返す
    return std::vector<std::vector<Plane>>{{Plane("", normal(), distance(), false)}};
}

std::shared_ptr<geom::Surface> geom::Plane::makeDeepCopy(const std::string &newName) const
{
    return std::make_shared<Plane>(newName, normal_, distance_, true);
}



std::unique_ptr<geom::Plane> geom::Plane::fromString(const std::string &srcStr, bool removeSign)
{
	std::vector<std::string> params = utils::splitString(" ", srcStr, true);
	if(params.size() <= 2) throw std::invalid_argument("invalid argument for Plane::fromString, arg=" + srcStr);
	std::string name = params.at(0), mnemonic = params.at(1);
	auto symbol = Plane::strToType(mnemonic);
	if(removeSign && name.front() == '-') name = name.substr(1);
	// paramsの最初2個は削除する。
	std::vector<double> dParams = utils::stringVectorTo<double>(std::vector<std::string>(params.begin()+2, params.end()));
	return Plane::createPlane(name, dParams, math::Matrix<4>(), symbol, false);
}

math::Point geom::Plane::projection(const geom::Plane &pl, const math::Point &pt)
{
	return pt - (math::dotProd(pt, pl.normal_) - pl.distance_)*pl.normal_;
}

bool geom::Plane::isSamePlane(const geom::Plane &p1, const geom::Plane &p2)
{
	return utils::isSameDouble(p1.distance_, p2.distance_) &&
			math::isSamePoint(p1.normal_, p2.normal_);
}

bool geom::Plane::isSamePlane(const std::shared_ptr<const geom::Plane> &p1,
									 const std::shared_ptr<const geom::Plane> &p2)
{
	return isSamePlane(*p1.get(), *p2.get());
}

math::Point geom::Plane::intersection(const geom::Plane &p1, const geom::Plane &p2, const geom::Plane &p3)
{
	// それぞれの面の法線を行ベクトルとして、distを右辺として方程式を解けばいい。det=0なら解なし
	math::Matrix<3> matrix(std::array<math::Vector<3>, 3>{p1.normal(), p2.normal(), p3.normal()});
    // Vector<3>は行ベクトルなのでこのmatrixは転置する必要がある。
    matrix = matrix.transposed();
	if(!matrix.isRegular()) return math::Vector<3>::INVALID_VECTOR();
    return  math::Point{math::dotProd(p1.normal_, p1.normal_*p1.distance_),
                        math::dotProd(p2.normal_, p2.normal_*p2.distance_),
                        math::dotProd(p3.normal_, p3.normal_*p3.distance_)}*matrix.inverse();
}
math::Point geom::Plane::intersection(const std::shared_ptr<Plane> &p1,
									  const std::shared_ptr<Plane> &p2,
									  const std::shared_ptr<Plane> &p3)
{
	return intersection(*p1.get(), *p2.get(), *p3.get());
}

geom::BoundingBox geom::Plane::generateBoundingBox() const
{
	//mDebug() << "Creating BoundingBox for Plane =" << this->toInputString();
	const double LARGE = geom::BoundingBox::MAX_EXTENT;
	const math::Vector<3> xvec{1, 0, 0}, yvec{0, 1, 0}, zvec{0, 0, 1};
	if(math::isDependent(normal_, xvec)) {
		// x軸平行なら
		if(math::dotProd(normal_, math::Vector<3>{1, 0, 0}) > 0){  // 法線が+x向き
			return geom::BoundingBox((distance_*normal_).x(), LARGE, -LARGE, LARGE,-LARGE, LARGE);
		} else {
			return geom::BoundingBox(-LARGE, (distance_*normal_).x(), -LARGE, LARGE,-LARGE, LARGE);
		}
	} else if(math::isDependent(normal(), yvec)) {
		if(math::dotProd(normal(), math::Vector<3>{0, 1, 0}) > 0){
			return geom::BoundingBox(-LARGE, LARGE, (distance_*normal_).y(), LARGE,-LARGE, LARGE);
		} else {
			return geom::BoundingBox(-LARGE, LARGE, -LARGE, (distance_*normal_).y(), -LARGE, LARGE);
		}
	} else if (math::isDependent(normal(), zvec)) {
		if(math::dotProd(normal(), math::Vector<3>{0, 0, 1}) > 0){
			return geom::BoundingBox(-LARGE, LARGE, -LARGE, LARGE, (distance_*normal_).z(), LARGE);
		} else {
			return geom::BoundingBox(-LARGE, LARGE, -LARGE, LARGE, -LARGE, (distance_*normal_).z());
		}
	} else {
		return BoundingBox::universalBox();
	}
}

#ifdef ENABLE_GUI
#include <vtkPlane.h>
vtkSmartPointer<vtkImplicitFunction> geom::Plane::generateImplicitFunction() const
{
	//mDebug() << "plane name=" << name() << "dist_=" << distance_ << "nor_=" << normal_ << "norm()=" << normal();
	auto c = distance_*normal_;
	auto n = -1*normal_;  // vtkとmcnpで法線の向きは逆っぽい
	auto plane = vtkSmartPointer<vtkPlane>::New();
	plane->SetOrigin(c.x(), c.y(), c.z());
	plane->SetNormal(n.x(), n.y(), n.z());
//	double *vtknorm = plane->GetNormal();
//	double *vtkorg = plane->GetOrigin();
	return plane;
}
#endif


bool geom::operator <(const geom::Plane &pl1, const geom::Plane &pl2)
{
	return pl1.name_ < pl2.name_;
}


