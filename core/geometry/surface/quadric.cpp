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
#include "quadric.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "plane.hpp"
#include "quadric.bs.hpp"
#include "cylinder.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/utils/message.hpp"
#include "core/utils/numeric_utils.hpp"

namespace {
template<class T> using vec = std::vector<T>;
using bp_set_type = std::vector<std::vector<geom::Plane>> ;
}

geom::Quadric::Quadric(const std::string &name, const std::vector<double> &params)
	:Surface("QUADRIC", name)
{
	if(params.size() != 10) {
		throw std::invalid_argument(std::string("Quadric surface requires 10 params but ")
									+ std::to_string(params.size()) + " are gvien.");
	}
    /*
     * リファクタリング
     * 背景：
     *   constructCsgModelの実装にあたっては二次形式の標準化が必要となる。
     *   これはboundingPlanes()と大半が重複するのでこれを共通化するためのリファクタリングが必要。
     *
     * とりあえずメンバ変数のmatrixを除去してtransform()を適用した場合、
     * 保持しているmatrixではなく直接A〜Kの係数に適用されるように変更した。
     *
     * 案１：
     * 標準化行列(対角化行列)を保持して、クロスターム係数D,E,Fをゼロ(あるいは存在自体を消す)にする。
     *
     * 案２：
     * 係数自体は保持し、前後、交差判定にはそれを使う。それとは別個にboundingPlane計算と
     * CGSモデル作成のためにmatrixを保持する。このmatrixは粒子との交差判定などには使用しない。
     *
     *
     * 案２で。抜本的にやるのであればQuadricを廃してElipticCylinder, ParabolicCylinderなどに
     * 分けるところまで考えるべきで、とりあえずは変更を最小に留める案２で行く。
     *
     * コンストラクタではどこまで計算するか？
     * ・対角化行列
     * 面種の判定と特徴パラメータを算出する関数をboundingPlanes()とcreateCsgModel()で共有する
     */


	A_ = params.at(0);
	B_ = params.at(1);
	C_ = params.at(2);
	D_ = params.at(3);
	E_ = params.at(4);
	F_ = params.at(5);
	G_ = params.at(6);
	H_ = params.at(7);
	J_ = params.at(8);
	K_ = params.at(9);
//	matrix_ = std::unique_ptr<math::Matrix<4>>(new math::Matrix<4>(math::Matrix<4>::IDENTITY()));
//	invMatrix_ = math::Matrix<4>::IDENTITY();
    boundingPlaneVectors_ = boundingPlanes();
}

std::shared_ptr<geom::Surface> geom::Quadric::makeDeepCopy(const std::string &newName) const
{
    auto newSurface = std::make_shared<Quadric>(newName, std::vector<double>{A_, B_, C_, D_, E_, F_, G_, H_, J_, K_});
//    if(this->matrix_) newSurface->transform(invMatrix_);
    return newSurface;
}


std::string geom::Quadric::toInputString() const
{
	std::stringstream ss;
	ss <<  name()  << " gq" << " " <<  A_ << " " << B_ << " " << C_ << " " << D_
	   << " " << E_ << " " << F_ << " " << G_ << " " << H_ << " " << J_ << " " << K_ << " ";


//	// TRSFはそのSurfaceに対するTransformなので、ここでは逆行列の方を書き出す。
//	if(matrix_) {
//		ss << " trsf=(";
//		auto trans = invMatrix_.translationVector();
//		ss << trans.x() << " " << trans.y() << " " << trans.z() << " ";
//		auto rot = invMatrix_.rotationMatrix();
//		for(size_t i = 0; i < 3; ++i) {
//			for(size_t j = 0; j < 3; ++j) {
//				ss << rot(i, j);
//				if(i != invMatrix_.size()-1 || j != invMatrix_.size()-1) ss << " ";  // 最後以外はスペースを入れる。
//			}
//		}
//		ss << ")";
//	}
	return ss.str();
}

