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
#include "cone.hpp"

#include <cmath>
#include <sstream>
#include <stdexcept>

#include "plane.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/utils/message.hpp"
#include "core/utils/numeric_utils.hpp"

// 名前、頂点、軸方向ベクトル、軸方向に単位長さ動いた位置での半径(=勾配)、シート
geom::Cone::Cone(const std::string name, const math::Point &point,
                 const math::Vector<3> &direction, double rad, int sheet)
	: Surface("CONE", name), vertex_(point), axis_(direction.normalized()), radius_(rad), sheet_(sheet)
{
	if(sheet_ != 0) sheet_ /= std::abs(sheet_);
	// axsix_がゼロベクトルは却下
	if(utils::isSameDouble(axis_.abs(), 0)) {
		throw std::invalid_argument(std::string("Axis vector of a cone should not zero vector"));
	}
	assert(utils::isSameDouble(axis_.abs(), 1));

	const double ax = axis_.abs();
	const double cossq = ax*ax/(ax*ax + radius_*radius_);  // cosθの二乗値
	cos_ = std::sqrt(cossq);
	M_ = math::tensorProd(axis_, axis_) - cossq*math::Matrix<3>::IDENTITY();
	//mDebug() << "theta = " << math::toDegrees(std::acos(cos_));
    boundingPlaneVectors_ = this->boundingPlanes();
}

std::string geom::Cone::toInputString() const
{
	// 円錐頂点3成分、軸ベクトル3成分
	std::stringstream ss;
	ss << name() << " ka" << " " <<  vertex_.x() << " " << vertex_.y() << " " << vertex_.z()
	   << " "  << axis_.x() << " " << axis_.y() << " " << axis_.z()
	   << " " << radius_ << " " << sheet_;
	return ss.str();
}

std::unique_ptr<geom::Surface> geom::Cone::createReverse() const
{
	std::unique_ptr<Surface> reversedCone(new Cone(Surface::reverseName(name_), vertex_, axis_, radius_, sheet_));
	reversedCone->setID(ID_*-1);
	return reversedCone;
}

bool geom::Cone::isForward(const math::Point &p) const
{
	auto relativeP = p - vertex_;
	double h = math::dotProd(axis_, relativeP);
	// h が正なら点pは軸方向にある。
	// hが0ならp==origin。「境界直上は表面の外側」ルールに従う
	if(h == 0) return reversed_ ? false : true;

	// シートとhの符号が異なる(+シートでhが負、-シートに対してhが正)なら問答無用でforward判定
	if(sheet_*h < 0) return reversed_ ? false : true;

	double value = math::dotProd(relativeP*M_, relativeP);
	// value >= 0ならcone内部
	//mDebug() << "h=" << h << ", val=" << value;
	// 以下ではシート符号は0か hと同符号のケース
	return reversed_ ? value > 0 :
	                   value <= 0;
}

