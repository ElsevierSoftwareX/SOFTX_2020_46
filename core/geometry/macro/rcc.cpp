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
#include "rcc.hpp"

#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"
#include "core/math/nvector.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/io/input/dataline.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surface/cylinder.hpp"



const char geom::macro::Rcc::mnemonic[]  = "rcc";

std::pair<std::string, std::string>
	geom::macro::Rcc::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
					  std::list<inp::DataLine>::iterator &it,
					  std::list<inp::DataLine> *surfInputList)
{
	using Vec = math::Vector<3>;
	auto card = inp::SurfaceCard::fromString(it->data);
//	Matrix matrix = (card.trNumber != inp::SurfaceCard::NO_TR) ? trMap.at(card.trNumber): Matrix();
	auto matrix = card.getMatrixPtr(trMap);

	// RCCの場合円筒1面、平面2面を生成する。
	checkParamLength(card.params, std::vector<std::size_t>{7}, mnemonic);
	math::Point base{card.params.at(0), card.params.at(1), card.params.at(2)};
	Vec refVec{card.params.at(3), card.params.at(4), card.params.at(5)};
	double radius = card.params.at(6);
	auto hvec = refVec.normalized();


	std::vector<std::shared_ptr<Surface>> surfaces{
		std::make_shared<Cylinder>(card.name + ".1", base, hvec, radius),
		std::make_shared<Plane>(card.name + ".2", hvec, math::dotProd(base, hvec) + refVec.abs()),
		std::make_shared<Plane>(card.name + ".3", -hvec, -math::dotProd(base, hvec))
	};
	macro::replaceSurfaceInput(surfaces, matrix, it, surfInputList);
	return std::make_pair(card.name, card.symbol);

//	auto filename = it->file;
//	auto line = it->line;
//	// eraseの戻り値は次の要素、
//	// insertの引数は挿入場所の次の要素で、返り値は挿入された要素。
//	// transformを掛けながらsurface入力へ追加していく
//	it = surfInputList->erase(it);

//	cylinder.transform(matrix);
//	it = surfInputList->insert(it, inp::DataLine(filename, line, cylinder.toInputString()));
//	++it;
//	top.transform(matrix);
//	it = surfInputList->insert(it, inp::DataLine(filename, line, top.toInputString()));
//	++it;
//	bottom.transform(matrix);
//	it = surfInputList->insert(it, inp::DataLine(filename, line, bottom.toInputString()));
//	return std::make_pair(card.name, card.symbol);
}

void geom::macro::Rcc::replace(const std::string &surfName,
						const std::list<inp::DataLine>::iterator &it)
{
	// 円筒は3平面置換
    it->data = macro::replacCelInput(surfName, numSurfaces, it->data);
}
