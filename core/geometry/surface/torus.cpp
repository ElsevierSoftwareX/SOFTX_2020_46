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
#include "torus.hpp"

#include <cmath>

#include "plane.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/utils/message.hpp"
#include "core/utils/numeric_utils.hpp"
#include "core/math/equationsolver.hpp"


geom::Torus::Torus(const std::string &name, const math::Point &center, const math::Vector<3> &axis,
                   double majorR, double minorVRadius, double minorHRadius)
    :Surface("TORUS", name), center_(math::Point{0,0,0}), axis_(math::Vector<3>{0, 0, 1}),
      R(majorR), a(minorVRadius), b(minorHRadius), a2(a*a), b2(b*b)
{
	typedef math::Matrix<4> Mat;
	if(a <= 0) {
		throw std::invalid_argument("Minor vertical radius of a torus should be positive");
	} else if (b <= 0) {
		throw std::invalid_argument("Minor horizontal radius of a torus should be positive");
	} else if (R <= 0) {
		throw std::invalid_argument("Major radius of a torus should be positive");
	}

	/*
	 * Surfaceに対して変換を掛ける場合 回転→並進の順に適用される。
	 * よって逆にに座標を変換する行列を作る場合はは 並進→回転の順に適用する。
	 */
	auto tmpMatrix = Mat::IDENTITY();
	auto v = axis.normalized();
	// 引数の軸ベクトルがz軸ではない場合は変換行列作成
	if(!isSamePoint(axis_, v)) {
		// axis を {0, 0, 1}へ回転する行列を求める
		auto rot = math::generateRotationMatrix1(v, axis_);
		auto tmp = Mat::IDENTITY();
		tmp.setRotationMatrix(rot);
		tmpMatrix = tmpMatrix*tmp;
	}
	if(!isSamePoint(center_, center)) {
		tmpMatrix = tmpMatrix*Mat({1, 0, 0, 0,
		                            0, 1, 0, 0,
		                            0, 0, 1, 0,
		                            center.x(), center.y(), center.z(), 1});
	}
	if(!math::isSameMatrix(tmpMatrix, Mat::IDENTITY())) {
		trMatrix_ = std::unique_ptr<Mat>(new Mat(tmpMatrix));
		invMatrix_ = trMatrix_->inverse();
	}
//	mDebug() << "END of constructor trmat="
    boundingPlaneVectors_ = boundingPlanes();
}

std::shared_ptr<geom::Surface> geom::Torus::makeDeepCopy(const std::string &newName) const
{
    return std::make_shared<Torus>(newName, center_, axis_, R, a, b);
}


std::string geom::Torus::toInputString() const
{
	/*
	 * 入力では TA  x y z  vx vy vz  R a b
	 * を取るが、内部的には交点計算の簡単化のため(x, y, z)=center_=(0, 0, 0), (vx, vy, vz)=axis_=(0, 0, 1)
	 * としている。故に入力文字列にexportする時center_→実際の中心、axis_→実際の軸ベクトル,
	 * への変換を施す必要がある。
	 */
	std::stringstream ss;
	auto realAxis = axis();
	auto realCenter = center();
	ss <<  name() << " ta" << " " <<  realCenter.x() << " " << realCenter.y() << " " << realCenter.z() << " "
	   << realAxis.x() << " " << realAxis.y() << " " << realAxis.z() << " " << R << " " << a << " " << b;

//	if(trMatrix_) {
//		auto trVec = trMatrix_->translationVector();
//		ss << "trcl=(" << trVec.x() << " " << trVec.y() << " " << trVec.z() << " ";
//		for(size_t i = 0; i < 3; ++i) {
//			for(size_t j = 0; j < 3; ++j) {
//				ss << (*trMatrix_.get())(i, j);
//				if(i != 2 && j != 2) ss << " ";
//			}
//		}
//	}
	return ss.str();
}

std::unique_ptr<geom::Surface> geom::Torus::createReverse() const
{
	std::unique_ptr<Surface> rev(new Torus(Surface::reverseName(name_), center_, axis_, R, a, b));
	rev->setID(ID_*-1);
	if(trMatrix_) rev->transform(*trMatrix_);
	return rev;
}

