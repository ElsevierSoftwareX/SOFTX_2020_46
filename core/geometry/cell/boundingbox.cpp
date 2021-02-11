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
#include "boundingbox.hpp"

#include <algorithm>
#include <chrono>
#include <thread>
#include <unordered_set>
#include "core/geometry/surface/surface.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/utils/message.hpp"
#include "core/utils/numeric_utils.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/system_utils.hpp"


// TODO Planeは依存ファイルが多いのでBBクラス自体は依存したくない。依存する部分は外に出すべし。


//const double geom::BoundingBox::MAX_EXTENT  = geom::Surface::MAX_LENTGH;  // なぜか０
const double geom::BoundingBox::MAX_EXTENT  = 1E+35;

geom::BoundingBox geom::BoundingBox::universalBox()
{
	return BoundingBox(-MAX_EXTENT, MAX_EXTENT,-MAX_EXTENT, MAX_EXTENT,-MAX_EXTENT, MAX_EXTENT);
}

geom::BoundingBox geom::BoundingBox::emptyBox()
{
	return BoundingBox(0, 0, 0, 0, 0, 0);
}

// xmin, xmax, ymin, ymax, zmin, zmaxを返す。
std::array<double, 6> geom::BoundingBox::range() const
{
	return std::array<double, 6>{{xmin, xmax, ymin, ymax, zmin, zmax}};
}

std::string geom::BoundingBox::info() const
{
	std::stringstream ss;
	ss << "BoundingBox {" << toInputString() << "}";
	return ss.str();
}

std::string geom::BoundingBox::toInputString() const
{
	std::stringstream ss;
	auto r = range();
	ss << "{";
	for(size_t i = 0; i < r.size(); ++i) {
		ss << r.at(i);
		if(i != r.size()-1) ss << ", ";
	}
	ss << "}";
	return ss.str();
}

void geom::BoundingBox::expand(double factor)
{
	auto c = center();
	//mDebug() << "center in expand=" << c;
	xmin = c.x() - (c.x() - xmin)*factor;
	xmax = c.x() + (xmax - c.x())*factor;
	ymin = c.y() - (c.y() - ymin)*factor;
	ymax = c.y() + (ymax - c.y())*factor;
	zmin = c.z() - (c.z() - zmin)*factor;
	zmax = c.z() + (zmax - c.z())*factor;
}

bool geom::BoundingBox::empty() const
{
	return xmax-xmin < math::EPS || ymax-ymin < math::EPS || zmax-zmin < math::EPS;
}

bool geom::BoundingBox::isUniversal(bool compareStrinct) const
{
//    mDebug() << "MAX===" << MAX_EXTENT <<  geom::Surface::MAX_LENTGH;
	if(!compareStrinct) {
		return (xmin <= -MAX_EXTENT) || (xmax >= MAX_EXTENT ) ||
				(ymin <= -MAX_EXTENT) || (ymax >= MAX_EXTENT) ||
				(zmin <= -MAX_EXTENT) || (zmax >= MAX_EXTENT);
	} else {
		return (xmin <= -MAX_EXTENT) && (xmax >= MAX_EXTENT ) &&
				(ymin <= -MAX_EXTENT) && (ymax >= MAX_EXTENT) &&
				(zmin <= -MAX_EXTENT) && (zmax >= MAX_EXTENT);
	}
}

bool geom::BoundingBox::contains(const geom::BoundingBox &bb)
{
	return bb.xmin > xmin && bb.xmax < xmax
			&& bb.ymin > ymin && bb.ymax < ymax
			&& bb.zmin > zmin && bb.zmax < zmax;
}


// 直線とBBが交差するか判定する。
bool geom::BoundingBox::hasIntersection(const math::Point &pt, const math::Vector<3> &dir) const
{
	(void) pt;
	(void) dir;
	// TODO BBと直線の交差判定を実装する。そうしたらDimDeclaratorの自動推定にBBが使える
	return true;
}

