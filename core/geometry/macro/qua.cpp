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
#include "qua.hpp"

#include <sstream>
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/geometry/surface/quadric.hpp"
#include "core/geometry/surface/plane.hpp"

const char geom::macro::Qua::mnemonic[] = "qua";


std::pair<std::string, std::string>
geom::macro::Qua::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
						 std::list<inp::DataLine>::iterator &it,
						 std::list<inp::DataLine> *surfInputList)
{
	using Vec = math::Vector<3>;
	auto macroBodyCard = inp::SurfaceCard::fromString(it->data); // マクロボディのカード
	checkParamLength(macroBodyCard.params, std::vector<std::size_t>{10}, mnemonic);

	// QUAでは二次曲面1枚，平面２枚を生成する。
	const std::vector<double> &params = macroBodyCard.params;
	double a = params.at(0), b = params.at(1), c = params.at(2), d = params.at(3),
			e = params.at(4), f = params.at(5), g = params.at(6), h = params.at(7);
	double z1 = params.at(8), z2 = params.at(9);
	std::string quadName = macroBodyCard.name + ".1", zplaneName1 = macroBodyCard.name + ".2", zplaneName2 = macroBodyCard.name + ".3";
    std::vector<double> qpara{a*a, d*d, b*b + e*e - g*g, 0, 2*d*e, 2*a*b, 2*a*c, 2*d*f, 2*(b*c + e*f - g*h), c*c + f*f - h*h};
	auto quadSurf = std::make_shared<Quadric>(quadName, qpara);
	auto zplane1 = std::make_shared<Plane>(zplaneName1, Vec{0, 0, -1}, math::Point{0, 0, z1});  // MBの法線は内向き
	auto zplane2 = std::make_shared<Plane>(zplaneName2, Vec{0, 0, 1}, math::Point{0, 0, z2});

	std::vector<std::shared_ptr<Surface>> surfaces{quadSurf, zplane1, zplane2};
	macro::replaceSurfaceInput(surfaces, macroBodyCard.getMatrixPtr(trMap), it, surfInputList);
	return std::make_pair(macroBodyCard.name, macroBodyCard.symbol);
}

void geom::macro::Qua::replace(const std::string &macroBodyName,
							   const std::list<inp::DataLine>::iterator &it)
{
	it->data = macro::replacCelInput(macroBodyName, numSurfaces, it->data);
}
