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
#include "baseunitelement.hpp"

#include <sstream>

#include "core/geometry/cell/boundingbox.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surface/surfacemap.hpp"


namespace {
auto add2Points = [](size_t kstart, const std::vector<std::pair<geom::Plane, geom::Plane>> &pairs,
					  const geom::Plane *pl1, const geom::Plane *pl2, std::vector<math::Point> *pts)
{
	for(size_t k = kstart; k < pairs.size(); ++k) {
		// 対面は平行なので交線が求まらないから無駄なので避けるが、六角格子なら対面以外でも交点なしが避けられないのでチェックする。
		auto pt = geom::Plane::intersection(*pl1, *pl2, pairs.at(k).first);
		if(pt.isValid()) pts->emplace_back(std::move(pt));
		pt = geom::Plane::intersection(*pl1, *pl2, pairs.at(k).second);
		if(pt.isValid()) pts->emplace_back(std::move(pt));
	}
};
}



std::unique_ptr<BaseUnitElement> BaseUnitElement::createBaseUnitElement(int latvalue,
																		const std::vector<std::string> &surfaceNames,
																		const geom::SurfaceMap &smap)
{
	// 矩形格子なら入力ペア数＝次元数、六角形格子なら次元数＝入力ペア数-1
	assert(surfaceNames.size()%2 == 0);
	int numIndex = static_cast<int>(surfaceNames.size()/2);
	int dimension = (latvalue == 1) ? numIndex : numIndex -1;

	std::vector<math::Vector<3>> indexVectors;
	std::vector<std::pair<geom::Plane, geom::Plane>> planePairs; // 基準単位要素[0 0 0]を構成する正側と負側の面
	math::Point elemCenter;  // centerの算出にも要素のコーナーを使うのでここで定義する必要がある。

	// LAT=2なら六角格子なので最低6面必要。LAT=1の場合は1次元格子というものもあるので2面以上
	if(latvalue == 1) {
		if(numIndex < 1) throw std::runtime_error(std::string("Number of surfaces is invalid for lattice definition. required >= 2, actual =")
												  + std::to_string(surfaceNames.size()));
	} else if(latvalue == 2) {
		if(numIndex < 3) throw std::runtime_error(std::string("Number of surfaces is invalid for lattice definition. required >= 6, actual =")
												  + std::to_string(surfaceNames.size()));
	}

	for(int ijk = 0; ijk < numIndex; ++ijk) {

		// pPlaneがインデックス正の側mPlaneが負の側。
		// surfCreatorはptr<Surf>なのでnormal()などplane固有のメソッドは呼べなく不便なのでここで変換
		auto pPlane = geom::Plane::fromString(smap.at(surfaceNames.at(2*ijk))->toInputString(), true);
		auto mPlane = geom::Plane::fromString(smap.at(surfaceNames.at(2*ijk+1))->toInputString(), true);
		// 面の法線をindexの増加する方向に揃えるためのベクトル
		math::Vector<3> direction = (pPlane->normal()*pPlane->distance() - mPlane->normal()*mPlane->distance()).normalized();
		pPlane->alignNormal(direction);
		mPlane->alignNormal(direction);

		planePairs.emplace_back(std::make_pair(*pPlane.get(), *mPlane.get()));

//		mDebug() << "生成されたプラス面===" << pPlane->toInputString();
//		mDebug() << "生成されたマイナス面===" << mPlane->toInputString();

		// 平行チェック
		if(!math::isDependent(pPlane->normal(), mPlane->normal())) {
			auto n1 = pPlane->normal(), n2 = mPlane->normal();
			std::stringstream ss;
			ss << "Lattice planes are not parallel, ";
			ss << "Plane " << pPlane->name();
			ss << " normal = (" << n1.x() << ", " << n1.y() << ", " << n1.z() << ") and ";
			ss << "Plane " << mPlane->name();
			ss << " normal = (" << n2.x() << ", " << n2.y() << ", " << n2.z() << ")";
			throw std::invalid_argument(ss.str());
		}

//		// 要素の中心を算出。3方向の+-要素面の原点からの最近接位置の平均をとる。
//		mDebug() << "プラス最近接点===" << pPlane->normal()*pPlane->distance();
//		mDebug() << "マイナス最近接点===" << mPlane->normal()*mPlane->distance();
		elemCenter += 0.5*(pPlane->normal()*pPlane->distance() + mPlane->normal()*mPlane->distance());
	}


	// ############### ここからは indexVectorの作成

	// 六角形の場合(numIndex-dimension > 0)のindexVector.
	bool isHexagon = numIndex - dimension > 0 && numIndex >= 3;

	// 基本要素[000]のコーナーの点からインデックスベクトルを作成する。
	// 2面の交差部分は点にならず線になるので
	// ”適当な”面でカットする。
	std::unique_ptr<geom::Plane> cuttingPlane;
	if(dimension == 3) {
		/*
		 * 3次元ならplus/minusPlanesの最後の面はそれ以外の面と別の次元なので
		 * plus/minusPlaneの最後の要素をを足して2で割ったような面でカットする。
		 */
		const geom::Plane &pp = planePairs.back().first, &mp = planePairs.back().second;
		cuttingPlane = std::make_unique<geom::Plane>("", pp.normal(), 0.5*(pp.normal()*pp.distance() - mp.normal()*pp.distance()));

	} else if(dimension == 2) {
			/*
			 * 2次元なら無限遠まで続いているのでカットする面は
			 * 既存の2面と直交し、原点を通る面を使う。
			 *
			 * 問題は基本要素定義面の法線が同一平面に乗っていない場合。
			 * 基本要素定義面の法線が同一平面に無い場合、mcnp plotterはバグるので
			 * 割と適当でも良いのかもしれない。
			 *
			 * その場合、原点から離れた点での断面が基本要素中心の形状と異なってくる。
			 * MCNPの場合、そのようなケースでもparticleロスは出ないのでセル定義は出来ている様子(計算が正しいかは不明)
			 * PHITSの場合、undefined領域として認識され、正しく作図される。
			 *
			 * 結局のところ同一平面に乗らないケースはあまり想定しなくても良さそう。
			 * PHITSの解釈のほうが一貫性はある。
			 * 対応するとしてもせいぜい警告を出すくらいで良い。
			 */
		const geom::Plane &p1 = planePairs.at(0).first, &p2 = planePairs.at(1).first;
		cuttingPlane = std::make_unique<geom::Plane>("", math::crossProd(p1.normal(), p2.normal()), math::Point{0, 0, 0});
	} else if(dimension == 1) {
		// 六角形latticeで1次元はありえないのでエラー
        if(isHexagon) throw std::invalid_argument("Dimension of lattice should be 2 or 3. for lat=2");

		// 1次元格子は未対応として暫定終了
		assert(!"Sorry one-Dimensional lattice is not implemented yet.");
	}

	// ここまででv方向(stuv:六角格子、stv:四角格子)にカットする平面cuttingPlaneが確定した。
	// インデックスベクトルを求める時に面と面の交差線から点に変換するためにcuttingPlaneを使う。

	if(isHexagon) {
		// 六角形格子でのインデックスベクトル。
		const geom::Plane &pp0 = planePairs.at(0).first, &mp0 = planePairs.at(0).second;
		const geom::Plane &pp1 = planePairs.at(1).first, &mp1 = planePairs.at(1).second;
		const geom::Plane &pp2 = planePairs.at(2).first, &mp2 = planePairs.at(2).second;
		const math::Point pt0 = geom::Plane::intersection(pp1, pp2, *cuttingPlane.get());
		const math::Point pt1 = geom::Plane::intersection(pp0, pp1, *cuttingPlane.get());
		const math::Point pt2 = geom::Plane::intersection(pp0, mp2, *cuttingPlane.get());
		const math::Point pt3 = geom::Plane::intersection(mp2, mp1, *cuttingPlane.get());
		const math::Point pt4 = geom::Plane::intersection(mp0, mp1, *cuttingPlane.get());
		indexVectors.resize(3);
		indexVectors.at(0) = pt2 - pt4;
		indexVectors.at(1) = pt1 - pt3;
		indexVectors.at(2) = pt0 - pt2;
	} else {
		// 四角格子でのインデックスベクトル
		const geom::Plane &pp0 = planePairs.at(0).first, &mp0 = planePairs.at(0).second;
		const geom::Plane &pp1 = planePairs.at(1).first, &mp1 = planePairs.at(1).second;
		const math::Point pt1 = geom::Plane::intersection(pp0, pp1, *cuttingPlane.get());
		const math::Point pt2 = geom::Plane::intersection(pp0, mp1, *cuttingPlane.get());
		const math::Point pt3 = geom::Plane::intersection(mp0, mp1, *cuttingPlane.get());
		indexVectors.resize(2);
		indexVectors.at(0) =  pt2 - pt3;
		indexVectors.at(1) =  pt1 - pt2;
	}

	// v方向はs，t方向と直交しているはずなのでコレで良い。
	if(dimension == 3) {
		const geom::Plane &pPlane = planePairs.back().first, &mPlane = planePairs.back().second;
		indexVectors.emplace_back(pPlane.normal()*pPlane.distance() - mPlane.normal()*mPlane.distance());
	}

	return std::make_unique<BaseUnitElement>(dimension, elemCenter, indexVectors, planePairs);
}

