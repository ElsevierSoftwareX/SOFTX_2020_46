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
#include "surfacecardparser.hpp"

#include <cassert>
#include <cstdlib>
#include "../../src/utils/string.hpp"
#include "../../src/utils/message.hpp"
#include "surface/sphere.hpp"

// 入力は "10 S 0 0 5 5"のような感じ
std::shared_ptr<geom::Surface> geom::SurfaceCardParser::createSurface(const std::string &str)
{
	std::vector<std::string>  elements = utils::split(' ', str, true);
	//mDebug() << "elements =" << elements;

	if(elements.size() <= 2) {
		std::cerr << "Error! Too short card =" << str << std::endl;
		std::exit(EXIT_FAILURE);
	}

	math::Matrix<4> trMatrix = math::Matrix<4>::IDENTITY();
	// 2個めの要素が数字ならTRカードあり。
	if(utils::isNumber(elements.at(1))) {
		size_t trNumber = utils::stringTo<size_t>(elements.at(1));
		matrix_map_type::iterator itr = trMap_.find(trNumber);
		if(itr != trMap_.end()) {
			trMatrix = itr->second;
		} else {
			throw std::invalid_argument(std::string("TR") + elements.at(1) + "is used but not defined.");
		}
		// TR指定している部分は削除する。
		elements.erase(elements.begin()+1);
	}

	std::string name = elements.at(0);
	std::string symbol = elements.at(1);

	// Surfaceカードの引数は数値のみなので種類を問わず共通処理として引数は全てdouble化しておく
	std::vector<double> parameters;
	try {
		parameters = utils::stringVectorTo<double>(elements, 2, elements.size() - 2);
	} catch (std::invalid_argument& e) {
		throw std::invalid_argument(std::string(e.what()) + ", invalid character exists");
	} catch (std::out_of_range & e) {
		throw std::invalid_argument(std::string(e.what()) + ", too short card");
	}

	// オブジェクト生成
	if(symbol == "S") {
		// PEND 必ずtransform行列かけているのは速度的に良くないかもしれない。
		geom::math::Vector4 paramVec4{parameters.at(0), parameters.at(1), parameters.at(2), 1};
//		mDebug() << "trMatrix=" << trMatrix;
//		mDebug() << "before vec4=" << paramVec4;
		paramVec4 = paramVec4*trMatrix;
		//mDebug() << "after vec4=" << paramVec4;
		return std::make_shared<Sphere>(name, paramVec4.toVector3(), parameters.at(3));
	} else {
		throw std::invalid_argument(std::string("invalid symbol=") + symbol);
	}
}


// keyになる文字列もペアにしないとunorder_mapに登録できない
// TRを適用するとなるとTrカードも必要
std::pair<std::string, std::shared_ptr<geom::Surface> > geom::SurfaceCardParser::createSurfacePair(const std::string &str)
{
	std::shared_ptr<geom::Surface> surf = createSurface(str);
	return std::pair<std::string, std::shared_ptr<geom::Surface> >(surf->name(), surf);
}


