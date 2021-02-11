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
#include "ell.hpp"


#include "core/utils/message.hpp"
#include "core/math/nvector.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/io/input/dataline.hpp"
#include "core/utils/string_utils.hpp"
#include "core/geometry/surface/quadric.hpp"

const char geom::macro::Ell::mnemonic[] = "ell";

std::pair<std::string, std::string>
	geom::macro::Ell::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
					  std::list<inp::DataLine>::iterator &it,
					  std::list<inp::DataLine> *surfInputList)
{
	using Vec = math::Vector<3>;
	auto card = inp::SurfaceCard::fromString(it->data);
	auto matrix = card.getMatrixPtr(trMap);

    //mDebug() << "Creating ELL name ===" << card.name << " from params ===" << card.params;
    // ELLの場合2次曲面1個を生成する。r
	checkParamLength(card.params, std::vector<std::size_t>{7}, mnemonic);
	math::Point focus1, focus2;  // 焦点
	double a, b; // 長軸半径、短軸半径
	if(std::abs(card.params.at(6)) < math::EPS) {
		throw std::invalid_argument("7 th input of ELL should not be 0");
	} else if(card.params.at(6) > 0) {
		/*
		 * MCNPマニュアルでは 第6引数が正の場合、値は長軸方向の"直径",
		 * みたいに書いてあるがMCNP入力では実際には長軸方向の"半径"で
		 * 実装されている？
         * "length of major axis"って半径と解釈するのが正しいってこと？ → んなこたーない。
		 *
		 * mcnpマニュアルでは
		 * ELL 0 0 -2 0 0 2 6
		 * ELL 0 0 0 0 0 3 -2
		 * An ellipsoid at the origin with major axis of length 6 in
		 * the z-direction and minor axis radius of length 4 normal to the z-axis
		 * と書かれているが前者の長軸直径は12後者の長軸直径は6で異なる。
		 * そしておそらく後者が正しい。
		 * ・前者は長軸直径と書いているが、実際は長半径として扱われている。
		 *
		 */

        // by MCNP implementation (guess)
        a = card.params.at(6);
        auto tmpFocus1 = math::Point{card.params.at(0), card.params.at(1), card.params.at(2)};
        auto tmpFocus2 = math::Point{card.params.at(3), card.params.at(4), card.params.at(5)};
        auto tmpFvec = 0.5*(tmpFocus2 - tmpFocus1);
        auto center = tmpFocus1 + tmpFvec;
        focus1 = center + tmpFvec*(a/tmpFvec.abs()-1);
        focus2 = center - tmpFvec*(a/tmpFvec.abs()-1);

//        // by MCNP manual
//        mWarning(it->pos()) << "In MCNP, ELL card with positive 7th param input has a problem."
//                            <<" major and minor radius has been doubled";
//        a = 0.5*card.params.at(6);
//        focus1 = math::Point{card.params.at(0), card.params.at(1), card.params.at(2)};
//        focus2 = math::Point{card.params.at(3), card.params.at(4), card.params.at(5)};

        if(a*a <= 0.25*math::dotProd(focus1 - focus2, focus1 - focus2)) {
            throw std::runtime_error("Major radius should be greater than minor radius");
        }
        b = std::sqrt(a*a - 0.25*math::dotProd(focus1 - focus2, focus1 - focus2));

	} else {
		b = -card.params.at(6);
		auto center = Vec{card.params.at(0), card.params.at(1), card.params.at(2)};
		auto avec = Vec{card.params.at(3), card.params.at(4), card.params.at(5)};
//        mDebug() << "center ===" << center;
        a = avec.abs();
        avec = avec.normalized();
		if(b> a) {
			focus1 = center - std::sqrt(b*b - a*a)*avec;
			focus2 = center + std::sqrt(b*b - a*a)*avec;
		} else {
			focus1 = center - std::sqrt(a*a - b*b)*avec;
			focus2 = center + std::sqrt(a*a - b*b)*avec;
		}
	}
//    mDebug() << "focus1===" << focus1;
//    mDebug() << "focus2===" << focus2;
//    mDebug() << "a===" << a << ", b===" << b;

	Vec trVec = 0.5*(focus1 + focus2);  // 並進
	Vec axVec = (focus2 - focus1).normalized();  // 長軸方向ベクトル

    // x軸をaxVecに合わせる行列を作成する。。
    math::Matrix<4> rotMatrix = math::Matrix<4>::IDENTITY();
    rotMatrix.setRotationMatrix(math::generateRotationMatrix1( axVec, Vec{1, 0, 0}));
	// 回転楕円体の中心を原点へ移動させる行列が求まっている。
    math::Matrix<4> trMatrix = math::Matrix<4>::IDENTITY();
    trMatrix.setTranslationVector(trVec);


	/*
     * b*b*X + a*a* Y + a*a* Z = 0 is implicit function.
	 */
    auto mat = math::Matrix<4>::IDENTITY()*rotMatrix*trMatrix;
//	auto quad = Quadric::createQuadric(card.name, std::vector<double>{b*b, a*a, a*a, 0, 0, 0, 0, 0, 0, -b*b*a*a}, mat, Quadric::TYPE::GQ);

//	mDebug() << "ELL is expanded to GQ " << b*b << a*a << a*a << 0 << 0 << 0 << 0 << 0 << 0 << -b*b*a*a
//			 << " with transform matrix =" << mat;
// マクロ展開で面を自動生成する時は警告を出さない

//    mDebug() << "trVec===" << trVec;
//    mDebug() << "axVec===" << axVec;
//    mDebug() << "rotMat===" << rotMatrix;
//    mDebug() << "mat===" << mat;


    std::vector<std::shared_ptr<Surface>> surfaces {
		// ELLマクロボディは1面で構成されるので面数は1つ
		Quadric::createQuadric(card.name, std::vector<double>{b*b, a*a, a*a, 0, 0, 0, 0, 0, 0, -b*b*a*a}, mat, Quadric::TYPE::GQ, false)
	};

	macro::replaceSurfaceInput(surfaces, matrix, it, surfInputList);

	return std::make_pair(card.name, card.symbol);
}

void geom::macro::Ell::replace(const std::string &surfName,
						const std::list<inp::DataLine>::iterator &it)
{
	(void) surfName;
	(void) it;
	// 回転楕円体(spheroid)は1平面置換
	// マクロボディと言いながら1平面なので名前は変更しない。


}