BaseUnitElement::BaseUnitElement(int dimension, const math::Point &center,
								 const std::vector<math::Vector<3>> &indexVecs,
								 const std::vector<std::pair<geom::Plane, geom::Plane>> &planePairs)
	:dimension_(dimension), center_(center), indexVectors_(indexVecs), planePairs_(planePairs)
{;}

geom::BoundingBox BaseUnitElement::unitBB() const
{
	std::vector<math::Point> cornerPoints = this->cornerPoints();
	geom::BoundingBox unitBB;
	if(dimension() == 3) {
		// 3Dなら基本格子の角の点が求まるので点群からBBを構成する。
		//mDebug() << "points===" << cornerPoints;
		unitBB = geom::BoundingBox::fromPoints(cornerPoints);
	} else {
		// 格子点から求まらない場合は面基本面から構成する。(無限セルの場合があるので求まるとは限らない。)
		std::vector<geom::Plane> unitPlanes;
		for(const auto &planePair: planePairs_) {
			// MCNPのカード入力に合うようにunitMinusPlanesもunitPlusPlanesと同じ向きを向いているので
			// plus側の面はBB計算時には逆向きにする必要がある。
			unitPlanes.emplace_back(geom::Plane("", -planePair.first.normal(), -planePair.first.distance()));
			unitPlanes.emplace_back(planePair.second);
		}

//		mDebug() << "BB単位要素を構成する面";
//		for(auto pl: unitPlanes) {
//			mDebug() << pl.toInputString();
//		}

		// 二次元LatticeでもBB生成できるように平面を追加する。まあ結局軸平行じゃなければBB計算できないので気休めではある。
		assert(indexVectors_.size() >= 2);  // LAT=2ならdim=2でもdispvectorは3つあるが、最初の2個を使えば第三の軸は得られる。
		auto dir3 = math::crossProd(indexVectors_.at(0), indexVectors_.at(1)).normalized();
		unitPlanes.emplace_back("", dir3, -(this->center() + geom::BoundingBox::MAX_EXTENT*dir3));
		unitPlanes.emplace_back("", -dir3, (this->center() + geom::BoundingBox::MAX_EXTENT*dir3));
//		mDebug() << "lattice要素のbb計算用planes";
//		for(const auto&up: unitPlanes) mDebug() << up.toInputString();

		// 2次元latticeの場合4平面なのでgeom::BoundingBox::fromPlanesでBBが計算できない。
		// その場合には被充填セルのBBを使うしか無い。
		unitBB = geom::BoundingBox::fromPlanes(nullptr, std::vector<std::vector<geom::Plane>>{unitPlanes});
	}
	return unitBB;
}

