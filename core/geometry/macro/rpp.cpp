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
#include "rpp.hpp"

#include "core/utils/message.hpp"
#include "core/math/nvector.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/io/input/dataline.hpp"
#include "core/utils/string_utils.hpp"
#include "core/geometry/surface/plane.hpp"



const char geom::macro::Rpp::mnemonic[] = "rpp";

std::pair<std::string, std::string>
	geom::macro::Rpp::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
					  std::list<inp::DataLine>::iterator &it,
					  std::list<inp::DataLine> *surfInputList)
{
	using Vec = math::Vector<3>;
	//using Matrix = math::Matrix<4>;
	auto card = inp::SurfaceCard::fromString(it->data);
	//Matrix matrix = (card.trNumber != inp::SurfaceCard::NO_TR) ? trMap.at(card.trNumber): Matrix();
	auto matrix = card.getMatrixPtr(trMap);

	// RPPの場合6平面を生成する。
	//assert(card.params.size() == 6);
	checkParamLength(card.params, std::vector<std::size_t>{6}, mnemonic);
	auto xmin = card.params.at(0), xmax = card.params.at(1);
	auto ymin = card.params.at(2), ymax = card.params.at(3);
	auto zmin = card.params.at(4), zmax = card.params.at(5);

	if(xmax < xmin) mWarning(it->pos()) << "In RPP card, xmin is larger than xmax";
	if(ymax < ymin) mWarning(it->pos()) << "In RPP card, ymin is larger than ymax";
	if(zmax < zmin) mWarning(it->pos()) << "In RPP card, zmin is larger than zmax";

	std::vector<std::shared_ptr<Surface>> surfaces{
		// 法線ベクトルが負の場合、distanceも-1が掛かったものが実質の距離になる
		std::make_shared<Plane>(card.name + ".1", Vec{ 1,  0,  0}, xmax),
		std::make_shared<Plane>(card.name + ".2", Vec{-1,  0,  0}, -xmin),
		std::make_shared<Plane>(card.name + ".3", Vec{ 0,  1,  0}, ymax),
		std::make_shared<Plane>(card.name + ".4", Vec{ 0, -1,  0}, -ymin),
		std::make_shared<Plane>(card.name + ".5", Vec{ 0,  0,  1}, zmax),
		std::make_shared<Plane>(card.name + ".6", Vec{ 0,  0, -1}, -zmin)
	};

	macro::replaceSurfaceInput(surfaces, matrix, it, surfInputList);
	return std::make_pair(card.name, card.symbol);

//	auto filename = it->file;
//	auto line = it->line;
//	// eraseの戻り値は次の要素、
//	// insertの引数は挿入場所の次の要素で、返り値は挿入された要素。
//	// transformを掛けながらsurface入力へ追加していく
//	it = surfInputList->erase(it);
//	for(auto &plane: planes) {
//		plane.transform(matrix);
//		it = surfInputList->insert(it, inp::DataLine(filename, line, plane.toInputString()));
//		++it;
//	}
//	--it;
//	return std::make_pair(card.name, card.symbol);
}

void geom::macro::Rpp::replace(const std::string &surfName,
						const std::list<inp::DataLine>::iterator &it)
{
    it->data = macro::replacCelInput(surfName, numSurfaces, it->data);
}