std::unique_ptr<geom::Surface> geom::Quadric::createReverse() const
{
	std::unique_ptr<Surface> reversedQuadric(new Quadric(Surface::reverseName(name_), parameters()));
	reversedQuadric->setID(ID_*-1);
//	if(matrix_) {
//		// Quadric::transformの引数は面のTrマトリックス。
//		// matrix_は粒子位置のTrマトリックスなので逆行列を渡す
//		reversedQuadric->transform(matrix_->inverse());
//	}
	return reversedQuadric;
}

std::string geom::Quadric::toString() const
{
	std::stringstream ss;
	ss << Surface::toString() << ", A=" << A_ << ", B=" << B_ << ", C=" << C_ << ", D=" << D_ << ", E="
	   << E_ << ", F=" << F_ << ", G=" << G_ << ", H=" << H_ << ", J=" << J_ << ", K=" << K_;
//	if(matrix_) {
//		ss << ", particleTr=" << *(matrix_.get()) << ", inv=" << invMatrix_;
//	}
	return ss.str();
}

void geom::Quadric::transform(const math::Matrix<4> &matrix)
{
    Surface::transform(matrix);  // Surface::transformはBoundingSurfacesをtransformする。

    /*
     * 新たなtransform実装はメンバ変数で変換行列を持つのではなく、
     * 直接陰関数の係数を変更する。
     */
    double a11 = matrix(0,0), a12 = matrix(0,1), a13 = matrix(0,2);
    double a21 = matrix(1,0), a22 = matrix(1,1), a23 = matrix(1,2);
    double a31 = matrix(2,0), a32 = matrix(2,1), a33 = matrix(2,2);
    double ll1 = matrix(3,0), ll2 = matrix(3,1), ll3 = matrix(3,2);
    double L1 = ll1*a11 + ll2*a12 + ll3*a13;
    double L2 = ll1*a21 + ll2*a22 + ll3*a23;
    double L3 = ll1*a31 + ll2*a32 + ll3*a33;

//    mDebug() << a11 << a12 << a13 << 0;
//    mDebug() << a21 << a22 << a23 << 0;
//    mDebug() << a31 << a32 << a33 << 0;
//    mDebug() << ll1 << ll2 << ll3 << 1;


    double nA = a11*a11*A_ + a21*a21*B_ + a31*a31*C_ + a11*a21*D_ + a21*a31*E_ + a11*a31*F_;
    double nB = a12*a12*A_ + a22*a22*B_ + a32*a32*C_ + a12*a22*D_ + a22*a32*E_ + a12*a32*F_;
    double nC = a13*a13*A_ + a23*a23*B_ + a33*a33*C_ + a13*a23*D_ + a23*a33*E_ + a13*a33*F_;
    double nD = 2*a11*a12*A_ + 2*a21*a22*B_ + 2*a31*a32*C_
        + a11*a22*D_ + a12*a21*D_ + a21*a32*E_ + a22*a31*E_ + a11*a32*F_ + a12*a31*F_;
    double nE = 2*a12*a13*A_ + 2*a22*a23*B_ + 2*a32*a33*C_
        + a12*a23*D_ + a13*a22*D_ + a22*a33*E_ + a23*a32*E_ + a12*a33*F_ + a13*a32*F_;
    double nF = 2*a11*a13*A_ + 2*a21*a23*B_ + 2*a31*a33*C_
        + a11*a23*D_ + a13*a21*D_ + a21*a33*E_ + a23*a31*E_ + a11*a33*F_ + a13*a31*F_;
    double nG = -2*L1*a11*A_ -2*L2*a21*B_ -2*L3*a31*C_
        - L1*a21*D_ -L2*a11*D_ -L2*a31*E_ -L3*a21*E_ -L1*a31*F_ - L3*a11*F_
        + a11*G_ + a21*H_ + a31*J_;
    double nH = -2*L1*a12*A_ -2*L2*a22*B_ -2*L3*a32*C_
        -L1*a22*D_ -L2*a12*D_-L2*a32*E_ -L3*a22*E_ -L1*a32*F_-L3*a12*F_
        +a12*G_ + a22*H_ + a32*J_;
    double nJ = -2*L1*a13*A_ -2*L2*a23*B_ -2*L3*a33*C_
        -L1*a23*D_ -L2*a13*D_ -L2*a33*E_ -L3*a23*E_ -L1*a33*F_ -L3*a13*F_
        +a13*G_ +a23*H_ + a33*J_;
    double nK = L1*L1*A_ + L2*L2*B_ + L3*L3*C_
        + L1*L2*D_ + L2*L3*E_ + L1*L3*F_
        -L1*G_ -L2*H_ -L3*J_ + K_;
    A_ = nA;
    B_ = nB;
    C_ = nC;
    D_ = nD;
    E_ = nE;
    F_ = nF;
    G_ = nG;
    H_ = nH;
    J_ = nJ;
    K_ = nK;
//	/*
//	 * 注意！このmatrix(=大体はTRカード入力)はオブジェクトをTransformするmatrixである。
//	 * このため粒子に適用するmatrixはその逆行列となる。
//	 * 通常のSurfaceではSurfaceの諸量(center等)をtransformsするが、
//	 * Quadricでは粒子の方をtransformsする。
//	 *
//	 */
//	typedef math::Matrix<4> Mat;
//	if(!math::isSameMatrix(matrix, Mat::ZERO())) {
//		if(!matrix_) {
//			matrix_ = std::unique_ptr<Mat>(new Mat(Mat::IDENTITY()));
//		}
//		// 引数のmatrixは通常の面をtransformするものであるのに対し、
//		// メンバ変数matrix_は粒子位置をtransformする。
//		// ゆえにmatrixは.inverseの方をmatrix_へ加える。
//		// しかも作用順も逆になる。
//		//*(matrix_.get()) = *(matrix_.get()) * matrix.inverse(); 間違い
//		*(matrix_.get()) = matrix.inverse()* (*(matrix_.get()));

//		invMatrix_ = matrix_->inverse();
////        if(this->isReversed()) {
////            mDebug() << "Quadric::transform for surface===" << this->name_;
////            mDebug() << "argument matrix===" << matrix;
////            mDebug() << "matrix=" << *(matrix_.get());
////            mDebug() << "inv   =" << invMatrix_;
////        }
//	}
}