void geom::BoundingBox::transform(const math::Matrix<4> &matrix)
{
	using Pt = math::Point;
	std::vector<Pt> points {
		Pt{xmin, ymin, zmin},
		Pt{xmin, ymin, zmax},
		Pt{xmin, ymax, zmin},
		Pt{xmin, ymax, zmax},
		Pt{xmax, ymin, zmin},
		Pt{xmax, ymin, zmax},
		Pt{xmax, ymax, zmin},
		Pt{xmax, ymax, zmax},
	};
	for(auto &pt: points) {
		math::affineTransform(&pt, matrix);
	}
	*this = std::move(geom::BoundingBox::fromPoints(points));
}

void geom::BoundingBox::translation(const math::Vector<3> &transVec)
{
	xmin += transVec.x();
	xmax += transVec.x();
	ymin += transVec.y();
	ymax += transVec.y();
	zmin += transVec.z();
	zmax += transVec.z();
}

std::vector<std::vector<geom::Plane>> geom::BoundingBox::boundingSurfaces(bool isInward) const
{
	using Vec = math::Vector<3>;
    if(isInward) {
        std::vector<geom::Plane> planes;
        planes.emplace_back("", Vec{ 1,  0,  0},  xmin);
        planes.emplace_back("", Vec{-1,  0,  0}, -xmax);
        planes.emplace_back("", Vec{ 0,  1,  0},  ymin);
        planes.emplace_back("", Vec{ 0, -1,  0}, -ymax);
        planes.emplace_back("", Vec{ 0,  0,  1},  zmin);
        planes.emplace_back("", Vec{ 0,  0, -1}, -zmax);
        return std::vector<std::vector<Plane>>{std::move(planes)};
    } else {
        return std::vector<std::vector<Plane>> {
            std::vector<Plane>{Plane("", Vec{-1,  0,  0}, -xmin)},
            std::vector<Plane>{Plane("", Vec{ 1,  0,  0},  xmax)},
            std::vector<Plane>{Plane("", Vec{ 0, -1,  0}, -ymin)},
            std::vector<Plane>{Plane("", Vec{ 0,  1,  0},  ymax)},
            std::vector<Plane>{Plane("", Vec{ 0,  0, -1}, -zmin)},
            std::vector<Plane>{Plane("", Vec{ 0,  0,  1},  zmax)}
        };
    }

}

geom::BoundingBox geom::BoundingBox::fromString(const std::string &str)
{
	std::vector<std::string> argVec = utils::splitString(",", utils::dequote(std::make_pair('{', '}'), str, true), true);
	auto params = utils::stringVectorTo<double>(argVec);
	if(params.size() != 6) 	throw std::invalid_argument("BoundingBox::fromSTring requires 6 parameters");

	return BoundingBox(params[0], params[1], params[2], params[3], params[4], params[5]);
}



geom::BoundingBox::BoundingBox(double x1, double x2, double y1, double y2, double z1, double z2)
	:xmin(x1), xmax(x2), ymin(y1), ymax(y2), zmin(z1), zmax(z2)
{
	if(xmin > xmax || ymin > ymax || zmin > zmax) {
		std::stringstream ss;
		ss << "box min is larger than max." << this->info();
		throw std::invalid_argument(ss.str());
	}
}

namespace {
using Pt = math::Point;
auto xpCompare = [](const Pt *pt1, const Pt *pt2){ return pt1->x() < pt2->x();};
auto ypCompare = [](const Pt *pt1, const Pt *pt2){ return pt1->y() < pt2->y();};
auto zpCompare = [](const Pt *pt1, const Pt *pt2){ return pt1->z() < pt2->z();};
auto xCompare = [](const Pt &pt1, const Pt &pt2){ return pt1.x() < pt2.x();};
auto yCompare = [](const Pt &pt1, const Pt &pt2){ return pt1.y() < pt2.y();};
auto zCompare = [](const Pt &pt1, const Pt &pt2){ return pt1.z() < pt2.z();};
}