std::string geom::Torus::toString() const
{
	std::stringstream ss;
	ss << Surface::toString() << ", center = " << center() << " axis = " << axis()
	   << " R = " << R << " a,b = " << a << "," << b;
	if(trMatrix_) {
		ss << "  tr from TZ =" << *trMatrix_;
	}
	return ss.str();
}

// 座標変換は中心位置はアフィン変換する。
// axis_はz方向固定なので、TRに回転成分がある場合はtrMatrixを変換する
void geom::Torus::transform(const math::Matrix<4> &matrix)
{
    Surface::transform(matrix);
	typedef math::Matrix<4> Mat4;
//	if(trMatrix_) {
//		mDebug() << "Transform befoe tr=" << *trMatrix_.get();
//		//mDebug() << "inv=" << invMatrix_;
//	} else {
//		mDebug() << "Transform before is not set";
//	}
	if(!math::isSameMatrix(matrix, Mat4::ZERO())) {
		if(!trMatrix_) trMatrix_ = std::unique_ptr<Mat4>(new Mat4(Mat4::IDENTITY()));
		*trMatrix_ = *trMatrix_ * matrix;
		invMatrix_ = trMatrix_->inverse();  // rotMatrixを更新したら逆行列も更新する
	}
	//mDebug() << "Transform after tr=" << *trMatrix_.get();
	//mDebug() << "inv=" << invMatrix_;
}

bool geom::Torus::isForward(const math::Point &point) const
{
	math::Point p = point;
	if(trMatrix_) math::affineTransform(&p, invMatrix_);

	double term1 = b*b*p.z()*p.z() + a2*(p.x()*p.x() + p.y()*p.y() + R*R - b*b);
	term1 = term1*term1;
	double term2 = -4*a2*a2*R*R*(p.x()*p.x() + p.y()*p.y());
	//mDebug() << "Torus implicit value=" << std::setw(25) << std::setprecision(25) << term1+term2;
	return (reversed_) ? term1 + term2 < 0
	                   : term1 + term2 >=0;

// 一般トーラスは複雑すぎるので採用しない
//	double A2 = a*a - b*b;
//	double Cn = math::dotProd(center_, axis_);
//	double C  = center_.abs();
//	double Pn = math::dotProd(p, axis_);
//	double P  = p.abs();
//	double term1 = 0.5*(P*P + A2 + C*C - R*R) - math::dotProd(center_, p)  - (a*a*a*a + A2*(Pn-Cn)*(Pn-Cn))/(2*a*a) ;
//	term1 = (term1*term1)/(R*R*b*b);
//	double term2 = (Pn-Cn)*(Pn-Cn)/(a*a) -1;
//	return (reversed_) ? term1 + term2 < 0
//	                   : term1 + term2 >=0;
}

#if defined(__GNUC__) && !defined(__clang__)
#include <quadmath.h>
#endif