std::vector<math::Point> BaseUnitElement::cornerPoints() const
{
	//	assert(plusPlanes_.size() == minusPlanes_.size());
	//	assert(static_cast<int>(plusPlanes_.size()) == numIndex_);
		/*
		 * 要素中心位置の算出はdim=3, lat=2の時はややこしい。なぜならijl方向は同一平面なので適当に足せば
		 * 中心が求まるが、k方向はそれらと直行しているため。
		 * z方向も同様に足していくとij(l)平面はz=0で定義しているため単純に足すと重み付けが不適切
		 *
		 * ・Dim=2なら同一平面上なので適当に足せばいい
		 * ・Dim=3, LAT=1なら全ての方向の重み付けは同じなので適当に足せばいい
		 * ・Dim=3, LAT=2のときが問題
		 */

//		for(size_t i = 0; i < planePairs_.size(); ++i) {
//			mDebug() << "+plane===" << planePairs_.at(i).first.toString();
//			mDebug() << "-plane===" << planePairs_.at(i).second.toString();
//		}

		if(dimension_ == 1) {
		// FIXME 1次元latticeというのもある。
			assert(!"ProgramError: Sorry, 1D lattice has not implemented yet.");
			throw std::invalid_argument("ProgramError: Sorry, 1D lattice has not implemented yet.");
		} else if(dimension_ == 2) {
			return std::vector<math::Point>();
		} else {
			// 3Dのケース
			std::vector<math::Point> points;
			for(size_t i = 0; i < planePairs_.size()-2; ++i) {
				// まずplanePairs.at(i).firstを選択
				for(size_t j = i+1; j < planePairs_.size()-1; ++j) {
					add2Points(j+1, planePairs_, &planePairs_.at(i).first, &planePairs_.at(j).first, &points);
					add2Points(j+1, planePairs_, &planePairs_.at(i).first, &planePairs_.at(j).second, &points);
					add2Points(j+1, planePairs_, &planePairs_.at(i).second, &planePairs_.at(j).first, &points);
					add2Points(j+1, planePairs_, &planePairs_.at(i).second, &planePairs_.at(j).second, &points);
				}
			}
			return points;
		}
}

// 要素の角の点を返す