math::Point geom::Cone::getIntersection(const math::Point &point, const math::Vector<3> &direction) const
{
	assert(utils::isSameDouble(direction.abs(), 1.0));

	/*
	 * 参照 https://www.geometrictools.com/Documentation/IntersectionLineCone.pdf
	 * 二次曲面は全て xMx形式で統一的に扱えるはず
	 */
	auto Delta = point - vertex_;      // 直線基準点--円錐頂点ベクトルΔ

	double c2 = math::dotProd(direction*M_, direction);
	double c1 = math::dotProd(direction*M_, Delta);
	double c0 = math::dotProd(Delta*M_, Delta);
	// 直線の媒介変数tに対し、 c2*t^2 + 2*c1*t + c0 = 0 の解 tが直線と円錐の交点に対応する。
	double discriminant = c1*c1-c0*c2;  // 判別式
	// 「直線が円錐に接している場合」は交点なし、「直線が円錐頂点を通る場合」は頂点が交点となる。
	if(discriminant <= 0 || utils::isSameDouble(c2, 0)) {
		if(math::isDependent(direction, axis_)) {
			return vertex_;  // 軸とdirectionが平行なら交点は頂点
		} else {
			return math::Point::INVALID_VECTOR();
		}
	}
	auto t1 = (-c1 - std::sqrt(discriminant))/c2;
	auto t2 = (-c1 + std::sqrt(discriminant))/c2;
	// tが負の解は進行方向後側にある点なので採用しない。
	double large, small;
	if(t1 > t2) {
		large = t1;
		small = t2;
	} else {
		large = t2;
		small = t1;
	}
	//mDebug() << "large, small=" << large << small;
	// 注意！  c2>0とは限らないので、t1とt2どちらが前方かはc2の符号を見ないと確定しない。
	// 進行方向遠い方の解がマイナスなら解は両方後方が確定するので、交点なしを返す
	if(large <= 0) return math::Point::INVALID_VECTOR();
	// ここで解ありが確定。
	/*
	 * 解は
	 * ・同一シート側に2個の場合、
	 * ・異なるシートに1個ずつある場合
	 * の2通りが考えられる。
	 *
	 * 2シートの場合「近い方」の「前方」の解を選択すれば良いが、
	 * 1シートの場合存在する交点の方を選択する必要がある。
	 *
	 */
	// 2シートの場合近い方を選択するだけでOK
	if(sheet_ == 0) return (small > 0) ? point + small*direction : point + large*direction;

	// 1シートの場合近い方からシート上にあるかを調べる。


	std::vector<math::Point> results;
	if(small > 0) results.emplace_back(point + small*direction);
	results.emplace_back(point + large*direction);

	//mDebug() << "temporaly result=" << results;

	for(auto &result: results) {
		auto axisProjection = math::dotProd(result - vertex_, axis_);
		//	mDebug() << "D=" << axis_;
		//	mDebug() << "result-V=" << result - origin_;
		//	mDebug() << "sheet=" << sheet_ << "prj=" << axisProjection;
		// Dとresult-Vが直行することはありえないが、発生したら交点なし
		// Dは頂点からrefPoint_向きなので、axisProjectionが正なら交点はrefPoint側のシート(=マイナス側シート)となる。
		if(utils::isSameDouble(axisProjection, 0)) return math::Point::INVALID_VECTOR();

		// sheet_と交点の軸への射影が同じ側にあればresultを返す。違う側にあればinvalid_vector
		if(sheet_*axisProjection > 0)  {  // 0を入れることで2sheetもここで考慮できる
			return result;
		}
	}
	return math::Vector<3>::INVALID_VECTOR();
}

std::string geom::Cone::toString() const
{
	std::stringstream ss;
	ss << Surface::toString() << ", vertex = " << vertex_ << ", axis = " << axis_
	   << ", gradient = " << radius_ << ", sheet = " << sheet_;
	return ss.str();
}

void geom::Cone::transform(const math::Matrix<4> &matrix)
{
    Surface::transform(matrix);
	if(!math::isSameMatrix(matrix, math::Matrix<4>::ZERO())) {
		math::affineTransform<3>(&vertex_, matrix);
		// axis_はrefPointから頂点へのベクトルなので平行移動の影響は受けない
		axis_ = (axis_*matrix.rotationMatrix()).normalized();
		// 軸テンソルも作り直し
		M_ = math::tensorProd(axis_, axis_) - cos_*cos_*math::Matrix<3>::IDENTITY();
	}
}



