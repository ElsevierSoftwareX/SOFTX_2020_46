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
#include "tor.hpp"

#include <sstream>
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/geometry/surface/torus.hpp"
#include "core/geometry/surface/plane.hpp"

const char geom::macro::Tor::mnemonic[]  = "tor";

std::pair<std::string, std::string>
geom::macro::Tor::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
                         std::list<inp::DataLine>::iterator &it,
                         std::list<inp::DataLine> *surfInputList)
{
    using Vec = math::Vector<3>;
    auto card = inp::SurfaceCard::fromString(it->data); // マクロボディのカード
    checkParamLength(card.params, std::vector<std::size_t>{9}, mnemonic);

    // TORではトーラス１、平面２を生成する。
    math::Point center{card.params.at(0), card.params.at(1), card.params.at(2)};
    double majorR = card.params.at(3), minorRh = card.params.at(4), minorRv = card.params.at(5);
    int dirFlag = static_cast<int>(card.params.at(6));
    double theta1 = card.params.at(7), theta2 = card.params.at(8);
    Vec axis, planeNormal;
    switch (dirFlag) {
    case 1:
        axis = Vec{1, 0, 0};
        planeNormal = Vec{0, 0, 1};
        break;
    case 2:
        axis = Vec{0, 1, 0};
        planeNormal = Vec{1, 0, 0};
        break;
    case 3:
        planeNormal = Vec{0, 1, 0};
        axis = Vec{0, 0, 1};
        break;
    default:
        std::stringstream ss;
        ss << "Invalid torus direction flag = " << dirFlag << ", only 1, 2, 3 are acceptable.";
        throw std::invalid_argument(ss.str());
    }
	/*
	 * TORの定義にはトーラス以外には2平面あれば良いが、平面の角度が180°以上の場合は凹セル
	 * になって論理和で定義することになり、論理多項式が変わる。これを避けるために、
	 * (角度によらず)常に凸パーツ2個で構成するよう3平面を用いる。
	 * .1 トーラス
	 * .2 開始角に対応した面 
	 * .3 終了角に対応した面
	 * .4 中間面
	 * セルは (-.1 .2 -.4):(-1 .3 -.4)
	 */
    auto torus = std::make_shared<geom::Torus>(card.name + ".1", center, axis, majorR, minorRv, minorRh);
    const std::string planeName1 = card.name + ".2";
    const std::string planeName2 = card.name + ".3";
    const std::string planeName3 = card.name + ".4";
    // FIXME 要するにこの時点でcenterを設定すると回転が狂う
    auto plane1 = std::make_shared<geom::Plane>(planeName1, planeNormal, math::Point{0,0,0});  // 開始角に対応した面
    auto plane2 = std::make_shared<geom::Plane>(planeName2, planeNormal, math::Point{0,0,0});  // 終了角に対応した面
    auto plane3 = std::make_shared<geom::Plane>(planeName3, planeNormal, math::Point{0,0,0});  // 凸セルの和にするた開始終了の間を分割する面
    auto mat1 = math::Matrix<4>::IDENTITY(), mat2 = math::Matrix<4>::IDENTITY(), mat3 = math::Matrix<4>::IDENTITY();



    mat1.setRotationMatrix(math::generateRotationMatrix2(axis, math::toRadians(theta1)));
    mat2.setRotationMatrix(math::generateRotationMatrix2(axis, math::toRadians(theta2)));
    mat3.setRotationMatrix(math::generateRotationMatrix2(axis, math::toRadians(0.5*(theta1 + theta2))));
    mat1.setTranslationVector(center);
    mat2.setTranslationVector(center);
    mat3.setTranslationVector(center);
    plane1->transform(mat1);
    plane2->transform(mat2);
    plane3->transform(mat3);


//    mDebug() << "degree1,2,3 ===" << theta1 << theta2 << 0.5*(theta1+theta2);
//    mDebug() << "rot1===" << math::generateRotationMatrix2(axis, -math::toRadians(theta1));
//    mDebug() << "rot2===" << math::generateRotationMatrix2(axis, -math::toRadians(theta2));
//    mDebug() << "rot3===" << math::generateRotationMatrix2(axis, -math::toRadians(0.5*(theta1 + theta2)));
//    mDebug() << "n1 n2 n3===" << plane1->normal() << plane2->normal() << plane3->normal();
    std::vector<std::shared_ptr<Surface>> surfaces{torus, plane1, plane2, plane3};
    macro::replaceSurfaceInput(surfaces, card.getMatrixPtr(trMap), it, surfInputList);
    return std::make_pair(card.name, card.symbol);
}

void geom::macro::Tor::replace(const std::string &macroBodyName,
                               const std::list<inp::DataLine>::iterator &it)
{
    /*
     * TORの置換は単純に全部マイナスか全部プラスではなく、マルチピースなので
     *     macro::replacCelInput(macroBodyName, numSurfaces, &(it->data));
     * は使えない。
     */
    // card.equation中の要素でsurfaceNameに完全一致する要素をreplacingで置き換える。
    auto card = inp::CellCard::fromString(it->data);
    std::string replacingString;
    std::string baseName = macroBodyName;  // +-符号を含まないマクロボディ名

    // マルチピースセルとなるので、単なる符号反転ではなく演算子も反転させる(ド・モルガン)。
    if(baseName.front() == '-') {
        baseName = baseName.substr(1);
        std::string ms1 = "-" + baseName + ".1", ms2 = "-" + baseName + ".2", ms3 = "-" + baseName + ".3";
        std::string ms4 = "-" + baseName + ".4", ps2 = baseName + ".2", ps3 = baseName + ".3", ps4 = baseName + ".4";
        std::stringstream ss;
        ss << "(" << ms1 << " " << ps2 << " " << ms4 << "):"
           << "(" << ms1 << " " << ps4 << " " << ms3 << ")";
        replacingString = "(" + ss.str() + ")";
    } else {
        if(baseName.front() == '+') baseName = baseName.substr(1);
        std::string ps1 = baseName + ".1", ps2 = baseName + ".2", ps3 = baseName + ".3", ps4 = baseName + ".4";
        std::string ms2 = "-" + baseName + ".2", ms3 = "-" + baseName + ".3", ms4 = "-" + baseName + ".4";
        std::stringstream ss;
        ss << "(" << ps1 << ":" << ms2 << ":" << ps4 << ") "
           << "(" << ps1 << ":" << ms4 << ":" << ps3 << ")";
        replacingString = "(" + ss.str() + ")";
    }
    //mDebug() << "Calling makeReplacedEquation, MB===" << macroBodyName << ", replacing===" << replacingString << ", eq===" << card.equation;
    auto newEquation = makeReplacedEquation(macroBodyName, replacingString, card.equation);

    std::string trclString;
    if(!card.trcl.empty()) trclString = " trcl=(" + card.trcl + ")";
    it->data = card.getHeaderString() + " " + newEquation + " " + card.getParamsString() + trclString;

}