// pointer版
geom::BoundingBox geom::BoundingBox::fromPoints(std::initializer_list<const math::Point *> lst)
{
	using cPt = const math::Point;
	std::vector<const math::Point*> points(lst.begin(), lst.end());
	std::sort(points.begin(), points.end(), [](cPt *p1, cPt *p2){return p1->x() < p2->x();});
	points.erase(std::unique(points.begin(), points.end(), [](cPt *p1, cPt *p2){return math::isSamePoint(*p1, *p2);}), points.end());
	if(points.size() < 4) return BoundingBox::universalBox();

	std::sort(points.begin(), points.end(), xpCompare);
	double xmin = points.front()->x(), xmax = points.back()->x();
	std::sort(points.begin(), points.end(), ypCompare);
	double ymin = points.front()->y(), ymax = points.back()->y();
	std::sort(points.begin(), points.end(), zpCompare);
	double zmin = points.front()->z(), zmax = points.back()->z();
	return BoundingBox(xmin, xmax, ymin, ymax, zmin, zmax);
}

geom::BoundingBox geom::BoundingBox::fromPoints(std::vector<math::Point> points)
{
	using cPt = const math::Point;
	//重複点は除く
	std::sort(points.begin(), points.end(), [](cPt &p1, cPt &p2){return p1.x() < p2.x();});
	points.erase(std::unique(points.begin(), points.end(), [](cPt &p1, cPt &p2){return math::isSamePoint(p1, p2);}), points.end());

	if(points.empty()) return emptyBox(); 	// 点がないならemptyを返す。
	if(points.size() == 1) return universalBox();  // 1点ならuniversal
	// 点数4以下は閉じたboxを作れないのでuniversalを返す。
	// が、異なる2点を選べなかった場合に包含されるのでsizeを直接調べる必要はない。

	/*
	 * pointsから面を作成し、残りの点が全て面上にあるかをチェックする。
	 * 問題は
	 * ・そもそも異なる3点が取れない。
	 * ・面を作る時に使う3点が同一線上にあると例外発生。
	 */

	// 最初に異なる2点を選ぶ。
	const math::Point *point1 = &points.front();
	const math::Point *point2 = nullptr;
	for(size_t i = 1; i < points.size(); ++i) {
		if(!math::isSamePoint(*point1, points.at(i))) {
			point2 = &points.at(i);
			break;
		}
	}
	// 異なる2点が得られなかったので諦めてuniversalを返す。
	if(point2 == nullptr) return BoundingBox::universalBox();


	// 次に2点と同じ直線上にない3点目を選ぶ
	math::Vector<3> vec12 = *point1 - *point2;
	const math::Point *point3 = nullptr;
	for(size_t i = 1; i < points.size(); ++i) {
		if(!math::isDependent(vec12, *point1 - points.at(i))) {
			point3 = &points.at(i);
			break;
		}
	}
	auto xmm = math::getMinMax(0, points);
	auto ymm = math::getMinMax(1, points);
	auto zmm = math::getMinMax(2, points);
	// 同一線上にない3点目が得られなかった場合
	if(point3 == nullptr) {
		// 閉じられないが1次元だけは束縛できる可能性がある。
		std::array<double, 6> arr{xmm.first, xmm.second, ymm.first, ymm.second, zmm.first, zmm.second};
		if(math::isDependent(vec12, math::Vector<3>{1, 0, 0})) {
			arr[2] = -BoundingBox::MAX_EXTENT;
			arr[3] =  BoundingBox::MAX_EXTENT;
			arr[4] = -BoundingBox::MAX_EXTENT;
			arr[5] =  BoundingBox::MAX_EXTENT;
		} else if(math::isDependent(vec12, math::Vector<3>{0, 1, 0})) {
			arr[0] = -BoundingBox::MAX_EXTENT;
			arr[1] =  BoundingBox::MAX_EXTENT;
			arr[4] = -BoundingBox::MAX_EXTENT;
			arr[5] =  BoundingBox::MAX_EXTENT;
		} else if(math::isDependent(vec12, math::Vector<3>{0, 0, 1})) {
			arr[0] = -BoundingBox::MAX_EXTENT;
			arr[1] =  BoundingBox::MAX_EXTENT;
			arr[2] = -BoundingBox::MAX_EXTENT;
			arr[3] =  BoundingBox::MAX_EXTENT;
		} else {
			// 軸平行でなければuniversal
			return BoundingBox::universalBox();
		}
		return BoundingBox(arr);
	}

	// 一直線上にない3点が取れたので平面を作成する。
	Plane plane("", *point1, *point2, *point3, Plane::NormalType::MCNP);

	bool isAllOnThePlane = true;
	for(size_t i = 3; i < points.size(); ++i) {
		if(std::abs(plane.distanceToPoint(points.at(i))) > math::EPS) isAllOnThePlane = false;
	}
	if(isAllOnThePlane) {
		// 全点が同一平面上にあってもその面が軸平行なら面の法線方向に無限大の大きさを持つBBが作成できる。
		auto normal = plane.normal();
		std::array<double, 6> arr{xmm.first, xmm.second, ymm.first, ymm.second, zmm.first, zmm.second};

		if(math::isDependent(normal, math::Vector<3>{1, 0, 0})) {
			arr[0] = -BoundingBox::MAX_EXTENT;
			arr[1] =  BoundingBox::MAX_EXTENT;
		} else if(math::isDependent(normal, math::Vector<3>{0, 1, 0})) {
			arr[2] = -BoundingBox::MAX_EXTENT;
			arr[3] =  BoundingBox::MAX_EXTENT;
		} else 	if(math::isDependent(normal, math::Vector<3>{0, 0, 1})) {
			arr[4] = -BoundingBox::MAX_EXTENT;
			arr[5] =  BoundingBox::MAX_EXTENT;
		} else {
			//mWarning() << "all the points are on the same plane. pts ===" << points;
			return BoundingBox::universalBox();
		}
		return BoundingBox(arr);
	}
	return BoundingBox(xmm.first, xmm.second, ymm.first, ymm.second, zmm.first, zmm.second);
}