math::Point geom::Torus::getIntersection(const math::Point &point, const math::Vector<3> &direction) const
{
//	typedef __float128 double_type;
	typedef double double_type;

	assert(!utils::isSameDouble(direction.abs(), 0));
	//mDebug() << "Enter Torus::getIntersection() name=" << name_ << "point=" << point << "direction=" << direction;
	math::Point p = point;
	math::Vector<3> d = direction.normalized();
	if(trMatrix_) {
		math::affineTransform(&p, invMatrix_);
		d = d*invMatrix_.rotationMatrix();
		//mDebug() << "トーラス局所座標系での位置=" << point << "direction=" << direction;
	}

//	mDebug() << "グローバルpos=" << point << ", 局所系p=" << p;
//	mDebug() << "グローバルdir=" << direction << ", 局所系d=" << d;

	double_type sx = p.x(), sy = p.y(), sz = p.z();
	double_type sx2 = sx*sx, sy2=sy*sy, sz2 = sz*sz;
	double_type dx = d.x(), dy = d.y(), dz = d.z();
	double_type c4 = b2*dz*dz + a2*dy*dy + a2*dx*dx;
	c4 = c4*c4;
	double_type c3 = 4*(sz*b2*dz + sy*a2*dy + sx*a2*dx)*(b2*dz*dz + a2*dy*dy + a2*dx*dx);
	double_type c2 = ((6*sz2-2*a2)*b2*b2 + (2*sy2 + 2*sx2 + 2*R*R)*a2*b2)*dz*dz
	        + (8*sy*sz*a2*b2*dy + 8*sx*sz*a2*b2*dx)*dz
	        + ((2*sz2*a2 -2*a2*a2)*b2 + (6*sy2 + 2*sx2 - 2*R*R)*a2*a2)*dy*dy
	        + 8*sx*sy*a2*a2*dx*dy + ((2*sz2*a2 - 2*a2*a2)*b2 + (2*sy2+ 6*sx2 - 2*R*R)*a2*a2)*dx*dx;
	double_type c1 = ((4*sz2*sz - 4*sz*a2)*b2*b2 + (4*sy2 + 4*sx2 + 4*R*R)*sz*a2*b2)*dz
	        + ((4*sy*sz2*a2 - 4*sy*a2*a2)*b2 + (4*sy2*sy + (4*sx2 - 4*R*R)*sy)*a2*a2)*dy
	        + ((4*sx*sz2*a2 - 4*sx*a2*a2)*b2 + (4*sx*sy2 + 4*sx2*sx - 4*R*R*sx)*a2*a2)*dx;
	double_type c0 = (a2*a2 - 2*sz2*a2 + sz2*sz2)*b2*b2
	        + ((-2*sy2 - 2*sx2 - 2*R*R)*a2*a2 + (2*sy2 + 2*sx2 + 2*R*R)*sz2*a2)*b2
	        + (sy2*sy2 + (2*sx2 - 2*R*R)*sy2 + sx2*sx2 -2*R*R*sx2 + R*R*R*R)*a2*a2;
//	mDebug() << "point(内部表現)=" << point << ", dir(内部表現)=" << direction;
//	mDebug() << "C4=" << c4 << "C3=" << c3 << "C2=" << c2 << "C1=" << c1 << "C0=" << c0;
//	std::cout << "c4=" <<std::setw(18) << std::setprecision(18) << c4 << ", c3=" << c3 << ", c2=" << c2 << ", c1=" << c1 << ", c0=" << c0 << std::endl;

	/*
	 * 4次式はもう少し安定的に解ける必要がある。
	 * 例：
	 *  ここで係数の僅かな差により実解2個(7, 13)→複素重解2個(10±13i)に別れる
	 * C4=81, C3=-3240,           C2=79542,          C1=-942840,          C0=3619161
	 *    → 4次式の解= {7, 13}
	 * C4=81, C3=-3239.9999999676, C2=79541.999999352, C1=-942839.999997052, C0=3619161
	 *    → 4次式の解= {9.99393, 9.99393, 10.0061, 10.0061}  (フェラリの公式では10 ± 13.821iの重解で合計4個)
	 *
	 * 解を実解に限定するように丸めてしまう方がよいかも。→ 誤差が乗ることに変わりはない。
	 * フェラーリの公式で得た解を使ってニュートン法で解を修正したら別に陰関数再計算しなくても
	 * いいんじゃね？ → 解に誤差が載っているというより、係数の計算に誤差が載っているのでは？
	 *                 → 係数の計算と陰関数の計算では陰関数のほうが多少マシという程度でそれほど変わらないと思われる。
	 *                  →あまり精度でない
	 * 非軸平行トーラスではうまく行くが少しでも軸に傾いたトーラスでは誤差発生する。
	 */


	auto sols = math::solve4thR<double>(c4, c3, c2, c1, c0, false);
	// 解のうち「正」で「一番小さいもの」を選択する。
	std::sort(sols.begin(), sols.end());
	//mDebug() << "修正前4次式の解=" << sols;
	for(size_t i = 0; i < sols.size(); ++i) {
		/*
		 * 4次式の解(byフェラーリの公式)の精度では p+t*dがトーラス上に乗ることは保証できない。
		 * なのでrefPointがトーラス上に来るようにtを調節する。
		 */
		//double before = sols.at(i);
		// NOTE 倍精度で期待できる精度はせいぜい11桁くらいであり、4次式の解の精度として期待できるのは1e-5程度であることは忘れてはならない
		/*
		 * 現在enterCell関数でのセル内への移動ステップサイズ＝100*math::EPS=1E+7*numeric_limits<double>::epsilon() 〜 1E-8
		 * はこれよりもかなり小さい。ゆえに 内外判定と交点位置の間に矛盾が出る可能性がある。
		 *
		 * 対策：とりあえず4倍精度ニュートン法で修正をかける → まあそれなりに改善する
		 */

#if defined(__GNUC__) && !defined(__clang__)
		sols.at(i) = math::mod4thRbyNewton<__float128>(c4, c3, c2, c1, c0, sols.at(i), 1e-12);
#else
		sols.at(i) = math::mod4thRbyNewton<long double>(c4, c3, c2, c1, c0, sols.at(i), 1e-12);
#endif
		//mDebug() << "before Newton correction=" << before << "after=" << sols.at(i) << "after - before=" << sols.at(i) - before;
	}
	//mDebug() << "修正後の解=" << sols;
	double t = -1;
	for(size_t i = 0; i < sols.size(); ++i) {
		if(sols.at(i) > 0) {
			t = sols.at(i);
			break;
		}
	}

	if(t < 0) {
		return math::Vector<3>::INVALID_VECTOR();
	}

	auto retPoint = p + t*d;

	if(trMatrix_) {
		//mDebug() << "内部表現交点=" << retPoint;
		math::affineTransform(&retPoint, *trMatrix_);
		//mDebug() << "グローバル交点=" << retPoint;
	}
	//mDebug() << "トーラス=" << name_ << "とpos, dir=" << point << direction << "との交点は" << retPoint << "t=" << t;

	return retPoint;

	//	double a2 = a*a;
//	double b2 = b*b;
//	double A2 = a*a - b*b;
//	double A4 = A2*A2;
//	double C2 = math::dotProd(center_, center_);
//	double Dn = math::dotProd(direction, axis_);
//	double Dn2 = Dn*Dn;
//	double Dc = math::dotProd(direction, center_);
//	double Cn = math::dotProd(center_, axis_);
//	double Cn2 = Cn*Cn;
//	double Sn = math::dotProd(point, axis_);
//	double Sn2 = Sn*Sn;
//	double Sc = math::dotProd(point, center_);
//	double S2 = math::dotProd(point, point);
//	double Sd = math::dotProd(point, direction);
//	double R2 = R*R;

//	double c4 = (a2 - A2*Dn2)*(a2 - A2*Dn2);
//	double c3 = 4*(a*a - A2*Dn2)*(Sd*a2 - Dc*a2 - A2*Dn*Sn + A2*Cn*Dn);
//	double c2 = 4*Dn2*R2*a2*b2 - 2*a2*a2*a2
//	           + (4*Sd*Sd - 8*Dc*Sd - 4*Sc + 2*S2  - 2*R2 + 2*A2*Dn2 + 4*Dc*Dc + 2*C2 + 2*A2)*a2*a2
//	           + (-2*A2*Sn2 +(-8*A2*Dn*Sd + 8*A2*Dc*Dn + 4*A2*Cn)*Sn +8*A2*Cn*Dn*Sd + 4*A2*Dn2*Sc - 2*A2*Dn2*S2 + 2*A2*Dn2*R2
//	               +(-2*A2*C2 - 2*A4)*Dn2 - 8*A2*Cn*Dc*Dn - 2*A2*Cn2)*a2
//	           + 6*A4*Dn2*Sn2 - -12*A4*Cn*Dn2*Sn + 6*A4*Cn2*Dn2;
//	double c1 = (8*Dn*R2*Sn - 8*Cn*Dn*R2)*a2*b2 + (4*Dc - 4*Sd)*a2*a2*a2
//	           + (4*A2*Dn*Sn + (-8*Sc + 4*S2 - 4*R2 + 4*C2 + 4*A2)*Sd +8*Dc*Sc -4*Dc*S2 + 4*Dc*R2 - 4*A2*Cn*Dn + (-4*C2-4*A2)*Dc)*a2*a2
//	           + ( (4*A2*Dc-4*A2*Sd)*Sn2 + (8*A2*Cn*Sd + 8*A2*Dn*Sc - 4*A2*Dn*S2 + 4*A2*Dn*R2 + (-4*A2*C2-4*A2*A2)*Dn - 8*A2*Cn*Dc)*Sn
//	               -4*A2*Cn2*Sd - 8*A2*Cn*Dn*Sc + 4*A2*Cn*Dn*S2 - 4*A2*Cn*Dn*R2 +(4*A2*C2+4*A2*A2)*Cn*Dn + 4*A2*Cn2*Dc)*a2
//	           + 4*A4*Dn*Sn2*Sn - 12*A4*Cn*Dn*Sn2 + 12*A4*Cn2*Dn*Sn - 4*A4*Cn2*Cn*Dn;

//	double c0 = (-4*R2*a2*a2 + (4*R2*Sn2 - 8*Cn*R2*Sn + 4*Cn2*R2)*a2)*b2
//	           + a2*a2*a2*a2 + (4*Sc - 2*S2 + 2*R2 - 2*C2 - 2*A2)*a2*a2*a2
//	           + (2*A2*Sn2 - 4*A2*Cn*Sn + 4*Sc*Sc + (-4*S2 + 4*R2 - 4*C2 - 4*A2)*Sc + S2*S2
//	              + (-2*R2 +2*C2 +2*A2)*S2 +R2*R2 + (-2*C2-2*A2)*R2 + 2*A2*Cn2 + C2*C2 + 2*A2*C2 + A2*A2)*a2*a2
//	           + ( (4*A2*Sc - 2*A2*S2 + 2*A2*R2 - 2*A2*C2 - 2*A2*A2)*Sn2 + (-8*A2*Cn*Sc + 4*A2*Cn*S2 -4*A2*Cn*R2 + (4*A2*C2 + 4*A2*A2)*Cn)*Sn
//	               +4*A2*Cn2*Sc -2*A2*Cn2*S2 + 2*A2*Cn2*R2 + (-2*A2*C2 - 2*A2*A2)*Cn2)*a2
//	           + A4*Sn2*Sn2 - 4*A4*Cn*Sn2*Sn + 6*A4*Cn2*Sn2 - 4*A4*Cn2*Cn*Sn + A4*Cn2*Cn2;
//	mDebug() << "point=" << point << ", direction=" << direction;
//	mDebug() << "C4=" << c4 << "C3=" << c3 << "C2=" << c2 << "C1=" << c1 << "C0=" << c0;
//	auto sols = math::solve4thR(c4, c3, c2, c1, c0, false);
	//	//auto sols = math::solve4thR(1, -398, 60993.5, -4257107.5, 1.137697640625E+8, false);
}

