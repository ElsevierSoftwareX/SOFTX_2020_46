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
#include "sph.hpp"


#include "core/io/input/dataline.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/math/nvector.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "core/geometry/surface/sphere.hpp"


const char geom::macro::Sph::mnemonic[] = "sph";

std::pair<std::string, std::string>
	geom::macro::Sph::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
					  std::list<inp::DataLine>::iterator &it,
					  std::list<inp::DataLine> *surfInputList)
{
	(void)surfInputList;
	using Matrix = math::Matrix<4>;
	auto card = inp::SurfaceCard::fromString(it->data);
	Matrix matrix = (card.trNumber != inp::SurfaceCard::NO_TR) ? trMap.at(card.trNumber): Matrix();

	// SPHの引数の数は4
	checkParamLength(card.params, std::vector<std::size_t>{1, 4}, mnemonic);


	math::Point center;
	if(card.params.size() == 4) {
		center = math::Point{card.params.at(0), card.params.at(1), card.params.at(2)};
	} else {
		mWarning(it->pos()) << "Only one parameter for SPH is not MCNP-compatible.";
		center = math::Point{0,0,0};
	}
	Sphere sp(card.name, center, card.params.at(3));
	// 1マクロボディ1面ならeraseせずに上書きすれば良い
	sp.transform(matrix);
	it->data = sp.toInputString();
	return std::make_pair(card.name, card.symbol);
}

void geom::macro::Sph::replace(const std::string &surfName,
						const std::list<inp::DataLine>::iterator &it)
{
	(void) surfName;
	(void) it;
	// 1マクロボディ1面でかつ法線方向も変わらないので置き換える必要なし。
}