/*
 * BB定義平面vectorから(BBの内側or境界上にある)有効な点のvectorを返す。
 */
std::vector<math::Point> getValidPoints(const std::vector<geom::Plane> &planes, const std::atomic_bool *timeoutFlag)
{
//mDebug() << "\nCreating tmpBB from planes num planes ===" << planes.size();
//for(auto &pl: planes) {
//	mDebug() << "    " << pl.toString();
//}

	if(timeoutFlag && timeoutFlag->load()) throw std::runtime_error("BoundingBox::fromPlanes timeout.");

	//3面未満の場合は交点なしで、4面未満の場合は妥当な交点なし、なのでこれらの場合空vectorを返す。
	if(planes.size() < 4) return std::vector<math::Point>();

	// 交点を求める。どの平面によって生成されたかも記憶しておく。
	// (面上の点は誤差で表裏判定が不安定になるから判定を避けるために使う)
	using PlaneSet = std::unordered_set<const geom::Plane*>;
	std::vector<std::pair<math::Point, PlaneSet>> pointsInfo;
	for(size_t i = 0; i < planes.size(); ++i) {
		for(size_t j = i+1; j < planes.size(); ++j) {
			for(size_t k = j +1; k < planes.size(); ++k) {
				const geom::Plane &pl1 = planes.at(i);
				const geom::Plane &pl2 = planes.at(j);
				const geom::Plane &pl3 = planes.at(k);
				math::Point pt = geom::Plane::intersection(pl1, pl2, pl3);
				if(pt.isValid()) {
					pointsInfo.emplace_back(std::make_pair(std::move(pt), PlaneSet{&pl1, &pl2, &pl3}));
					//mDebug() << "(i,j,k)=" << i << j << k << "reject前交点=" << pt;
				}
			}
			if(timeoutFlag && timeoutFlag->load()) throw std::runtime_error("BoundingBox::fromPlanes timeout.");
		}
	}

//mDebug() << "交点points=";
//for(auto info: pointsInfo) {
//	mDebug() << "pt=" << std::get<0>(info).toString();
//}


	// BBは論理積で繋がれた面セットのすべての面の表側に入らなければならない。
	// 故に面の裏にある点はBB外であるため、BB構築前に除去する。
	/*
	 * チェック対象は「その点が乗っている面"以外"の面に対して、"厳密"にforwardであるかどうか」
	 * たしか数値誤差が問題になった気が…
	 */
	for(auto &plane: planes) {
		// pointsInfoはvector<pair<Point, set<Plane*>>なので
		// 要素のfirstが点で、secondが点を計算するのに使われた面
		for(auto it = pointsInfo.begin(); it != pointsInfo.end();) {
			const PlaneSet & plSet = std::get<1>(*it);
//				if(plSet.find(&plane) != plSet.end() || plane.isForward(std::get<0>(*it) + math::Point::delta()*plane.normal())) {
//					++it;
//				} else {
//					//mDebug() << "point =" << std::get<0>(*it) <<" is  outside of plane=" << plane.toString();
//					it = pointsInfo.erase(it);
//				}

			if(plSet.find(&plane) == plSet.end() && !plane.isForward(std::get<0>(*it) + 10*math::Point::delta()*plane.normal())) {
				//mDebug() << "point =" << std::get<0>(*it) <<" is  outside of plane=" << plane.toString();
				it = pointsInfo.erase(it);
			} else {
				++it;
			}
		}
	}
	std::vector<math::Point> points;
	for(const auto& info: pointsInfo) {
		points.emplace_back(std::get<0>(info));
	}


//mDebug() << "最終残存points=";
//for(auto info: pointsInfo) {
//	mDebug() << "pt=" << std::get<0>(info).toString();
//}


	return points;
}