std::vector<std::vector<geom::Plane> > geom::Cone::boundingPlanes() const
{
	// 円錐では側面4つと1シートなら更に追加で1平面
	// 2シート面の場合4+1シートを2セット返す。
	// 側面はaxix_をaxisに垂直なに方向を軸に±(90-θ)度回転させた面を法線として作る
	auto vPair = math::get2OrthogonalUnitVectors(axis_);
	math::Vector<3> v1 = vPair.first, v2 = vPair.second;

	// mDebug() << "円錐軸に対して垂直な2方向はv1=" << v1 << ", v2=" << v2;
	// θを円錐頂角の1/2とすると、軸ベクトルを±(pi/2-θ)度回転させれば境界面の法線になる。
	// tan(-θ)=-tanθなのでわざわざ2通り計算する必要は無い。
	double t1 = std::tan(0.5*math::PI - std::atan(radius_));// tan(pi/2 -theta), tan(-theta)=-tan(theta)なのでt2は計算不要
    double rad1 = std::acos(std::sqrt(1.0/(1+t1*t1)));
    math::Vector<3> n1p = axis_*math::generateRotationMatrix2(-v2, -rad1);
    math::Vector<3> n1m = axis_*math::generateRotationMatrix2( v2, -rad1);
    math::Vector<3> n2p = axis_*math::generateRotationMatrix2(-v1, -rad1);
    math::Vector<3> n2m = axis_*math::generateRotationMatrix2( v1, -rad1);

//	mDebug() << "v2軸回り +" << math::toDegrees(0.5*math::PI - std::atan(radius_)) << "度回転で生成された法線は n1p=" << n1p;
//	mDebug() << "v2軸回り -" << math::toDegrees(0.5*math::PI - std::atan(radius_)) << "度回転で生成された法線は n1m=" << n1m;
	using PVec = std::vector<Plane>;

	if(sheet_ != 0) {
		// 1シートcone
		if(reversed_) {
			// 内向きならAND連結
			// 裏面のBB面は外接面を使うのでこのまま。
			PVec ret;
            ret.emplace_back("", sheet_*n1p, vertex_, false);
            ret.emplace_back("", sheet_*n1m, vertex_, false);
            ret.emplace_back("", sheet_*n2p, vertex_, false);
            ret.emplace_back("", sheet_*n2m, vertex_, false);
            ret.emplace_back("", sheet_*axis_, vertex_, false);
			return std::vector<PVec>{std::move(ret)};
		} else {
			// 外向きなら各要素別々にvectorへpush
			// 表面のBB面はaxisに直交する断面で切断した時に内接するような面になるように調整。
			// 外接面はaxisを90-θ回転させるが内接面は 90-θ', tanθ'=r/sqrt(2)とする。
			double t2 = std::tan(0.5*math::PI - std::atan(0.5*std::sqrt(2.0)*radius_));
            double rad2 = std::acos(std::sqrt(1.0/(1.0+t2*t2)));
            math::Vector<3> n1pi = axis_*math::generateRotationMatrix2(-v2, -rad2);
            math::Vector<3> n1mi = axis_*math::generateRotationMatrix2( v2, -rad2);
            math::Vector<3> n2pi = axis_*math::generateRotationMatrix2(-v1, -rad2);
            math::Vector<3> n2mi = axis_*math::generateRotationMatrix2( v1, -rad2);
			return std::vector<PVec>{
                PVec{Plane("", -sheet_*n1pi, vertex_, false)},
                PVec{Plane("", -sheet_*n1mi, vertex_, false)},
                PVec{Plane("", -sheet_*n2pi, vertex_, false)},
                PVec{Plane("", -sheet_*n2mi, vertex_, false)},
                PVec{Plane("", -sheet_*axis_, vertex_, false)}
			};
		}
	} else {
		// 2シート
		if(reversed_) {
			// 裏面のBB面は外接面を使うのでこのまま。
			PVec ret1;
            ret1.emplace_back("", n1p, vertex_, false);
            ret1.emplace_back("", n1m, vertex_, false);
            ret1.emplace_back("", n2p, vertex_, false);
            ret1.emplace_back("", n2m, vertex_, false);
            ret1.emplace_back("", axis_, vertex_, false);
			PVec ret2;
            ret2.emplace_back("", -n1p, vertex_, false);
            ret2.emplace_back("", -n1m, vertex_, false);
            ret2.emplace_back("", -n2p, vertex_, false);
            ret2.emplace_back("", -n2m, vertex_, false);
            ret2.emplace_back("", -axis_, vertex_, false);
			return std::vector<PVec>{std::move(ret1), std::move(ret2)};
		} else{
			// 2シートconeの外向きはやや複雑
			// 表面のBB面は内接するような面になるように調整
			double t2 = std::tan(0.5*math::PI - std::atan(0.5*std::sqrt(2.0)*radius_));
            double rad2 = std::acos(std::sqrt(1.0/(1.0+t2*t2)));
            math::Vector<3> n1pi = axis_*math::generateRotationMatrix2(-v2, -rad2);
            math::Vector<3> n1mi = axis_*math::generateRotationMatrix2( v2, -rad2);
            math::Vector<3> n2pi = axis_*math::generateRotationMatrix2(-v1, -rad2);
            math::Vector<3> n2mi = axis_*math::generateRotationMatrix2( v1, -rad2);

            Plane zpPlane = Plane("", -axis_, vertex_, false);
            Plane zmPlane = Plane("", axis_, vertex_, false);
			return std::vector<PVec>{
                PVec{Plane("", n1pi, vertex_, false), zpPlane},
                PVec{Plane("", n1mi, vertex_, false), zpPlane},
                PVec{Plane("", n2pi, vertex_, false), zpPlane},
                PVec{Plane("", n2mi, vertex_, false), zpPlane},
                PVec{Plane("", -n1pi, vertex_, false), zmPlane},
                PVec{Plane("", -n1mi, vertex_, false), zmPlane},
                PVec{Plane("", -n2pi, vertex_, false), zmPlane},
                PVec{Plane("", -n2mi, vertex_, false), zmPlane},
			};
		}
    }
}

std::shared_ptr<geom::Surface> geom::Cone::makeDeepCopy(const std::string &newName) const
{
    return std::make_shared<Cone>(newName, vertex_, axis_, radius_, sheet_);
}


