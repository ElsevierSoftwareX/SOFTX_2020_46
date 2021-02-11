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
#include "trc.hpp"

#include "core/geometry/macro/rcc.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surface/cone.hpp"
#include "core/geometry/surface/cylinder.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/io/input/dataline.hpp"
#include "core/math/nvector.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/numeric_utils.hpp"

const char geom::macro::Trc::mnemonic[] = "trc";



std::pair<std::string, std::string>
geom::macro::Trc::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
						 std::list<inp::DataLine>::iterator &it, std::list<inp::DataLine> *surfInputList)
{
	using Vec = math::Vector<3>;
	auto card = inp::SurfaceCard::fromString(it->data);
	auto matrix = card.getMatrixPtr(trMap);

	// TRCの場合円錐1面、上底下底平面2面を生成する。 TRCは8入力。
	checkParamLength(card.params, std::vector<std::size_t>{8}, mnemonic);
	math::Point base{card.params.at(0), card.params.at(1), card.params.at(2)};
	Vec refVec{card.params.at(3), card.params.at(4), card.params.at(5)};
	auto hvec = refVec.normalized();
	double lowerRadius = card.params.at(6);
	double upperRadius = card.params.at(7);

	std::vector<std::shared_ptr<Surface>> surfaces;
	if(utils::isSameDouble(lowerRadius, upperRadius)) {
		mWarning(it->pos()) << "Lower and upper radii are the same in TRC. Replaced by RCC. (but this is fatal in MCNP)";
		surfaces = std::vector<std::shared_ptr<Surface>>{
			std::make_shared<Cylinder>(card.name + ".1", base, hvec, lowerRadius),
			std::make_shared<Plane>(card.name + ".2", hvec, math::dotProd(base, hvec) + refVec.abs()),
			std::make_shared<Plane>(card.name + ".3", -hvec, -math::dotProd(base, hvec))
		};
	} else {
		auto origin = base + lowerRadius/(lowerRadius - upperRadius)*refVec;  // 円錐頂点
		// TRC は.1が円錐側面、.2が上底、.3が下底
		auto coneRefVec = -hvec;
		if(lowerRadius < upperRadius) coneRefVec = -1*coneRefVec;
		surfaces = std::vector<std::shared_ptr<Surface>>{
			std::make_shared<Cone>(card.name + ".1", origin, coneRefVec, (lowerRadius - upperRadius)/refVec.abs(), 1),
			std::make_shared<Plane>(card.name + ".2", hvec, math::dotProd(base, hvec) + refVec.abs()),
			std::make_shared<Plane>(card.name + ".3", -hvec, -math::dotProd(base, hvec))
		};
	}
	macro::replaceSurfaceInput(surfaces, matrix, it, surfInputList);
	return std::make_pair(card.name, card.symbol);

//	Cone cone(card.name + ".1", origin, coneRefVec, (lowerRadius - upperRadius)/refVec.abs(), 1);
//	Plane top(card.name + ".2", hvec, math::dotProd(base, hvec) + refVec.abs());
//	Plane bottom(card.name + ".3", -hvec, -math::dotProd(base, hvec));

//	mDebug() << "origin=" << origin << "hvec=" << hvec;
//	mDebug() << "cone=" << cone.toInputString();
//	mDebug() << "top Plane=" << top.toInputString();
//	mDebug() << "bottom plane=" << bottom.toInputString();

//	auto filename = it->file;
//	auto line = it->line;
//	// eraseの戻り値は次の要素、
//	// insertの引数は挿入場所の次の要素で、返り値は挿入された要素。
//	// transformを掛けながらsurface入力へ追加していく
//	it = surfInputList->erase(it);
//	cone.transform(matrix);
//	it = surfInputList->insert(it, inp::DataLine(filename, line, cone.toInputString()));
//	++it;
//	top.transform(matrix);
//	it = surfInputList->insert(it, inp::DataLine(filename, line, top.toInputString()));
//	++it;
//	bottom.transform(matrix);
//	it = surfInputList->insert(it, inp::DataLine(filename, line, bottom.toInputString()));
//	return std::make_pair(card.name, card.symbol);
}

void geom::macro::Trc::replace(const std::string &surfName, const std::list<inp::DataLine>::iterator &it)
{
	// 上底半径と下底半径が等しい場合はRCCに置き換えるが、RCCとTRCのreplaceは実質同じなので特に考慮しなくても良い。
	// 円錐は3平面置換
    it->data = macro::replacCelInput(surfName, numSurfaces, it->data);
}