geom::BoundingBox geom::BoundingBox::fromPlanes(std::atomic_bool *timeoutFlag, const std::vector<std::vector<Plane> > &planeVectors)
{
	/*
	 * 3平面から1点ずつ求め、面の表側に入っていない点は削除し、4点以上でこれらの点を含むBBを作成する
	 */

	//mDebug() << "ENTER  BB::fromPlanes. planeVecs Size ===" << planeVectors.size();
	if(planeVectors.empty()) return BoundingBox::universalBox();  // boundingBoxなしなら当然無限大

	BoundingBox bb = BoundingBox::emptyBox();
	//BoundingBox bb = BoundingBox::universalBox();
	int num = -1;
	for(auto planes: planeVectors) {
		++num;

		//4面未満はbox作成不可なのでユニバーサルBoxを適当にカットして半無限BOXを返す
		if(planes.size() < 4) {
			auto tmpBB2 = BoundingBox::universalBox();
			// 面倒なので軸平行面以外ではカットしない。
			for(auto &plane: planes) {
				//mDebug() << "cutting BB by plane=" << plane.toString();
				tmpBB2 = BoundingBox::AND(tmpBB2, plane.generateBoundingBox());
			}
			bb = BoundingBox::OR(bb, tmpBB2);
			//mDebug() << "Calculated BB from plane vectors at i===" << num << bb.toInputString();
			continue;
		}

		std::vector<math::Point> points = getValidPoints(planes, timeoutFlag);
		BoundingBox tmpBB;

		if(points.size() == 0) {
			/*
			 *  残存点が0個なら体積ゼロのBBと考えてemptyを適用。非ゼロ個4個未満なら閉じられない=無限セルとしてuniversalBoxを適用
			 * → だめ。例えば無限長さ斜角柱の場合を考えると、この場合universalが正解
			 * 問題は交点がなかった場合に
			 * ・空集合だから交点がない場合と
			 * ・無限体系なので交点が無い場合の見分けがつかないこと。
			 * 面倒だが後者なら無限に近い距離にある±xyz平面との交点が存在するので理論的に区別できないわけではない。
			 */
			auto newPlanes = planes;
			newPlanes.emplace_back(Plane("", math::Vector<3>{-1,  0,  0}, -0.1*BoundingBox::MAX_EXTENT));
			newPlanes.emplace_back(Plane("", math::Vector<3>{ 1,  0,  0}, -0.1*BoundingBox::MAX_EXTENT));
			newPlanes.emplace_back(Plane("", math::Vector<3>{ 0, -1,  0}, -0.1*BoundingBox::MAX_EXTENT));
			newPlanes.emplace_back(Plane("", math::Vector<3>{ 0,  1,  0}, -0.1*BoundingBox::MAX_EXTENT));
			newPlanes.emplace_back(Plane("", math::Vector<3>{ 0,  0, -1}, -0.1*BoundingBox::MAX_EXTENT));
			newPlanes.emplace_back(Plane("", math::Vector<3>{ 0,  0,  1}, -0.1*BoundingBox::MAX_EXTENT));
			auto points2 = getValidPoints(newPlanes, timeoutFlag);

			if(points2.size() == 0) {
				tmpBB = BoundingBox::emptyBox();
			} else {
				// ここに来たら無限の長さのセルということ。
				// 軸平行面の可能性があるので、まだワンチャンある。
				tmpBB = BoundingBox::universalBox();
				for(const auto& plane: planes) {
					tmpBB = BoundingBox::AND(tmpBB, plane.generateBoundingBox());
				}
			}
		} else if(points.size() < 4){
			// 交点が4個未満なら閉多面体を構成できないのでuniversalになる。
			// 一応まだ軸平行面から構成される場合はワンチャンある。
			tmpBB = BoundingBox::universalBox();
			for(const auto& plane: planes) {
				tmpBB = BoundingBox::AND(tmpBB, plane.generateBoundingBox());
			}
		} else {
			tmpBB = BoundingBox::fromPoints(points);
//            mDebug() << "points cands=====" << points << "tmpBox===" << tmpBB.toInputString();
		}

		// planesのBB同士とORをとるとUniversalになるので
		// このループ内でuniversalBoxが発生したらuniversalBox確定。故にreturnする。
        if(tmpBB.isUniversal(true)) {
//            mDebug() << "tmpBB is universal===" << tmpBB.toInputString();
            return BoundingBox::universalBox();
        }

//        mDebug() << "Created BB for part index===" << num << " is " << tmpBB.toInputString();
		if(!tmpBB.empty()) bb = BoundingBox::OR(bb, tmpBB);
//        mDebug() << "bb is updated to ===" << bb.toInputString();

	}
//    mDebug() << "Returnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn bb ===" << bb.toInputString();
	return bb;
}