bool geom::Quadric::isForward(const math::Point &p) const
{
	auto pos = p;
//	if(matrix_) {
//		math::affineTransform(&pos, *(matrix_.get()));
//	}
	const double x = pos.x(), y = pos.y(), z = pos.z();

	return (reversed_) ?  A_*x*x + B_*y*y + C_*z*z + D_*x*y + E_*y*z + F_*z*x + G_*x + H_*y + J_*z + K_ < 0
						: A_*x*x + B_*y*y + C_*z*z + D_*x*y + E_*y*z + F_*z*x + G_*x + H_*y + J_*z + K_ >= 0;

}

math::Point geom::Quadric::getIntersection(const math::Point &point, const math::Vector<3> &direction) const
{
	assert(utils::isSameDouble(direction.abs(), 1.0));

	math::Point p = point;
	math::Vector<3> d = direction;
//	if(matrix_) {
//		math::affineTransform(&p,  *(matrix_.get()));
//		d = d*matrix_->rotationMatrix();
//	}
//	mDebug() << "orgPos=" << point << ", tredPos=" << p;
//	mDebug() << "orgDir=" << direction << ", tredDir=" << d;

	double d1 = d.x();
	double d2 = d.y();
	double d3 = d.z();
	double x = p.x();
	double y = p.y();
	double z = p.z();
//	double c2 = A*d1*d1 + B*d2*d2 + C*d3*d3 + D*d1*d2 + E*d2*d3 + F*d1*d3;
//	double c1 = 2*A*d1*x +2*B*d2*y + 2*C*d3*z
//			+ D*(d1*y+d2*x) + E*(d2*z+d3*y) + F*(d1*z + d3*x)
//			+ G*d1 + H*d2 + J*d3;

	double c2 = C_*d3*d3  + E_*d2*d3 + F_*d1*d3 + B_*d2*d2  + D_*d1*d2 + A_*d1*d1;
	double c1 = 2*C_*d3*z + E_*d2*z + F_*d1*z + E_*d3*y + 2*B_*d2*y + D_*d1*y + F_*d3*x + D_*d2*x + 2*A_*d1*x + J_*d3 + H_*d2 + G_*d1;
	double c0 = A_*x*x + B_*y*y + C_*z*z + D_*x*y + E_*y*z + F_*x*z + G_*x + H_*y + J_*z + K_;


	double disc = c1*c1 - 4*c2*c0;  // discriminant

	//mDebug() << "discriminant ==== " << disc;
	// 解なしあるいは接している場合は交点なしを返す
	if(disc <= 0) {
		return math::Point::INVALID_VECTOR();
	}

	// c2が0なら解は1つ
	if(utils::isSameDouble(c2, 0)) {
		double t = -c0/c1;
		if(t < 0)  {
			return math::Point::INVALID_VECTOR();
		} else {
			auto intersection = p + t*d;
//			if(matrix_) math::affineTransform(&intersection, invMatrix_);
			return intersection;
		}
	}
	double t1 = 0.5*(-c1 + std::sqrt(disc))/c2;
	double t2 = 0.5*(-c1 - std::sqrt(disc))/c2;

	double large, small;
	if(t1 < t2) {
		large = t2;
		small = t1;
	} else {
		large = t1;
		small = t2;
	}

	//mDebug()<< "c2, c1, c0=" << c2 << c1 << c0;
	// 最も進行方向先の方の解がマイナスなら2つの回はどちらも後方なので交点なし
	if(large < 0) return math::Point::INVALID_VECTOR();

	// あとは前方の近い方を選択する。
//	mDebug() << "inv=" << invMatrix_;
	math::Point retpos = (small > 0) ? p + small*d : p + large*d;
//	mDebug() << "orgIntersection=" << retpos;
	// Transformされている場合は逆変換してグローバル座標系に戻す
//	if(matrix_) math::affineTransform(&retpos, invMatrix_);
//	mDebug() << "tredIntersection=" << retpos;
	return retpos;
}



