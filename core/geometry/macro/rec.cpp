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
#include "rec.hpp"

#include <memory>

#include "core/utils/message.hpp"
#include "core/math/nvector.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surface/quadric.hpp"
#include "core/geometry/surface/surface.hpp"

const char geom::macro::Rec::mnemonic[]  = "rec";

std::pair<std::string, std::string> geom::macro::Rec::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
															 std::list<inp::DataLine>::iterator &it,
															 std::list<inp::DataLine> *surfInputList)
{
	using Vec = math::Vector<3>;
	auto card = inp::SurfaceCard::fromString(it->data);
	auto matrix = card.getMatrixPtr(trMap);

	// RCCの場合GQ楕円筒1面、平面2面を生成する。
	checkParamLength(card.params, std::vector<std::size_t>{10, 12}, mnemonic);
	math::Point baseCenter{card.params.at(0), card.params.at(1), card.params.at(2)};
	Vec axis{card.params.at(3), card.params.at(4), card.params.at(5)};
	Vec majorAxis{card.params.at(6), card.params.at(7), card.params.at(8)}, minorAxis;

	// major axisとaxisは直交していないといけない。
	if(!math::isOrthogonal(axis, majorAxis)) {
        std::stringstream ss;
        ss << it->pos() <<  " In REC card, cylinder axis height vector("
           << axis << ") and major radius axis vector(" << majorAxis << ") should be orthogonal";
        throw std::invalid_argument(ss.str());
	}

	if(card.params.size() == 10) {
		minorAxis = card.params.at(9)*math::crossProd(axis, majorAxis).normalized();
	} else if(card.params.size() == 12) {
		minorAxis = Vec{card.params.at(9), card.params.at(10), card.params.at(11)};
	}

	// minorAxisはaxis, majorAxis両者と直行している必要がある。
	if(!math::isOrthogonal(axis, minorAxis)) {
        std::stringstream ss;
        ss << it->pos() << " In REC card, cylinder axis height vector(" << axis
           << ") and minor radius axis vector(" << minorAxis << ") should be orthogonal";
        throw std::invalid_argument(ss.str());
	} else if(!math::isOrthogonal(minorAxis, majorAxis)) {
        std::stringstream ss;
        ss << it->pos() << " In REC card, major radius vector(" << majorAxis
           <<  ") and minor radius axis vector(" << minorAxis << ") should be orthogonal";
        throw std::invalid_argument(ss.str());
	}

	auto hvec = axis.normalized();
	mDebug() << "major, minor Axis ===" << majorAxis << minorAxis;

	/*
	 * ここで二次曲面のパラメータを与える。与えるのは
	 * [xyz]の二次の係数、xy, yz, zx, x, y, z, の係数、 定数項 の順。
	 *
	 * 一方、内部表現では底面中心や長短半径ベクトルを保持している。
	 * これは座標系に依存しないので陰関数の計算時には楽ではあるが、
	 * GQ面の入力に戻してやる必要がある。
	 * 一番手っ取り早いのはaxis方向をz軸、長軸方向をx, 短軸方向をyとして定式化し、
	 * 生成した面に元の座標系と合うようにアフィン変換すれば良い。
	 *
	 * そうすると陰関数は
	 * b^2 x^2 + a^2 y^2 -1 となる。
	 */
	double a = majorAxis.abs(), b = minorAxis.abs();
	std::vector<double> quadParams{b*b, a*a, 0, 0, 0, 0, 0, 0, 0, -a*a*b*b};
	std::shared_ptr<Quadric> quad = std::make_shared<Quadric>(card.name + ".1", quadParams);
	math::Matrix<4> qmatrix = math::Matrix<4>::IDENTITY();
	// x軸aを合わせる回転
	// generateRotationMatrix1は第二引数ベクトルを第一引数ベクトルへ回転する行列を生成する
	// ここではmajorAxisをx軸へ, minorAxisをy軸へ、axisをz軸へ回転させる
	// majorとminorは必ず直行しているので、2方向を合わせればあとは自然と一致する。
	// しかも、axisとmajorAxisは直交している(はず)のでこれらの回転方向も直交しているため、
	// 単にそれぞれの回転行列を積で合成しても問題ない。
	math::Matrix<3> zmat = generateRotationMatrix1(Vec{0, 0, 1}, axis);
	math::Matrix<3> xmat = generateRotationMatrix1(Vec{1, 0, 0}, majorAxis);
	auto rotMat = zmat*xmat;
//	mDebug() << "axis ===" << axis;
//	mDebug() << "axis*zmat===" << axis*zmat;
//	mDebug() << "axis*rotmat===" << axis*rotMat;
//	mDebug() << "majorAxis===" << majorAxis;
//	mDebug() << "majorAxis*xmat ===" << majorAxis*xmat;
//	mDebug() << "majorAxis*rotmat===" << majorAxis*rotMat;
//	mDebug() << "zmat===" << zmat;
//	mDebug() << "xmat===" << xmat;
//	mDebug() << "rotMat===" << rotMat;
	qmatrix.setRotationMatrix(rotMat);
	qmatrix.setTranslationVector(baseCenter);



	/*
	 * ここまでではquadは定式化空間での楕円筒曲面
	 *
	 * matrix: 入力ファイルでの明示的TRSFによるアフィン変換行列
	 * qmatrix: 実座標系から定式化座標系への変換行列
	 *
	 * matrix はGQとPlane両方に適用する必要がある。
	 * qmatrixはGQのみに適用
	 *
	 * ゆえにquad->transformしてからreplacing関数にmatrixを与える
	 */
	quad->transform(qmatrix.transposed()); // qmatrixは実→定式化座標の変換なのでsurfaceにはその逆変換を適用する。

	mDebug() << "REC円筒の軸をz方向へ回転する行列の転置===" << qmatrix;
	// RECは.1が楕円筒側面、2が上面、3が底面。裏面の法線が内部を向くようにする。
	std::vector<std::shared_ptr<Surface>> surfaces{
		quad,
		std::make_shared<Plane>(card.name + ".2",  hvec, math::dotProd(baseCenter, hvec) + axis.abs()),
		std::make_shared<Plane>(card.name + ".3", -hvec, -math::dotProd(baseCenter, hvec))
	};

	macro::replaceSurfaceInput(surfaces, matrix, it, surfInputList);

	return std::make_pair(card.name, card.symbol);
}




void geom::macro::Rec::replace(const std::string &surfName, const std::list<inp::DataLine>::iterator &it)
{
    it->data = macro::replacCelInput(surfName, numSurfaces, it->data);
}