std::vector<std::vector<geom::Plane>>
	geom::BoundingBox::mergePlaneVectorsAnd(const std::atomic_bool *timeoutFlag,
											const std::vector<std::vector<Plane>> &vecs1,
											const std::vector<std::vector<Plane>> &vecs2)
{
	if(timeoutFlag && timeoutFlag->load()) throw std::runtime_error("BoundingBox::mergePlaneVectorAnd timeout");
	// 第二第三引数のどちらかが空なら他方を返す。
	if(vecs1.empty()) {
		return vecs2;
	} else if(vecs2.empty()) {
		return vecs1;
	}

//    mDebug() << "Enter  mergePlaneVectorsAnd";
//    mDebug() << "vecvec1=";
//    for(size_t i = 0; i < vecs1.size(); ++i) {
//        mDebug() << "i=" << i;
//        for(auto plane: vecs1.at(i)) {
//            mDebug() << "    pl=" << plane.toString();
//        }
//    }
//    mDebug() << "vecvec2=";
//    for(size_t i = 0; i < vecs2.size(); ++i) {
//        mDebug() << "i=" << i;
//        for(auto plane: vecs2.at(i)) {
//            mDebug() << "    pl=" << plane.toString();
//        }
//    }

	// 危険。ここで場合の数でベクトル要素数が一気にO(n^2)で爆発する場合がある。
	std::size_t size1 = 0, size2 = 0;
	for(auto &vec1: vecs1) size1 += vec1.size();
	for(auto &vec2: vecs2) size2 += vec2.size();
	// mDebug() << "free mem=" <<  utils::getFreeMemMB()*0.4 << "required=" << sizeof(Plane)*size1*size2*1e-6;
	if(utils::getAvailMemMB()*0.5 <= sizeof(Plane)*size1*size2*1e-6) {
		throw std::runtime_error  ("BoundingBox::mergePlaneVectorAnd no enough memory");
	}


	std::vector<std::vector<Plane>> retVec;
	for(auto &vec1: vecs1) {
		if(timeoutFlag && timeoutFlag->load()) throw std::runtime_error("BoundingBox::mergePlaneVectorAnd timeout");
		for(auto &vec2: vecs2) {
			//vec1.insert(vec1.end(), vec2.begin(), vec2.end());
			std::vector<Plane> planes = vec1;
			planes.insert(planes.end(), vec2.begin(), vec2.end());

			/*
			 * 同値はどのようにして検出するか？
			 * 1．法線と距離を比較する。
			 * 2．スマポにして同じアドレスを指しているか比較
			 * 方法2の方がスマートで速いけど、異なるインスタンスが同等なbounding面を生成する場合がある、
			 * というのと今の実装から大分変更が必要になるという欠点がある。
			 */
			// normal と distanceで比較して同値を削除する。
			std::sort(planes.begin(), planes.end());
			auto it = std::unique(planes.begin(), planes.end(),
								  [](const Plane &p1, const Plane &p2)
									{
										return  math::isSamePoint(p1.normal(), p2.normal())
												&& utils::isSameDouble(p1.distance(), p2.distance());
									});
			planes.erase(it, planes.end());

			// ・ここでPlaneの矛盾を検出し、その場合成分除去をしたい
			// normal*distanceが等しく、normalが等しくなければ同じ位置にある正反対の面なので
			// それらが同じvectorに含まれていれば矛盾となる。
			bool isInvalid = false;
			if(!planes.empty()) {
				for(size_t i = 0; i < planes.size()-1; ++i) {
					for(size_t j = i+1; j <  planes.size(); ++j) {
						math::Point p1 = planes.at(i).distance()*planes.at(i).normal();
						math::Point p2 = planes.at(j).distance()*planes.at(j).normal();
						if(math::isSamePoint(p1, p2)
								&& utils::isSameDouble(math::dotProd(planes.at(i).normal(), planes.at(j).normal()), -1.0)){
							isInvalid = true;
							break;
						}
					}
					if(isInvalid) break;
				}
			}

			if(!isInvalid) retVec.emplace_back(std::move(planes));
		}
	}

//    mDebug() << "returnVec=";
//    for(size_t i = 0; i < retVec.size(); ++i) {
//        mDebug() << "i=" << i;
//        for(const auto &plane: retVec.at(i)) {
//            mDebug() << "    pl=" << plane.toString();
//        }
//    }

	return retVec;
}



