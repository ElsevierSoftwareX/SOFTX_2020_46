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
#include "box.hpp"

#include <algorithm>
#include <array>

#include "core/utils/string_utils.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/utils/message.hpp"
#include "core/math/nvector.hpp"
#include "core/geometry/surface/plane.hpp"



const char geom::macro::Box::mnemonic[]  = "box";

std::pair<std::string, std::string> geom::macro::Box::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
														std::list<inp::DataLine>::iterator &it,
														std::list<inp::DataLine> *surfInputList)
{
	using Vec = math::Vector<3>;
	auto card = inp::SurfaceCard::fromString(it->data);
	auto matrix = card.getMatrixPtr(trMap);
    // 9-arg-BOX create a box with infinit length in crossProd(v1,v2) direction
    checkParamLength(card.params, std::vector<std::size_t>{9, 12}, mnemonic);

    std::vector<std::shared_ptr<Surface>> surfaces;
    Vec v0{card.params.at(0), card.params.at(1), card.params.at(2)};
    Vec a1{card.params.at(3), card.params.at(4), card.params.at(5)};
    Vec a2{card.params.at(6), card.params.at(7), card.params.at(8)};
    if(!math::isOrthogonal(a1, a2)) mWarning(it->pos()) << "In BOX card, first vector and second one is not orthogonal.";
    surfaces.emplace_back(std::make_shared<Plane>(card.name + ".1",  a1.normalized(),      math::dotProd(v0, a1.normalized()) + a1.abs()));
    surfaces.emplace_back(std::make_shared<Plane>(card.name + ".2", -a1.normalized(), -1.0*math::dotProd(v0, a1.normalized())));
    surfaces.emplace_back(std::make_shared<Plane>(card.name + ".3",  a2.normalized(),      math::dotProd(v0, a2.normalized()) + a2.abs()));
    surfaces.emplace_back(std::make_shared<Plane>(card.name + ".4", -a2.normalized(), -1.0*math::dotProd(v0, a2.normalized())));
    if(card.params.size() == 12) {
        // 12-arg-BOX
        Vec a3{card.params.at(9), card.params.at(10), card.params.at(11)};
        if(!math::isOrthogonal(a2, a3)) mWarning(it->pos()) << "In BOX card, second vector and third one is not orthogonal.";
        if(!math::isOrthogonal(a3, a1)) mWarning(it->pos()) << "In BOX card, third vector and first one is not orthogonal.";

        // Boxの場合6平面を生成する。
        // p2, p4, p6はマクロボディ定義(マクロボディ外側が＋)では法線の向きが逆になるのでdistanceにも-1を掛ける
        surfaces.emplace_back(std::make_shared<Plane>(card.name + ".5",  a3.normalized(),      math::dotProd(v0, a3.normalized()) + a3.abs()));
        surfaces.emplace_back(std::make_shared<Plane>(card.name + ".6", -a3.normalized(), -1.0*math::dotProd(v0, a3.normalized())));
    } else {
        // 9-arg-BOX plane5 and plane6 have infinity distances respectively.
        Vec a3 = math::crossProd(a1, a2);
        surfaces.emplace_back(std::make_shared<Plane>(card.name + ".5",  a3.normalized(), Surface::MAX_LENTGH));
        surfaces.emplace_back(std::make_shared<Plane>(card.name + ".6", -a3.normalized(), Surface::MAX_LENTGH));
    }
	macro::replaceSurfaceInput(surfaces, matrix, it, surfInputList);
	return std::make_pair(card.name, card.symbol);
}

void geom::macro::Box::replace(const std::string macroBodyName,
                               const std::list<inp::DataLine>::iterator &it)
{
    it->data = macro::replacCelInput(macroBodyName, numSurfaces, it->data);
}