// TODO boundingPlanesは本来、面数の回数だけ実行すれば十分であるが、今はセルで使われている回数だけ呼ばれて無駄
// Surfaceをインスタンス化する時に計算して、メンバ変数へ代入しておけばいい。
std::vector<std::vector<geom::Plane> > geom::Quadric::boundingPlanes() const
{
    using Vec3 = math::Vector<3>;
    //using Matrix4 = math::Matrix<4>;
    // 全空間指定に対応する内包面セット
    static const std::vector<std::vector<Plane>> WHOLESPACE {
        std::vector<Plane>{Plane("", Vec3{1, 0, 0}, 0)},
        std::vector<Plane>{Plane("", Vec3{-11, 0, 0}, 0)},
    };
    // 符号数算出のための正値判定関数
    //auto IsNonzeroPositive = [](double d) {return d > math::EPS;};

    //mDebug() << "surface ===" << name_ << "のBoundingPlanes計算開始";

    // 二次形式(quadratic form)の主要部(principal)行列
    math::Matrix<3> qfPrincipal{
		A_,     0.5*D_, 0.5*F_,
		0.5*D_, B_,     0.5*E_,
		0.5*F_, 0.5*E_,     C_
	};
    // 二次形式の行列
    math::Matrix<4> qfMatrix{
		A_,     0.5*D_, 0.5*F_, 0.5*G_,
		0.5*D_, B_,     0.5*E_, 0.5*H_,
		0.5*F_, 0.5*E_,     C_, 0.5*J_,
		0.5*G_, 0.5*H_, 0.5*J_, K_
	};
    const int rankPrincipal = qfPrincipal.rank(), rankMatrix = qfMatrix.rank();

    std::vector<double> evalues, evalues2;
    std::vector<math::Vector<3>> evectors;
    std::vector<math::Vector<4>> evectors2;
    // 行列、主要部行列の符号数
    qfPrincipal.symEigenVV(&evalues, &evectors, true, true);
    qfMatrix.symEigenVV(&evalues2, &evectors2, true, true);
//    mDebug() << "主要部行列 ===" << qfPrincipal;
//    mDebug() << "主要部rank ===" << rankPrincipal << ", signature ===" << sigPrincipal;
//    mDebug() << "主要部固有値 ===" << evalues;
//    mDebug() << "主要部固有ベクトル===";
//     for(const auto& vec: evectors) mDebug() << vec;
//    mDebug() << "主要部固有ベクトル1" <<  evectors.at(0);
//    mDebug() << "主要部固有ベクトル2" <<  evectors.at(1);
//    mDebug() << "主要部固有ベクトル3" <<  evectors.at(2);
//    mDebug() << "";
//    mDebug() << "二次形式行列 =" << qfMatrix;
//    mDebug() << "二次形式rank ===" << rankMatrix << ", signature ===" << sigMatrix;
//    mDebug() << "二次形式固有値===" << evalues2;
//    mDebug() << "二次形式固有ベクトル===";
//    for(const auto& vec: evectors2) mDebug() << vec;

	/*
	これチェックしたいけど現実的にわりと数値誤差乗るので微妙な感じになる。
	厳密なチェックは無理なのでロバストに対応するしかない
	assert(rank1 == std::get<0>(sigPrincipal) + std::get<1>(sigPrincipal));
	assert(rank2 == std::get<0>(sigMatrix) + std::get<1>(sigMatrix));
	*/

    //  quadric:符号数の和とrankが数値誤差で合わない場合がある。

	/* 回転行列は固有ベクトルを行ベクトルとしてPを作成した場合 P    *A* P^-1 が対角行列となる
	* 同様に    固有ベクトルを列ベクトルとしてPを作成した場合 P^-1 *A* P    が対角行列となる。

	* 教科書の定式化では標準化空間への変換は位置ベクトルを列ベクトルで表し、
	* Pは固有列ベクトルから作成している。つまり変換は以下の式である。
	*  (x)     (x')   (l)
	*  (y) = P (y') + (m)
	*  (z)     (z')   (n)
	* 標準化空間で生成されたBounding Planeはこの回転行列Pと並進ベクトルの逆変換を適用する必要がある。
	*
	* 当コードでどう扱うかであるが、定式化は自分でやったところも含めて列ベクトルベースでやっている。
	* 従って実装もそれに従い、PAP-1が対角行列となるように、対角化行列(=回転行列を決める)
	* ・アフィン変換の引数となる行列は行ベクトルに対する変換行列であることに留意
	*/
	// MatrixのコンストラクタでVectorのarrayを採る場合は行ベクトルからの生成に相当する
	// 従って転置して、列ベクトルで生成したことと同値にする。
	// よって rotation.inverse * A * rotation が対角行列
    auto rotation = (math::Matrix<3>{std::array<math::Vector<3>, 3>{evectors.at(0), evectors.at(1), evectors.at(2)}}).transposed();

	// ここまでは共通化可能。
//    mDebug() << "実空間→標準化空間への回転行列(列ベクトル用) P ====" << rotation;
//    mDebug() << "主要部対角化結果 tP*A*P===" << rotation.transposed()*qfPrincipal*rotation;
	// 数値誤差で失敗する可能性は割とあるのでリリースではチェックしない。
	//assert(diagonalMatrix.isDiagonal());



	// 返り値などの保存用データ
	BoundingBox bb;
    std::vector<std::vector<geom::Plane>> boundingPlaneVecs;
    std::unique_ptr<math::Matrix<4>> normToRealMatrix;
	/*
	 *  rotationは実空間の基底列ベクトルを標準化空間の基底列ベクトルへ変換するものなので、
	 * 点(=位置"列"ベクトル)等は P^-1*cvecを適用するに等しい。
	 * 従って、標準化空間から実空間へ位置"列"ベクトルを変換する場合は更にその逆の変換Pを適用することになる。
	 * 当コードではVectorは行ベクトルなので変換はその転置 rvec*tPを適用することになる。
	 */

    /*
     * boundingPlaneVecsを返す
     * // 主要部のランクが３の場合のboudingPlaneVecsを返す。
     * getBoundingPlaneVectors3(const math::Matrix &qfPrincipal, const math::Matrix &qfMatrix, rankMatrix
     */

    /*
     * 以下のrank,符号数による場合分けの中では
     * 1.標準化座標系でのbounding surface vectorを boundingVectorsへ代入
     * 2. 標準化座標系 → 実座標系への変換行列tMatrixを修正(あれば)
     *
     * ループを抜けてから、boundingPlaneVecsの各要素にtMatrixを適用する。
     */

    // 主要部のランク(rank)でまず場合分けし、次に二次形式行列のランク(rank2)で場合分けする。
	// ネストは深くなるが共通部分を増やせる。はず。
    if(rankPrincipal == 3) {
        /*
         * rankPrincipal が 3なら主要部はフルランクで、主要部の逆行列で一次の項を消すように並進変換が求まる。
         */
        // translationは実空間→標準化空間の並進として定義する。
        math::Vector<3> translation = math::Vector<3>{0.5*G_, 0.5*H_, 0.5*J_}*qfPrincipal.inverse().transposed();
        // rankPrincipal == 3 なら gqPrincipalはフルランクなのでdet != 0
        // よってdetは厳密にはチェックしない。固有値のゼロ判定とdetのゼロ判定を首尾一貫して矛盾なく定めるのは意外と難しい。
        double d = qfMatrix.determinant()/qfPrincipal.determinant();

        CalcBoundingPlaneVectors_rank3(name_, reversed_, rankMatrix, translation, rotation, evalues, d,
                                       boundingPlaneVecs, bb);



    } else if (rankPrincipal == 2) {
        /*
         * 主要部のランクが2ならばランク落ちしていて主要部行列の逆行列は存在せず、
         * 標準化空間での並進移動もrankMatrix依存なので暫定的な平行移動pqrを与える。
         */
		double d = K_;
		// ここは本来列ベクトルにたいしてrotation.transposed()を作用させるところであるが、
		// pqrは行ベクトルなのでtransposedのtransposedで元に戻ったものが後ろから作用させられる。
        Vec3 pqr = math::Vector<3>{0.5*G_, 0.5*H_, 0.5*J_}*rotation;
        CalcBoundingPlaneVectors_rank2(name_, reversed_, rankMatrix, pqr, rotation, evalues, d, boundingPlaneVecs);


    } else if (rankPrincipal == 1) {
        Vec3 pqr = math::Vector<3>{0.5*G_, 0.5*H_, 0.5*J_}*rotation;
        double d = K_;
        CalcBoundingPlaneVectors_rank1(name_, reversed_, rankMatrix, pqr, rotation, evalues, d, boundingPlaneVecs);

    } else if (rankPrincipal == 0) {
        // 主要部ランクが0、つまり主要部行列がゼロ行列の場合GQは単なる1平面
        // 標準化の必要なし。標準化空間→実空間への変換も不要。
        if(rankMatrix == 2) {
            mDebug() << "平面";
            // rank(~A)=2ならl,m,nのいずれかは非ゼロなので1平面
            auto normal = Vec3{G_, H_, J_};
            if(reversed_) {
                boundingPlaneVecs = bp_set_type{vec<geom::Plane>{geom::Plane("", -normal, K_)}};
            } else {
                boundingPlaneVecs = bp_set_type {vec<geom::Plane>{geom::Plane("", normal, -K_)}};
            }
        } else if(rankMatrix == 1) {
            // rank(~A)=1ならl=m=n=0
            throw std::runtime_error("Invalid implicit function. All the coefficients are 0.");
        } else if(rankMatrix == 0) {
            // rank(~A)=0なら係数、定数項全て0なので図形として意味なし。おかしい。
            throw std::runtime_error("Invalid implicit function. All the coefficients and constants are 0.");
        }
    }
	// おわり

	if(boundingPlaneVecs.empty()) {
		mWarning() << "Calculation of boundingPlanes() for Quadric is not implemented.";
        return std::vector<std::vector<geom::Plane>> {std::vector<Plane>{}};
	} else {
		// matrix_, invMatrixがある場合はinvMatrixを適用する。
		// 適用順としてはinvMatrixは最後になるのでここでまとめて実行する。
//		if(matrix_) {
//			for(auto& planeVec: boundingPlaneVecs) {
//				for(auto &plane: planeVec) {
//					plane.transform(invMatrix_);
//				}
//			}
//		}
        // bb使用していないのでは？
        //bb.transform(*normToRealMatrix.get());
        //if(matrix_) bb.transform(invMatrix_);

		return boundingPlaneVecs;
    }
}