// b1またはb2を包含するBD(但しAABB)を作成。どちらかがemptyBoxならemptyでない方を返す
geom::BoundingBox geom::BoundingBox::OR(const BoundingBox &b1, const geom::BoundingBox &b2)
{
	if(b1.empty()) {
		return b2;
	} else if(b2.empty()) {
		return b1;
	} else {
	//	mDebug() << "OR for BB1=" << b1.toString() << " BB2=" << b2.toString();
	//	mDebug() << "OR result=" << BoundingBox(std::min(b1.xmin, b2.xmin), std::max(b1.xmax, b2.xmax),
	//											std::min(b1.ymin, b2.ymin), std::max(b1.ymax, b2.ymax),
	//											std::min(b1.zmin, b2.zmax), std::max(b1.zmax, b2.zmax)).toString();
		return BoundingBox(std::min(b1.xmin, b2.xmin), std::max(b1.xmax, b2.xmax),
						   std::min(b1.ymin, b2.ymin), std::max(b1.ymax, b2.ymax),
						   std::min(b1.zmin, b2.zmin), std::max(b1.zmax, b2.zmax));
	}
}
// b1かつb2を包含するBDを作成
geom::BoundingBox geom::BoundingBox::AND(const geom::BoundingBox &b1, const geom::BoundingBox &b2)
{
//	mDebug() << "AND for BB1=" << b1.toString() << " BB2=" << b2.toString();
	if(!isOverlapping(b1, b2)) {
		//mDebug() << "No overlap, in BB AND operation";
		return BoundingBox::emptyBox();
	}
	std::vector<double> xb{b1.xmin, b1.xmax, b2.xmin, b2.xmax};
	std::vector<double> yb{b1.ymin, b1.ymax, b2.ymin, b2.ymax};
	std::vector<double> zb{b1.zmin, b1.zmax, b2.zmin, b2.zmax};

	std::sort(xb.begin(), xb.end());
	std::sort(yb.begin(), yb.end());
	std::sort(zb.begin(), zb.end());

//	mDebug() << "AND result=" << BoundingBox(xb.at(1), xb.at(2), yb.at(1), yb.at(2), zb.at(1), zb.at(2)).toString();
	return BoundingBox(xb.at(1), xb.at(2), yb.at(1), yb.at(2), zb.at(1), zb.at(2));
}

