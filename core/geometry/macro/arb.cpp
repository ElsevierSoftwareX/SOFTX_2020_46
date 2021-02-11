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
#include "arb.hpp"


#include <algorithm>
#include <array>
#include <cmath>

#include "core/geometry/surface/plane.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/math/nvector.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"


const char geom::macro::Arb::mnemonic[]  = "arb";


#include <set>

std::pair<std::string, std::string>
	geom::macro::Arb::expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
							 std::list<inp::DataLine>::iterator &it,
							 std::list<inp::DataLine> *surfInputList)
{
	using Pt = math::Point;
	inp::SurfaceCard card = inp::SurfaceCard::fromString(it->data);
	auto matrix = card.getMatrixPtr(trMap);
	checkParamLength(card.params, std::vector<std::size_t>{30}, mnemonic);

	// 位相情報に出てくる点のみが使用されることに注意。
	// 最後の6個の入力(index=24-29)が位相情報で、4文字の文字列か0。
	// 0は不要なので除く。paramsは既にdouble化されているのでstringに戻す
	std::vector<std::array<int, 4>> phaseInfoSet;
	std::set<size_t> usedIndexSet;  // 位相情報で使われたindex番号のセット
	for(size_t i = 0; i < 6; ++i) {
		std::string str = std::to_string(std::lround(card.params.at(24+i)));
		if(str != "0") {
			if(str.size() != 4) {
				throw std::invalid_argument("Digit of phase info should be 4, actual = " + str);
			} else if(str.find_first_not_of("012345678") != std::string::npos) {
				throw std::invalid_argument("Phase info character should be 0-8, actual = " + str);
			}

			std::array<int, 4> phaseInfo {
				utils::stringTo<int>(std::string{str.at(0)}),
				utils::stringTo<int>(std::string{str.at(1)}),
				utils::stringTo<int>(std::string{str.at(2)}),
				utils::stringTo<int>(std::string{str.at(3)}),
			};

			phaseInfoSet.emplace_back(phaseInfo);
			for(size_t i = 0; i < 4; ++i) usedIndexSet.insert(utils::stringTo<size_t>(std::string{str.at(i)}));

		}
	}
	if(phaseInfoSet.size() <= 3) {
		throw std::invalid_argument("Number of phase inputs should be larger than 4. current="
									+ std::to_string(phaseInfoSet.size()));
	}


	// 最初の24個入力で8点を作成。
	std::array<math::Point, 8> points;
	for(size_t i = 0; i < points.size(); ++i) {
		points[i] = math::Point({card.params.at(3*i), card.params.at(3*i+1), card.params.at(3*i+2)});
	}

	math::Point center{0, 0, 0};
	size_t numUsedPoints = 0;
	// 使用されている点のみを平均化してARB中心を算出
	for(size_t i = 0; i < points.size(); ++i) {
		if(usedIndexSet.find(i+1) != usedIndexSet.end()) {
			++numUsedPoints;
			center += points.at(i);
		}
	}
	if(numUsedPoints < 4) {
		throw std::invalid_argument("Less than 4 point are used in the phase information. used = "
									+ std::to_string(numUsedPoints));
	}
	center = center*(1.0/static_cast<double>(numUsedPoints));




	/* Plane作成は
	 * 1.createPlane(名前、 パラメータ、変換行列、 type, phits互換性bool)
	 *		type: {P, PX, PY, PZ}
	 *		パラメータ：Pなら9パラメータ(3点) 但しPカードで作成された面の法線方向はMCNP独特のルールで決まる
	 * 2.コンストラクタ(名前、法線ベクトル、原点からの距離)で作成する
	 * 3.コンストラクタ(3点)
	 *
	 * ここでは3を使う
	 */

	//mDebug() << "phaseInfoSEt=" << phaseInfoSet;

	std::vector<std::shared_ptr<Surface>> surfaces;

	std::vector<Pt*> pts;  // 面を生成するための点
    for(size_t i = 0; i < phaseInfoSet.size(); ++i) {
        pts.resize(4);
		try {
			// phase番号は1から始めるから1を引く 頂点番号0は-1になるがそこは不使用としてnullptrを入れる
			pts[0] = (phaseInfoSet.at(i).at(0)-1 < 0) ? nullptr : &points.at(phaseInfoSet.at(i).at(0)-1);
			pts[1] = (phaseInfoSet.at(i).at(1)-1 < 0) ? nullptr : &points.at(phaseInfoSet.at(i).at(1)-1);
			pts[2] = (phaseInfoSet.at(i).at(2)-1 < 0) ? nullptr : &points.at(phaseInfoSet.at(i).at(2)-1);
			pts[3] = (phaseInfoSet.at(i).at(3)-1 < 0) ? nullptr : &points.at(phaseInfoSet.at(i).at(3)-1);

			// nullptrが2個以上あったら面を定義できないのでエラー
			if(std::count(pts.cbegin(), pts.cend(), nullptr) >= 2) {
                std::stringstream ess;
                ess << "More than tow non-zero vertex number should be required. Actual={";
                for(size_t j = 0; i < phaseInfoSet.at(i).size(); ++j) {
                    ess << phaseInfoSet.at(i).at(j);
                    if (j != phaseInfoSet.at(i).size()-1) ess << ", ";
                }
                ess << "}";
                throw std::invalid_argument(ess.str());
			}
			// nullptrは削除
			auto itr = std::remove_if(pts.begin(), pts.end(), [](Pt* p){return p == nullptr;});
			pts.erase(itr, pts.end());

			auto planeName = card.name + "." + std::to_string(i+1);
			/*
             * ARBの各面の向きは全て外を向くように設定される。単なるPカードの挙動とはことなるので特別の取扱が必要。
			 * Pカードでは原点をnegative側に含むようにする、とあるがARBの場合それとは違った挙動になっていることに注意する必要がある。
			 * 多分原点ではなく、ARBの中心(頂点の平均)を含むようにしているのではないかと推察される。
			 */
			Plane tmpPlane("", *pts.at(0), *pts.at(1), *pts.at(2), Plane::NormalType::MCNP);
			// ARB中心がtmpPlaneの内側に入っていなければnormalを逆転させる。
			// ARBは凸多面体なのでARB中心が多面体の外側に来ることはない。
			math::Vector<3> newNormal = tmpPlane.normal();
			double newDistance = tmpPlane.distance();
			if(tmpPlane.isForward(center)) {
				newNormal *= -1;
				newDistance *= -1;
			}
			auto newPlane = std::make_shared<Plane>(planeName, newNormal, newDistance);

//			auto newPlane = std::make_shared<Plane>(planeName, *pts.at(0), *pts.at(1), *pts.at(2), Plane::NormalType::MCNP);


			surfaces.emplace_back(newPlane);
			if(pts.size() == 4) {
				// 4点目の同一平面性チェック
				const Pt &pt = *pts.at(3);
				double distance = newPlane->distanceToPoint(pt); // math::dotProd(pt, newPlane->normal()) - newPlane->distance();
				if(std::abs(distance) > 1e-6) {  // 1e-6はmcnpの基準
					mWarning() << "Point" << pt << "is not on the plane defined by points. distance=" << distance
							   << "points=" << *pts.at(0) << *pts.at(1) << *pts.at(2);
				}
			}
		} catch (std::out_of_range &oor) {
            (void)oor;
			std::stringstream ss;
			ss << "phase set=";
			for(auto info: phaseInfoSet) {
				// operator<<(os, array<int,4>)は名前空間std::内で定義されているoperator<<とぶつかるので適当に変換する。
				ss << PRINTABLE<std::array<int, 4>>(info);
			}
            throw std::invalid_argument("Point index is invalid. index set="+ ss.str());
		}
	}
	// ARBで使われる平面数は可変で、"最大"6平面、最小4平面が生成される。今のマクロボディ実装は可変を想定していないので
	// 最大数が使われるようにし、6平面に満たない分については最後に定義された面を重複して使用する。
	// numSurfaceを書き換えるよりも確実。


	//	// numSurfacesを書き換えた場合マルチスレッド環境などでの動作が怪しくなるので避ける。
	//	numSurfaces = surfaces.size();

	size_t init_size = surfaces.size();
	for(size_t i = init_size; i < 6; i++) {
		auto numStr = std::string(".") + std::to_string(i+1);
        surfaces.emplace_back(std::make_shared<Plane>(card.name + numStr , *pts.at(0), *pts.at(1), *pts.at(2), Plane::NormalType::MCNP));
	}


	macro::replaceSurfaceInput(surfaces, matrix, it, surfInputList);
	return std::make_pair(card.name, card.symbol);
}

void geom::macro::Arb::replace(const std::string &surfName,
							   const std::list<inp::DataLine>::iterator &it)
{
    it->data = macro::replacCelInput(surfName, numSurfaces, it->data);
}
