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
#ifndef XYZ_HPP
#define XYZ_HPP

#include <algorithm>
#include <array>

#include "core/geometry/macro/macro.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surface/cylinder.hpp"
#include "core/geometry/surface/cone.hpp"
#include "core/geometry/surface/quadric.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/numeric_utils.hpp"



namespace geom {
namespace macro{
// マクロは展開用の関数2種類をstaticで持つだけ。

constexpr char getMnemonicChar(math::AXIS ax)
{
	return (ax == math::AXIS::X) ? 'x'
								 : (ax == math::AXIS::Y) ? 'y'
													  : 'z';
}


template<math::AXIS AX>
class AxSym {
public:
	static constexpr char mnemonic[] = {getMnemonicChar(AX), '\0'};
	static std::pair<std::string, std::string>
			expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
					std::list<inp::DataLine>::iterator &it,
					std::list<inp::DataLine> *surfInputList)
	{
		using Vec = math::Vector<3>;
		//using Matrix = math::Matrix<4>;
		std::shared_ptr<geom::Surface> expandedSurf;
		Vec axisDir = math::getAxisVector(AX);
		auto scard = inp::SurfaceCard::fromString(it->data);
		//Matrix matrix = (scard.trNumber != inp::SurfaceCard::NO_TR) ? trMap.at(scard.trNumber): Matrix();
		auto matrix = scard.getMatrixPtr(trMap);

		geom::macro::checkParamLength(scard.params, std::vector<std::size_t>{2, 4, 6}, "XYZ");

		// 軸対称面は1個面を作成する。X軸対称の場合、PX CX KX SQのうちの何れか。

		if(scard.params.size() == 2
		|| (scard.params.size() == 4 && utils::isSameDouble(scard.params.at(0), scard.params.at(2)))
		|| (scard.params.size() == 6 && utils::isSameDouble(scard.params.at(0), scard.params.at(2))
									 && utils::isSameDouble(scard.params.at(0), scard.params.at(4)) )) {
		// 1平面の場合
			// 入力2個 x1 r1 の場合 あるいは
			// 入力4個 x1 r1 x2 r2       かつx1==x2なら     PX  x1 が生成される。
			// 入力6個 x1 r1 x2 r2 x3 r3 かつx1==x2==x3なら PX  x1 が生成される。
			expandedSurf = std::make_shared<geom::Plane>(scard.name, axisDir, scard.params.at(0));
		} else if (scard.params.size() == 4) {
		// 円錐の場合
			mDebug() << "円錐やで!!!!";
			// x1 != x2 かつ r1 == r2なら円筒、
			// x1 != x2 かつ r1 != r2なら円錐
			double x1 = scard.params.at(0), x2 = scard.params.at(2);
			double r1 = scard.params.at(1), r2 = scard.params.at(3);
			if(std::abs(r1 - r2) < math::EPS) {  // x1==x2は前のブロックで確認済み
				// 円筒
				expandedSurf = std::make_shared<geom::Cylinder>(scard.name, math::Point{0, 0, 0}, axisDir, r1);
			} else {
				// 円錐
				// 名前、頂点、軸方向ベクトル、頂角、シート-1:0:1
				double grad = (r1 - r2)/(x1 - x2);  // x-r断面での面境界の勾配
				Vec dir = grad > 0 ? axisDir : -1.0*axisDir;
				expandedSurf = std::make_shared<geom::Cone>(scard.name, math::Point{(-x1*r2 + x2*r1)/(r1 - r2), 0, 0}, dir, grad, 0);
			}
		} else if (scard.params.size() == 6) {
		// 双曲面の場合
			double x1 = scard.params.at(0), x2 = scard.params.at(2), x3 = scard.params.at(4);
			double r1 = scard.params.at(1), r2 = scard.params.at(3), r3 = scard.params.at(5);
			// x1==x2==x3は既にチェック済みなのでここでは1平面の可能性は除去済み。
			// よってx座標に同じものがあった場合(x1==x2!=x3)は点が同シートにないのでエラー。
			if(utils::isSameDouble(x1, x2) || utils::isSameDouble(x2, x3)) {
				std::stringstream ss;
				ss << "Points "
				   << "(" << x1 << ", " << r1 << ") "
				   << "(" << x2 << ", " << r2 << ") "
				   << "(" << x3 << ", " << r3 << ") are not on the same sheet.";
				throw std::invalid_argument(ss.str());
			}
			/*
			 * ここまできたら双曲面SQとして面を生成する。途中のパラメータがゼロで
			 * 実質円錐等になっていても別に構わないのでチェックしない。
			 */
			double denom = 1.0/((x1 - x2)*(x2 - x3)*(x3 - x1));
			double coeff2 = (-r1*r1*(x2-x3) - r2*r2*(x3-x1) - r3*r3*(x1-x2))*denom;
			double coeff1 = (+r1*r1*(x2+x3)*(x2-x3) + r2*r2*(x3+x1)*(x3-x1) + r3*r3*(x1+x2)*(x1-x2))*denom;
			double coeff0 = (-r1*r1*x2*x3*(x2-x3) - r2*r2*x1*x3*(x3-x1) - r3*r3*x1*x2*(x1-x2))*denom;
			// 2次曲面のコンストラクタは生成はサイズ10のvectorを引数に取るものしか無いのでcreateQuadricを使う
			Vec c2 = Vec{1, 1, 1} - (1 + coeff2)*axisDir;
			Vec c1 = -0.5*coeff1*axisDir;
			std::vector<double> paramVec{c2.x(), c2.y(), c2.z(), c1.x(), c1.y(), c1.z(),   -coeff0,  0, 0, 0};
	//		std::vector<double> paramVec{-coeff2, 1, 1,   -0.5*coeff1, 0, 0,   -coeff0,  0, 0, 0}; // pxの場合

//			mDebug() << "x1r1= " << x1 << r1;
//			mDebug() << "x2r2= " << x2 << r2;
//			mDebug() << "x3r3= " << x3 << r3;
//			mDebug() << "denom=" << denom;
//			mDebug() << "coeffs2=" << coeff2 << ", coeff1=" << coeff1 << "coeff0=" << coeff0;
//			mDebug() << "residual=" << coeff2*x1*x1 + coeff1*x1 + coeff0 - r1*r1;
//			mDebug() << "ABC=" << -coeff2 << 1 << 1;
//			mDebug() << "DEF=" << -0.5*coeff1 << 0 << 0;
//			mDebug() << "G,x~y~z~="   << -coeff0 << 0 << 0 << 0;

            // 面を自動生成する時は警告はしない
            expandedSurf = geom::Quadric::createQuadric(scard.name, paramVec, math::Matrix<4>::IDENTITY(), geom::Quadric::TYPE::SQ, false);
		}
		std::vector<std::shared_ptr<geom::Surface>> surfaces{expandedSurf};
		geom::macro::replaceSurfaceInput(surfaces, matrix, it, surfInputList);
		return std::make_pair(scard.name, scard.symbol);
	}

	static void replace (const std::string &surfName,
						 const std::list<inp::DataLine>::iterator &it)
	{
		// 軸対称面は1マクロ1面なので面の名前が変わらず、よってセルの論理多項式も変わらない。
		(void) surfName;
		(void) it;
	}

};

template<math::AXIS AX> constexpr char AxSym<AX>::mnemonic[];

}  // end namespace macro
}  // end namespace geoms

#endif // XYZ_HPP