std::vector<std::vector<geom::Plane> > geom::Torus::boundingPlanes() const
{
    using Vec3 = math::Vector<3>;
//	using Pt = math::Point;
//	auto bb = generateBoundingBox();
//	mDebug() << "in Torus::BoundingPlanes bb=" << bb.toInputString();
//    return bb.boundingSurfaces(reversed_);

	// center(), axis()はmatrixを考慮したグローバル座標系で定義されている。
	auto vPair = math::get2OrthogonalUnitVectors(axis());
	auto v1 = vPair.first, v2 = vPair.second;// v1, v2は外向き
	if(reversed_) {
		// 裏面のBoundingPlanesはxyz方向に接する面。をandでつなぐ
		return std::vector<std::vector<Plane>>{
			std::vector<Plane>{
                Plane("", -axis(), center() + axis()*a, false),  // 上面
                Plane("",  axis(), center() - axis()*a, false),  // 下面
                Plane("", -v1, center() + v1*(b+R), false),
                Plane("",  v1, center() - v1*(b+R), false),
                Plane("", -v2, center() + v2*(b+R), false),
                Plane("",  v2, center() - v2*(b+R), false),
			}
		};
	} else {
        // BBが定義できず全空間、とする場合、BSなしというのは正しい扱いではないので
        // 全空間を含むようなBSを作成して返す。一応外側内包面も作れなくは無いが、
        // BB作成のためと考えるとあまり意味はないので作成しない。
        return std::vector<std::vector<Plane>> {
            std::vector<Plane>{Plane("", Vec3{0, 0, 1}, 0, false)},
            std::vector<Plane>{Plane("", Vec3{0, 0, -1}, 0, false)},
        };
    }
}