std::unique_ptr<geom::Quadric>
geom::Quadric::createQuadric(const std::string &name,
							 const std::vector<double> &params,
							 const math::Matrix<4> &trMatrix,
							 geom::Quadric::TYPE type,
							 bool warnPhitsCompat)
{
	(void) warnPhitsCompat;
	math::Matrix<4> matrix = trMatrix;
	std::vector<double> args(10);
	switch(type) {
	case Quadric::TYPE::SQ:
		Surface::CheckParamSize(10, "SQ", params);
		args.at(0) = params.at(0);
		args.at(1) = params.at(1);
		args.at(2) = params.at(2);
		args.at(3) = 0;
		args.at(4) = 0;
		args.at(5) = 0;
		args.at(6) = 2*params.at(3);
		args.at(7) = 2*params.at(4);
		args.at(8) = 2*params.at(5);
		args.at(9) = params.at(6);
		if(math::isSameMatrix(matrix, math::Matrix<4>())) {
			matrix = math::Matrix<4> {
			    1, 0, 0, 0,
			    0, 1, 0, 0,
			    0, 0, 1, 0,
			    params.at(7), params.at(8), params.at(9), 1
		    } * math::Matrix<4>::IDENTITY();

		} else {
			matrix = math::Matrix<4> {
			    1, 0, 0, 0,
			    0, 1, 0, 0,
			    0, 0, 1, 0,
			    params.at(7), params.at(8), params.at(9), 1
		    }* trMatrix;
		}
		break;

	case Quadric::TYPE::GQ:
		Surface::CheckParamSize(10, "GQ", params);
		args = params;
		break;
	}

	std::unique_ptr<Quadric> quad(new Quadric(name, args));
	quad->transform(matrix);
	return quad;
}

#ifdef ENABLE_GUI
#include <vtkQuadric.h>
#include <vtkTransform.h>
vtkSmartPointer<vtkImplicitFunction> geom::Quadric::generateImplicitFunction() const
{
	auto quadric = vtkSmartPointer<vtkQuadric>::New();
	quadric->SetCoefficients(A_, B_, C_, D_, E_, F_, G_, H_, J_, K_);

//	auto trf = vtkSmartPointer<vtkTransform>::New();
//	// vtkMatrixは並びはrow-majorだが、vectorを縦ベクトルとして
//	// Matrixを左から掛けるようになっている。故に転置する必要がある。
//	auto vtkMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
//	for(int j = 0; j < 4; ++j) {
//		for(int i = 0; i < 4; ++i) {
//            vtkMatrix->SetElement(i, j, (*matrix_)(static_cast<size_t>(j), static_cast<size_t>(i)));
//		}
//	}
//	trf->SetMatrix(vtkMatrix);
//	quadric->SetTransform(trf);
	// vtkでのquadricは法線の向きは同じ
	if(reversed_) {
		return quadric;
	} else {
		return Surface::getVtkCompliment(quadric);
	}
}
#endif