std::unique_ptr<geom::Cone> geom::Cone::createCone(const std::string &name,
                                                       std::vector<double> params,
                                                       const math::Matrix<4> &trMatrix,
													   geom::Cone::TYPE type,
												   bool warnPhitsCompat)
{
	(void)warnPhitsCompat;
	// シートパラメータ省略時は0を追加する。
	if(params.size() == 4 || params.size() == 2 || params.size() == 7) {
		params.emplace_back(0);
	}
	math::Point origin;    // 頂点
	math::Vector<3> axis;  // 参照点から頂点へのクトル
	double radius2 = -1;         // 参照点での半径の二乗
	int sheet;             // 正：+側1シート、負：-側1シート、0：2シート

	// KX, KXO, KY, KYO, KZ, KZO, KA
	switch(type) {
	case TYPE::KX:
		Surface::CheckParamSize(5, "K/X", params);
		origin = math::Point{params.at(0), params.at(1), params.at(2)};
		axis = math::Vector<3>{1, 0, 0};
		radius2 = params.at(3);
		break;
	case TYPE::KY:
		Surface::CheckParamSize(5, "K/Y", params);
		origin = math::Point{params.at(0), params.at(1), params.at(2)};
		axis = math::Vector<3>{0, 1, 0};
		radius2 = params.at(3);
		break;
	case TYPE::KZ:
		Surface::CheckParamSize(5, "K/Z", params);
		origin = math::Point{params.at(0), params.at(1), params.at(2)};
		axis = math::Vector<3>{0, 0, 1};
		radius2 = params.at(3);
		break;

	case TYPE::KXO:
		Surface::CheckParamSize(3, "KX", params);
		origin = math::Point{params.at(0), 0, 0};
		axis = math::Vector<3>{1, 0, 0};
		radius2 = params.at(1);
		break;
	case TYPE::KYO:
		Surface::CheckParamSize(3, "KY", params);
		origin = math::Point{0, params.at(0), 0};
		axis = math::Vector<3>{0, 1, 0};
		radius2 = params.at(1);
		break;
	case TYPE::KZO:
		Surface::CheckParamSize(3, "KZ", params);
		origin = math::Point{0, 0, params.at(0)};
		axis = math::Vector<3>{0, 0, 1};
		radius2 = params.at(1);
		break;

	case TYPE::KA:
		Surface::CheckParamSize(8, "KA", params);
		origin = math::Point{params.at(0), params.at(1), params.at(2)};
		axis = math::Vector<3>{params.at(3), params.at(4), params.at(5)};
		radius2 = params.at(6)*params.at(6);
		break;
    default:
        throw std::invalid_argument("Invalid Cone type;");
	}
	if(radius2 < 0) {
		throw std::invalid_argument("t^2 (or radius at reference point) is negative");
	}
	// このルーチンの冒頭でparamsの最後がシートオプションになるように調整済み
	sheet = static_cast<int>(params.back());
	std::unique_ptr<Cone> cone(new Cone(name, origin, axis, std::sqrt(radius2), sheet));
	cone->transform(trMatrix);
	return cone;
}

geom::BoundingBox geom::Cone::generateBoundingBox() const
{
	// coneが2シートあるいは法線が外側ならBBは全空間
	if(!reversed_ || sheet_ == 0) return BoundingBox::universalBox();

	// 単純のため軸方向でなければ無限大適用
	const double LARGE = BoundingBox::MAX_EXTENT;
	const math::Vector<3> ux{1, 0, 0}, uy{0, 1, 0}, uz{0, 0, 1};
	if(math::isDependent(axis_, ux)) {
		if(math::dotProd(ux, axis_) * sheet_ > 0) {
			return BoundingBox(vertex_.x(), LARGE, -LARGE, LARGE, -LARGE, LARGE);
		} else {
			return BoundingBox(-LARGE, vertex_.x(), -LARGE, LARGE, -LARGE, LARGE);
		}
	} else if(math::isDependent(axis_, uy)) {
		if(math::dotProd(uy, axis_) * sheet_ > 0) {
			return BoundingBox(-LARGE, LARGE, vertex_.y(), LARGE, -LARGE, LARGE);
		} else {
			return BoundingBox(-LARGE, LARGE, -LARGE, vertex_.y(), -LARGE, LARGE);
		}
	} else if(math::isDependent(axis_, uz))  {
		if(math::dotProd(uz, axis_) * sheet_ > 0) {
			return BoundingBox(-LARGE, LARGE, -LARGE, LARGE, vertex_.z(), LARGE);
		} else {
			return BoundingBox(-LARGE, LARGE, -LARGE, LARGE, -LARGE, vertex_.z());
		}
	} else {
		return BoundingBox::universalBox();
	}


}