math::Vector<3> geom::Torus::normal(const math::Point &pos) const
{
    double A2 = a*a - b*b;
    double P2 = math::dotProd(pos, pos);
    double C2 = math::dotProd(center_, center_);
    double Cn = math::dotProd(center_, axis_);
    double Pc = math::dotProd(pos, center_);
    double Pn = math::dotProd(pos, axis_);

    double factor1 = 2*a*a*(0.5*(A2 + P2 + C2 -R*R) - Pc - (a*a*a*a + A2*(Pn-Cn)*(Pn-Cn))/(2*a*a));
    std::array<double, 3> xi{{pos.x(), pos.y(), pos.z()}};
    std::array<double, 3> ci{{center_.x(), center_.y(), center_.z()}};
    std::array<double, 3> ni{{axis_.x(), axis_.y(), axis_.z()}};
    std::array<double, 3> grad;
    for(size_t i = 0; i < 3; ++i) {
        mDebug() << "xi=" << xi[i] << ", ci=" <<ci[i] << "ni=" << ni[i];
        mDebug() << "factor1=" << factor1 << "term1="
                 << factor1*(std::sqrt(pos.abs())*xi[i] - ci[i] + A2*(Cn-Pn)*ni[i]/(a*a))
                 << "term2=" << 2*b*b*R*R*(Pn-Cn)*ni[i];
        grad[i] = factor1*(std::sqrt(pos.abs())*xi[i] - ci[i] + A2*(Cn-Pn)*ni[i]/(a*a)) + 2*b*b*R*R*(Pn-Cn)*ni[i];
    }
    mDebug() << "normVec=" << math::Vector<3>(grad);
	return math::Vector<3>(grad).normalized();
}

