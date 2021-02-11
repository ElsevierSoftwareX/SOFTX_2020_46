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
#include "wed.hpp"

#include "core/utils/message.hpp"
#include "core/math/nvector.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/io/input/dataline.hpp"
#include "core/utils/string_utils.hpp"
#include "core/geometry/surface/plane.hpp"



const char geom::macro::Wed::mnemonic[] = "wed";

std::pair<std::string, std::string>
    geom::macro::Wed::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
                      std::list<inp::DataLine>::iterator &it,
                      std::list<inp::DataLine> *surfInputList)
{
    using Vec = math::Vector<3>;
	auto card = inp::SurfaceCard::fromString(it->data);
	auto matrix = card.getMatrixPtr(trMap);
    /*
     *  WEDの場合平面5面を生成する。
     * 1.斜面、２．v2,v3を含む、3. v1,v3を含む　　４．トップ　５．ボトム
     */
    checkParamLength(card.params, std::vector<std::size_t>{12}, mnemonic);
	math::Point vertex1{card.params.at(0), card.params.at(1), card.params.at(2)};
	Vec vec1{card.params.at(3), card.params.at(4), card.params.at(5)};
	Vec vec2{card.params.at(6), card.params.at(7), card.params.at(8)};
    Vec vech{card.params.at(9), card.params.at(10), card.params.at(11)};

	// 斜辺法線ベクトル(大きさは適当)
	Vec vechypo = vec1.abs()*vec2.normalized() + vec2.abs()*vec1.normalized();
	// 面は法線と原点からの距離、で定義する方式を使う。※法線ベクトルはWEDの外側を向く
	typedef std::shared_ptr<Surface> ptr_type;
	std::vector<ptr_type> surfaces{
		ptr_type(new Plane(card.name + ".1",   vechypo.normalized(), math::dotProd(vertex1+vec1,  vechypo.normalized()))),
		ptr_type(new Plane(card.name + ".2",  -vec1.normalized(), math::dotProd(vertex1,      vec1.normalized()))),
		ptr_type(new Plane(card.name + ".3",  -vec2.normalized(), math::dotProd(vertex1,      vec2.normalized()))),
		ptr_type(new Plane(card.name + ".4",   vech.normalized(), math::dotProd(vertex1+vech, vech.normalized()))),
		ptr_type(new Plane(card.name + ".5",  -vech.normalized(), math::dotProd(vertex1,     -vech.normalized()))),
	};

	macro::replaceSurfaceInput(surfaces, matrix, it, surfInputList);
	return std::make_pair(card.name, card.symbol);
}

void geom::macro::Wed::replace(const std::string &surfName, const std::list<inp::DataLine>::iterator &it)
{
    it->data = macro::replacCelInput(surfName, numSurfaces, it->data);
}