#ifdef ENABLE_GUI
#include <vtkCone.h>
#include <vtkImplicitBoolean.h>
#include <vtkPlane.h>
#include <vtkSphere.h>
#include <vtkTransform.h>
vtkSmartPointer<vtkImplicitFunction> geom::Cone::generateImplicitFunction() const
{
	using Mat = math::Matrix<4>;
//    mDebug() << "\nCone name=" << name() <<  "vertex=" << vertex_ << "axis=" << axis_
//             << "radius=" << radius_ << "angle(deg)=" << math::toDegrees(std::atan(radius_)) << "sheet=" << sheet_;


	/*
     * ********************** 重要 *****************************
     *  vtkConeはX軸方正向きしか無いので適当に回転させる必要がある
     * ・先端の座標が(0, 0, 0)
     * ・底面から先端方向の軸方向ベクトルは(-1, 0, 0)
     *
     * したがってまずvtk空間内で陰関数を生成して、最後に変換をかける。
     * 変換は (0,0,0) → vertex_
     * (-1, 0, 0) → axis_方向
     * *********************************************************
	 */

    // vtk空間から実空間への変換行列trMatrix(math::Matrix), trf(vtkMatrix)を作成する。
	// 並進は0,0,0→vetex_の並進, 回転はx軸とaxis
	Mat trMatrix = Mat::IDENTITY();
    trMatrix.setTranslationVector(-1*vertex_);
    math::Matrix<3> rotMatrix3 = math::generateRotationMatrix1(math::Vector<3>{-1, 0, 0}, axis_);
    auto rotMatrix4 = Mat::IDENTITY();
    rotMatrix4.setRotationMatrix(rotMatrix3);

    trMatrix = trMatrix*rotMatrix4;
	// vtkMatrixは並びはrow-majorだが、vectorを縦ベクトルとして
	// Matrixを左から掛けるようになっている。故に転置する必要がある。
    auto vtkMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    for(size_t j = 0; j < 4; ++j) {
        for(size_t i = 0; i < 4; ++i) {
            vtkMatrix->SetElement(static_cast<int>(i), static_cast<int>(j), trMatrix(j, i));
		}
	}
	auto trf = vtkSmartPointer<vtkTransform>::New();
	trf->SetMatrix(vtkMatrix);
    /*
     * ここまで整理
     * trMatrix:円錐頂点を原点に、円錐軸をx軸負方向へ回転させる4x4行列
     * trf:そのvtkTransform版
     *
     */
    auto tmpVertex = vertex_;

    math::affineTransform(&tmpVertex, trMatrix);
//    mDebug() << "axis===" << axis_ << ", axis*rotMat ===" << axis_*rotMatrix3;
//    mDebug() << "rotMat ===" << rotMatrix3;
//    mDebug() << "vertex===" << vertex_ << ", affine(vertex_)===" << tmpVertex;
//    assert(math::isSamePoint(tmpVertex, math::Point{0, 0, 0}));
//    assert(math::isSamePoint(axis_*rotMatrix3, math::Vector<3>{-1, 0, 0}));

    auto cone = vtkSmartPointer<vtkCone>::New();
    cone->SetAngle(math::toDegrees(std::atan(radius_)));


    vtkSmartPointer<vtkImplicitFunction> coneFunc;
	if(sheet_ != 0) {
	// 1sheetなら平面を作ってboolean
        //mDebug() << "for 1sheet cone, setting cutting plane. vertex ===" << vertex_ << "axis===" << axis_;
		auto plane = vtkSmartPointer<vtkPlane>::New();
        plane->SetOrigin(0, 0, 0);
		if(sheet_ == 1) {
            plane->SetNormal(1, 0, 0);
		} else if (sheet_ == -1) {
            plane->SetNormal(-1, 0, 0);
		} else {
			throw std::invalid_argument("Cone'ssheet_ should be -1, 1, or 2. actual = "
										+ std::to_string(sheet_));
		}
//        plane->Print(std::cout);

		auto boolOp = vtkSmartPointer<vtkImplicitBoolean>::New();
		boolOp->AddFunction(cone);
        boolOp->AddFunction(plane);
        boolOp->SetOperationTypeToIntersection();
        boolOp->SetTransform(trf);
        boolOp->Modified();
		coneFunc = boolOp;

	} else {
    // 2sheetならカットなし
        mDebug() << "2sheet cone!!!!!!!!!!!!!!!!";
        cone->SetTransform(trf);
		cone->Modified();
		coneFunc = cone;
	}
	// vtkの陰関数定義に合うように適当に反転させる
	if(!reversed_) {
		return Surface::getVtkCompliment(coneFunc);

	} else {
		return coneFunc;
	}
}
#endif