std::unique_ptr<geom::Torus>
geom::Torus::createTorus(const std::string &name,
                         const std::vector<double> &params,
                         const math::Matrix<4> &trMatrix,
						 geom::Torus::TYPE type,
						 bool warnPhitsCompat)
{
	(void)warnPhitsCompat;
	math::Point center, axis;
	double majorR = 0, minorVR = 0, minorHR = 0;
	switch(type) {
	case Torus::TYPE::TX:
	case Torus::TYPE::TY:
	case Torus::TYPE::TZ:
		Surface::CheckParamSize(6, "T[XYZ]", params);
		center = math::Point{params.at(0), params.at(1), params.at(2)};
		majorR = params.at(3);
		minorVR = params.at(4);
		minorHR = params.at(5);
		if(type == Torus::TYPE::TX) {
			axis = math::Point{1, 0, 0};
		} else if (type == Torus::TYPE::TY) {
			axis = math::Point{0, 1, 0};
		} else if (type == Torus::TYPE::TZ) {
			axis = math::Point{0, 0, 1};
		}
		break;

	case Torus::TYPE::TA:
		Surface::CheckParamSize(9, "TA", params);
		center = math::Point{params.at(0), params.at(1), params.at(2)};
		axis = math::Point{params.at(3), params.at(4), params.at(5)};
		majorR = params.at(6);
		minorVR = params.at(7);
		minorHR = params.at(8);
		break;

	}
	if(warnPhitsCompat) {
		// axis*matrixが軸方向を向いていない場合警告する。
		auto tmpAxis = (axis*trMatrix.rotationMatrix()).normalized();
		auto xprod = math::dotProd(tmpAxis, math::Vector<3>{1,0,0});
		auto yprod = math::dotProd(tmpAxis, math::Vector<3>{1,0,0});
		auto zprod = math::dotProd(tmpAxis, math::Vector<3>{1,0,0});
		if(!utils::isSameDouble(xprod, 0) && !utils::isSameDouble(xprod, 1.0)
		&& !utils::isSameDouble(xprod, 0) && !utils::isSameDouble(yprod, 1.0)
		&& !utils::isSameDouble(zprod, 0) && !utils::isSameDouble(zprod, 1.0)){
			mWarning() << "Non-axis-parallel torus is not phits/mcnp-compatible.";
		}
	}

	//mDebug() << "Creating Torus, center=" << center << ", axis=" << axis << "Rab=" << majorR << minorVR << minorHR << "TR=" << trMatrix;
	std::unique_ptr<Torus> tor(new Torus(name, center, axis, majorR, minorVR, minorHR));
	//std::unique_ptr<Torus> tor(new Torus(name, math::Point{0, 0, 0}, math::Vector<3>{0, 0, 1}, majorR, minorVR, minorHR));
	tor->transform(trMatrix);
	return tor;
}

