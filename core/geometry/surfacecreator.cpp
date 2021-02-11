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
#include "surfacecreator.hpp"

#include <algorithm>
#include <functional>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <vector>

#include "surf_utils.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/math/nmatrix.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"
#include "core/utils/matrix_utils.hpp"
#include "core/geometry/surface/polyhedron.hpp"
#include "core/geometry/surface/surface_utils.hpp"







geom::SurfaceCreator::SurfaceCreator(const std::list<inp::DataLine> &surfInputList,
									 const std::unordered_map<size_t, math::Matrix<4>> &trMatrixes,
									 bool warnPhitsCompat)
	:SurfaceCreator()
{
	trMap_ = trMatrixes;
	for(auto &element: surfInputList) {
		try{
			//mDebug() << "Construct SurfaceObjects data=" << element;
			//surfaceMap_に重複をチェックしながらinsert
			inp::SurfaceCard scard = inp::SurfaceCard::fromString(element.data);

			if(scard.name.empty()) {
				throw std::invalid_argument("Empty surface name is not acceptable");
			} else if (scard.name.front() == '-') {
				throw std::invalid_argument("Surface name starts with \"-\" is forbidden"); // -で始まるsurface名は禁止
			} else if (scard.name.front() == '*') {
				mWarning(element.pos()) << "Surface name starting with * (reflecting boundary) is not collectly suported.";
				scard.name = scard.name.substr(1);
				//throw std::invalid_argument("Surface name starting with * is forbidden (reflecting surface is not supported).");
			} else if (scard.name.front() == '+') {
				mWarning(element.pos()) << "Surface name starting with + (white boundary) is not collectly suported.";
				scard.name = scard.name.substr(1);
				//throw std::invalid_argument("Surface name starting with + is forbidden (white surface is not supported).");
			} else if(surfaceMap_.hasSurfaceName(scard.name)) {
				throw std::invalid_argument(std::string("Multiple surface definition found. name =") + scard.name);
			} else if (scard.trNumber < 0) {
				throw std::invalid_argument("Negative TR number(periodic boundary) is not supported.");
			} else if(warnPhitsCompat && !utils::isArithmetic(scard.name)) {
				mWarning(element.pos()) << "Non-integer surface name \"" << scard.name << "\" is not phits-compatible.";
			}
			// サーフェイス名が実装されているかはcreateSurfaceでチェックして例外を投げるからここでは不要。

			math::Matrix<4> matrix = math::Matrix<4>::IDENTITY();
			// surfaceはTRSFかTRnでTransform指定される。TRSFを先に処理する。
			if(!scard.trStr.empty()) {
				matrix = matrix * utils::generateSingleTransformMatrix(scard.trStr, warnPhitsCompat);
			}
			if(scard.trNumber != inp::SurfaceCard::NO_TR) {
				if(trMap_.find(static_cast<size_t>(scard.trNumber)) == trMap_.end()) {
					throw std::invalid_argument(std::string("TR") + std::to_string(scard.trNumber)
												+ " is used but not defined.");
				} else {
					matrix = matrix*trMap_.at(static_cast<size_t>(scard.trNumber));
				}
			}
			/*
			 * MCNP/PHITSとの互換性を向上させるために
			 * integerと解釈できる面名はint化してから文字列化する。
			 * 問題はマクロボディの場合。浮動小数点数に見える場合がある。
			 * 0500.1みたいに整数化すべきマクロボディもある。
			 * sphere.1みたいに整数化すべきでないマクロボディもある。
			 */
            std::shared_ptr<Surface> surface
                    = geom::createSurface(scard.name, scard.symbol, scard.params,
                                          scard.paramMap, matrix, warnPhitsCompat);
			//mDebug() << "createdSurface=" << surface->toString();
			surfaceMap_.registerSurface(surface->getID(), surface);

		} catch (std::invalid_argument &e) {
			// カードのパラメータがおかしいとinvalid_argument
            throw std::invalid_argument(element.pos() + " Invalid input data. " + e.what());
		} catch (std::out_of_range &e) {
			// 面シンボルが見つからない場合out_of_range
            throw std::invalid_argument(element.pos() + " Invalid surface symbol. " + e.what());
		} catch (std::exception& e) {
            throw std::invalid_argument(element.pos() + e.what());
		}
	}

	// ここで裏面追加
	utils::addReverseSurfaces(&surfaceMap_);
}

geom::SurfaceCreator::SurfaceCreator(const Surface::map_type &surfMap)
	:surfaceMap_(surfMap)
{
	utils::addReverseSurfaces(&surfaceMap_);
}


void geom::SurfaceCreator::removeUnusedSurfaces(bool warn)
{
	utils::removeUnusedSurfaces(&surfaceMap_, warn);

}


std::ostream &geom::operator <<(std::ostream &os, const geom::SurfaceCreator &creator)
{
	// elemはpair<string, shared_ptr<Surface>>
	os << "SurfaceMap:";
	for(auto& surfPair: creator.map().frontSurfaces()) {
		if(surfPair.second) {
			os << std::endl;
			os << "key = " << surfPair.second->getID() << ", surface = " << surfPair.second->toString();
		}
	}
	for(auto& surfPair: creator.map().backSurfaces()) {
		if(surfPair.second) {
			os << std::endl;
			os << "key = " << surfPair.second->getID() << ", surface = " << surfPair.second->toString();
		}
	}
	return os;
}