std::vector<math::Point> geom::BoundingBox::vertices() const
{
	return std::vector<math::Point> {
		Vec{xmin, ymin, zmin},
		Vec{xmin, ymin, zmax},
		Vec{xmin, ymax, zmin},
		Vec{xmin, ymax, zmax},
		Vec{xmax, ymin, zmin},
		Vec{xmax, ymin, zmax},
		Vec{xmax, ymax, zmin},
		Vec{xmax, ymax, zmax}
	};
}


bool geom::isOverlapping(const geom::BoundingBox b1, const geom::BoundingBox &b2)
{
	if(b1.xmin > b2.xmax || b1.xmax< b2.xmin
	|| b1.ymin > b2.ymax || b1.ymax< b2.ymin
	|| b1.zmin > b2.zmax || b1.zmax< b2.zmin) {
		return false;
	} else {
		return true;
	}
}



bool isForwardPoint(const math::Point &pt, const std::set<geom::Plane> &pset)
{
	for(auto &pl: pset) {
		if(pl.isForward(pt)) return true;
	}
	return false;
}
bool isIncluded(const std::unordered_set<const geom::Plane*> &ppSet, const std::set<geom::Plane> &pset)
{
	for(auto &plane: pset) {
		if(ppSet.find(&plane) != ppSet.end()) return true;
	}
	return false;
}



bool geom::isSameBB(const geom::BoundingBox b1, const geom::BoundingBox &b2)
{
	return  utils::isSameDouble(b1.xmin, b2.xmin) && utils::isSameDouble(b1.xmax, b2.xmax) &&
			utils::isSameDouble(b1.ymin, b2.ymin) && utils::isSameDouble(b1.ymax, b2.ymax) &&
			utils::isSameDouble(b1.zmin, b2.zmin) && utils::isSameDouble(b1.zmax, b2.zmax);
}

bool geom::hasIntersection(const geom::BoundingBox bb, const geom::Plane &pl)
{
	// BBの格子点が全て面の同じ側にあればfalse
	auto points = bb.vertices();
	bool forward = pl.isForward(points.front());
	for(size_t i = 1; i < points.size(); ++i) {
		if(forward != pl.isForward(points.at(i))) return true;
	}
	return false;
}