math::Point geom::Torus::center() const
{
	if(trMatrix_) {
		auto c = center_;
		math::affineTransform(&c, *trMatrix_);
		return c;
	} else {
		return center_;
	}
}


math::Vector<3> geom::Torus::axis() const
{
	if(trMatrix_) {
		return axis_*(trMatrix_->rotationMatrix());
	} else {
		return axis_;
    }
}

geom::BoundingBox geom::Torus::generateBoundingBox() const
{
	// (inv)Matrixを考慮しているようには見えない... がcenter(), axis()でちゃんと考慮されている。
	using Vec = math::Vector<3>;
	Vec ux{1, 0, 0}, uy{0, 1, 0}, uz{0, 0, 1};
	if(reversed_) {
		//mDebug() << "In calculating BB for cenn=" << name() << "center=" << center();
		auto c = center();
		auto ax = axis();
		assert(std::abs(ax.abs() - 1) < 1e-3);
		// 軸方向か大半径方向の大きい方の値がbox境界になる。
		double minorMax = std::max(a, b);  // 縦横小半径の大きい方
		double xwidth = std::max(std::abs(math::dotProd(a*ax, ux)), std::abs(math::crossProd(ax, ux).abs()*R + minorMax));
		double ywidth = std::max(std::abs(math::dotProd(a*ax, uy)), std::abs(math::crossProd(ax, uy).abs()*R + minorMax));
		double zwidth = std::max(std::abs(math::dotProd(a*ax, uz)), std::abs(math::crossProd(ax, uz).abs()*R + minorMax));
		return BoundingBox(c.x() - xwidth, c.x() + xwidth,
						   c.y() - ywidth, c.y() + ywidth,
						   c.z() - zwidth, c.z() + zwidth);
	} else {
		return BoundingBox::universalBox();
	}
}


#ifdef ENABLE_GUI
#include <vtkSphere.h>
#include <vtkSmartPointer.h>
#include <vtkImplicitBoolean.h>
#include <vtkPlane.h>
#include <vtkTransform.h>

