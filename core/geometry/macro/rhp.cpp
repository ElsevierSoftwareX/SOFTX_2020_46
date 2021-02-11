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
#include "rhp.hpp"


#include "core/utils/message.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/io/input/dataline.hpp"
#include "core/utils/string_utils.hpp"
#include "core/geometry/surface/plane.hpp"

const char geom::macro::Rhp::mnemonic[]  = "rhp";

std::pair<std::string, std::string>
	geom::macro::Rhp::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
					  std::list<inp::DataLine>::iterator &it,
					  std::list<inp::DataLine> *surfInputList)
{
	using Vec = math::Vector<3>;
	//using Matrix = math::Matrix<4>;
	auto card = inp::SurfaceCard::fromString(it->data);
	//Matrix matrix = (card.trNumber != inp::SurfaceCard::NO_TR) ? trMap.at(card.trNumber): Matrix();
	auto matrix = card.getMatrixPtr(trMap);

	// HPの場合平面8面(上底、下底、側面×6)を生成する。
//	assert(card.params.size() == 9 || card.params.size() == 15);
	checkParamLength(card.params, std::vector<std::size_t>{9, 15}, mnemonic);

	math::Point basePos{card.params.at(0), card.params.at(1), card.params.at(2)};
	Vec topVec{card.params.at(3), card.params.at(4), card.params.at(5)};
	Vec rVec{card.params.at(6), card.params.at(7), card.params.at(8)};
	auto hUnit = topVec.normalized();
	auto rUnit = rVec.normalized();

	Vec sVec, tVec;
	if(card.params.size() == 9) {
		// 1/2=cos(60deg), sqrt3/2=sin(60deg), -1/2=cos(120), sqrt3/2=sin(120deg)
		sVec = rVec.abs()*0.5*(rUnit + std::sqrt(3)*(math::crossProd(hUnit, rUnit)));
		tVec = rVec.abs()*0.5*(-rUnit + std::sqrt(3)*(math::crossProd(hUnit, rUnit)));
	} else {
		sVec = Vec{card.params.at(9), card.params.at(10), card.params.at(11)};
		tVec = Vec{card.params.at(12), card.params.at(13), card.params.at(14)};
	}

	auto sUnit = sVec.normalized();
	auto tUnit = tVec.normalized();

	std::vector<std::shared_ptr<Plane>> planes {
		std::make_shared<Plane>(card.name + ".1",  rUnit, math::dotProd(basePos + rVec,  rUnit)),
		std::make_shared<Plane>(card.name + ".2", -rUnit, math::dotProd(basePos - rVec, -rUnit)),
		std::make_shared<Plane>(card.name + ".3",  sUnit, math::dotProd(basePos + sVec,  sUnit)),
		std::make_shared<Plane>(card.name + ".4", -sUnit, math::dotProd(basePos - sVec, -sUnit)),
		std::make_shared<Plane>(card.name + ".5",  tUnit, math::dotProd(basePos + tVec,  tUnit)),
		std::make_shared<Plane>(card.name + ".6", -tUnit, math::dotProd(basePos - tVec, -tUnit)),
		std::make_shared<Plane>(card.name  + ".7",  hUnit, math::dotProd(basePos + topVec, hUnit)),
		std::make_shared<Plane>(card.name  + ".8", -hUnit, math::dotProd(basePos, -hUnit))
	};
	// surfaces.at(6)はtop面
//	if(warnPhitsCompat) {
		for(size_t i = 0; i < 6; ++i) {
			// Surface::normal()を追加したい
			if(!math::isOrthogonal(planes.at(i)->normal(), topVec)) {
				mWarning(it->pos()) << "RHP/HEX's side surface should be orthogonal to top/bottom in phits";
				break;
			}
		}
//	}
	std::vector<std::shared_ptr<Surface>> surfaces;
	for(auto &surf:planes) surfaces.emplace_back(surf);
	macro::replaceSurfaceInput(surfaces, matrix, it, surfInputList);
	return std::make_pair(card.name, card.symbol);



}

void geom::macro::Rhp::replace(const std::string &surfName,
						const std::list<inp::DataLine>::iterator &it)
{
    it->data = macro::replacCelInput(surfName, numSurfaces, it->data);
}