#ifdef USE_SUPERQUADRIC
#include <vtkSuperquadric.h>
vtkSmartPointer<vtkImplicitFunction> geom::Torus::generateImplicitFunction() const
{
	// superquadricに切り替え
	auto torus = vtkSmartPointer<vtkSuperquadric>::New();
	torus->ToroidalOn();
	torus->SetSize(1);
	double alpha = static_cast<double>(R)/b;
	torus->SetThickness(1.0/alpha);
	torus->SetScale((alpha+1)*b, (alpha+1)*a, (alpha+1)*b);
	// vtkSuperquadricはトロイダル軸方向をy方向にしているのでこれをz方向へ回転させる行列が必要
	auto rotYtoZ = vtkSmartPointer<vtkMatrix4x4>::New();
    for(int i = 0; i < 4; ++i) for(int j = 0; j < 4; ++j) rotYtoZ->SetElement(i,j, 0);
	rotYtoZ->SetElement(0, 0,  1);
	rotYtoZ->SetElement(1, 1,  0); // cos(90deg)
	rotYtoZ->SetElement(2, 2,  0); // cos(90deg)
	rotYtoZ->SetElement(3, 3,  1);
	rotYtoZ->SetElement(1, 2, -1); //  sin(90deg)
	rotYtoZ->SetElement(2, 1,  1); // -sin(90deg)

	auto mat = vtkSmartPointer<vtkMatrix4x4>::New();
	mat->DeepCopy(rotYtoZ);

	// 今のtorusは主軸をz軸平行なので、任意方向へは回転させる。
	// geom::Torusも原点中心主軸はz軸方向なのでtransformをそのまま適用すれば良い
	// ・trMatrixを行ベクトルに対する変換なので列ベクトルのVTK形式あうように転置する。
	// ・trMatrixは粒子に適用する変換なので、surfaceに対する変換はinvMatrixの方を適用する。
	if(trMatrix_) {
		auto vtkMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
		for(int j = 0; j < 4; ++j) {
			for(int i = 0; i < 4; ++i) {
                vtkMatrix->SetElement(i, j, (invMatrix_)(static_cast<size_t>(j), static_cast<size_t>(i)));
//				vtkMatrix->SetElement(i, j, (*trMatrix_.get())(j, i));
			}
		}
		// vtkでは右からかけるからytoz回転行列を第一引数に取って先に作用させるようにする。
		vtkMatrix4x4::Multiply4x4(rotYtoZ, vtkMatrix, mat);
//		auto trf = vtkSmartPointer<vtkTransform>::New();
//		trf->SetMatrix(vtkMatrix);
//		torus->SetTransform(trf);
	}
	auto trf = vtkSmartPointer<vtkTransform>::New();
	trf->SetMatrix(mat);
	torus->SetTransform(trf);

	if(!reversed_) {
		return Surface::getVtkCompliment(torus);
	} else {
		return torus;
	}
}
#else
#include <vtkPlane.h>
#include <vtkTransform.h>
#include "component/vtk/vtkEllipticTorus/vtkEllipticTorus.h"
vtkSmartPointer<vtkImplicitFunction> geom::Torus::generateImplicitFunction() const
{
    //	auto torus = vtkSmartPointer<vtkTorus>::New();
    //	// とりあえず円形断面トーラスで妥協
    //	torus->SetLargeRadius(R);
    //	torus->SetSmallRadius(0.5*std::sqrt(a*a + b*b));
    auto torus = vtkSmartPointer<vtkEllipticTorus>::New();
    // とりあえず円形断面トーラスで妥協
    torus->SetMajorRadius(R);
    torus->SetMinorVRadius(a);
    torus->SetMinorHRadius(b);

    // 今のvtkTorusは主軸がz軸平行なので回転させる。
    // geom::Torusも原点中心主軸はz軸方向なのでtransformをそのまま適用すれば良い
    // ・trMatrixを行ベクトルに対する変換なので列ベクトルのVTK形式あうように転置する。
    // ・trMatrixは粒子に適用する変換なので、surfaceに対する変換はinvMatrixの方を適用する。
    if(trMatrix_) {
        auto vtkMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
        for(int j = 0; j < 4; ++j) {
            for(int i = 0; i < 4; ++i) {
                vtkMatrix->SetElement(i, j, (invMatrix_)(j, i));
                //				vtkMatrix->SetElement(i, j, (*trMatrix_.get())(j, i));
            }
        }
        auto trf = vtkSmartPointer<vtkTransform>::New();
        trf->SetMatrix(vtkMatrix);

        torus->SetTransform(trf);
    }

    if(!reversed_) {
        return Surface::getVtkCompliment(torus);
    } else {
        return torus;
    }
}
#endif

#endif

