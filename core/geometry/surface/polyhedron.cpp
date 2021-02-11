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
#include "polyhedron.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <fstream>
#include <unordered_map>
#include <regex>
#include <sstream>
#include "plane.hpp"
#include "sphere.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/system_utils.hpp"

#include "vtksurfaceheaders.hpp"

namespace {

std::regex solidPattern("^solid");
std::regex outerLoopPattern(R"(^[\t ]*outer[\t ]+loop[\t ]*$)");
std::regex vertexPattern(R"(^[\t ]*vertex[\t ]*([-+0-9.eE]*)[\t ]*([-+0-9.eE]*)[\t ]*([-+0-9.eE]*)[\t ]*)");
std::regex endloopPattern(R"(^[\t ]*endloop[\t ]*$)");
std::regex endfacetPattern(R"(^[\t ]*endfacet[\t ]*$)");
std::regex endsoidPattern(R"(^[\t ]*endsolid[\t ]*$)");

// getfileと同様にifsからbuffに文字列を読み込む。このとき読み取り結果を小文字化する。
std::ifstream& getlineLower(std::ifstream &ifs, std::string &buff) {
	getline(ifs, buff);
	if(ifs) {
		utils::tolower(&buff);
	}
	return ifs;
}

// toleranceは絶対値ではなく割合指定
class LessCompare {
public:
    LessCompare(size_t index, double c): dirIndex_(index), tolerance_(c){}

	bool operator()(const std::shared_ptr<math::Point> &p1, const std::shared_ptr<math::Point> &p2)
	{
		auto p2Value = p2->data()[dirIndex_];
		double factor = (p2Value > 0) ? 1 - tolerance_ : 1 + tolerance_;
		return p1->data()[dirIndex_] <  p2Value*factor;
	}
private:
	size_t dirIndex_;
	double tolerance_;
};

}


bool geom::TriangleEdge::isInterior() const
{
	auto mid = 0.5*(tPair_.first->center() + tPair_.second->center());
	auto result1 = tPair_.first->isForward(mid), result2 = tPair_.second->isForward(mid);
	if(result1 != result2) {
		std::stringstream ss;
		ss << "Triangle is insideout, point = " << mid.toString()
		   << "Triangle1 =" << tPair_.first->toString() << "\n,"
		   << "Triangle2 =" << tPair_.second->toString() << "\n,"
		   << "is forward =" << result1 << ", " << result2;
		throw std::invalid_argument(ss.str());
	} else if (result1) {
		return false;
	} else {
		return true;
	}
}


std::string geom::TriangleElement::toString() const
{
	std::stringstream ss;
	ss << triangle_->toString() << ", neighbor =" << std::endl;
	for(const auto& n: neighbors_) {
		ss << "		" << n->toString();
	}
	return ss.str();
}

math::Point geom::TriangleElement::getIntersection(const math::Point &point,
												   const math::Vector<3> &direction) const
{
	//mDebug() << "Calculating intersection from point=" << point << ", dir=" << direction;
	/*
	 * Triangleとの交点を計算し、もし辺の上に交点がきていたら隣接Triangleかthisか
	 * 一定の基準でどちらかが交点を返すことにする。
	 */
//	std::shared_ptr<Triangle> triangle_;
//	std::unordered_set<std::shared_ptr<const Triangle>> neighbors_;
	auto retPair = triangle_->getIntersection2(point, direction);
	// 交点がエッジ上ではない場合
	if(!retPair.first) return retPair.second;

	// エッジ交点処理が不完全。まだ漏れる
	//mDebug() << "edge intersection found, pos, dir=" << point << direction << "triangle=" << triangle_->toString();

	// 交点がエッジ上の場合 三角形中心のz座標が小さい方に交点を設ける。z座標が同じならyが小さい方。
	std::shared_ptr<const Triangle> nextTriangle;
	for(const auto& tri: neighbors_) {
		if(!isSamePoint(tri->getIntersection(point, direction), math::Vector<3>::INVALID_VECTOR())) {
			nextTriangle = tri;
			break;
		}
	}
	// このTriangleのエッジ上に交点があるが隣の三角形に交点がない場合。基本エラーだが数値誤差的にありえる
	if(!nextTriangle) {
		mDebug() << "エッジ上交点なのに隣の三角に交点がない, point, dir=" << point << direction << ", tri=" << triangle_->toString();
		return retPair.second;
	}

	math::Point thisCenter = triangle_->center(), nextCenter = nextTriangle->center();
	if(thisCenter.z() < nextCenter.z()) {
		return retPair.second;
	} else if (thisCenter.z() > nextCenter.z()) {
		return math::Vector<3>::INVALID_VECTOR();  // 隣接するTriangleの方に交点を設けるからこちらは交点なしを返す
	} else if(thisCenter.y() < nextCenter.y()) {
		return retPair.second;
	} else if (thisCenter.y() > nextCenter.y()) {
		return math::Vector<3>::INVALID_VECTOR();
	} else if (thisCenter.x() < nextCenter.x()) {
		return retPair.second;
	} else if(thisCenter.x() > nextCenter.x()) {
		return math::Vector<3>::INVALID_VECTOR();
	} else {
		throw std::invalid_argument("Same Triangle ???");
	}
}



geom::PolyHedron::PolyHedron(const std::string &name,
                             const std::vector<std::shared_ptr<math::Point> > vertices,
                             const std::vector<geom::TriangleElement> &elems)
    :Surface("POLYHEDRON", name), uniqueVertices_(vertices), elements_(elems)
{
	bb_ = this->generateBoundingBox();
    boundingPlaneVectors_ = boundingPlanes();
}



std::unique_ptr<geom::PolyHedron> geom::PolyHedron::fromStlFile(const std::string &name, std::string filename, double tolerance, bool isReverse)
{
	using Pt = math::Point;
    if(filename.empty())  throw std::invalid_argument("Empty stl filename.");

	std::ifstream ifs(utils::utf8ToSystemEncoding(filename).c_str());
    if(ifs.fail()) throw std::invalid_argument(std::string("No such a file =") + filename);

	std::string buff;
	getlineLower(ifs, buff);
	buff = buff.substr(buff.find_first_not_of(" \t"));
//	auto strVec = utils::split(' ', buff, false, true);
	auto strVec = utils::splitString('\"', " ", buff, false);

	if(strVec.empty()) {
        throw std::invalid_argument("Header is empty.");
	} else if (!std::regex_search(strVec.at(0), solidPattern)) {
        throw std::invalid_argument(std::string("Header should be start with \"solid\", actual=") + strVec.at(1));
	}
	std::string solidname;
	if(strVec.size() >= 2) {
		solidname = strVec.at(1);
	}

	std::array<LessCompare, 3> lessWithTor {LessCompare(0, tolerance), LessCompare(1, tolerance), LessCompare(2, tolerance)};


	// pointをキーにしてその点を使用している三角形へのポインタをmapで格納しておく
	std::unordered_map<std::shared_ptr<Pt>, std::unordered_set<std::shared_ptr<Triangle>>> ptTriMap;
	std::vector<std::shared_ptr<Pt>> pointPool;  // z-orderでソートしてinsertすること。
	std::vector<std::shared_ptr<Triangle>> trianglePool;
	mDebug() << "Reading slt file, solid name =" << solidname;
	std::smatch sm;
	while(true) {
		getlineLower(ifs, buff);  // facet normal行は無視
		if(std::regex_search(buff, endsoidPattern)) break;
		getlineLower(ifs, buff);  // outer loop
        if(!std::regex_search(buff, outerLoopPattern)) {
            throw std::invalid_argument(std::string("keyword \"outer loop\" is expected but actual=") + buff);
        }

		// ここからvertex3行読み取り
		std::array<std::shared_ptr<Pt>, 3> parray;
		for(size_t vindex = 0; vindex < 3; ++vindex) {
			getlineLower(ifs, buff);  // vetex
			// ここから頂点処理
			if(std::regex_search(buff, sm, vertexPattern)) {
				assert(sm.size() == 4);
				auto pt = std::make_shared<Pt>(std::initializer_list<double>{
				                                   utils::stringTo<double>(sm.str(1)),
				                                    utils::stringTo<double>(sm.str(2)),
				                                    utils::stringTo<double>(sm.str(3))});

				std::vector<std::shared_ptr<Pt>>::iterator lowerItr = pointPool.begin(), upperItr = pointPool.end();
				// STLではz座標で整列している可能性が高いのでz方向から検索を始めるのが効率がいい
				for(int xyzindex = 2; xyzindex >= 0; --xyzindex) {
					auto itrPair = std::equal_range(lowerItr, upperItr, pt, lessWithTor.at(xyzindex));
					lowerItr = itrPair.first;
					upperItr = itrPair.second;

//					mDebug() << "index=" << xyzindex << "で範囲["
//					          << std::distance(pointPool.begin(), prevLow) <<", "<<  std::distance(pointPool.begin(), prevUp) << ") "
//					          << "を検索した結果"
//					          << "low, hight=" << std::distance(pointPool.begin(), lowerItr) <<  std::distance(pointPool.begin(), upperItr);


					// ptがプールになかった場合 lowerとupperが一致したら//
					if(lowerItr == upperItr) {
						assert(lowerItr <= upperItr);  // 整列済みpoolに重複がない場合lowerがupperより後ろに来ることはないはず
//						mDebug() << "プールにないので追加";
						pointPool.insert(lowerItr, pt);
						break;
					} else {
						// index(xyz)座標が同じ(or差が許容範囲の)点がpoolにある場合は検索続行
						// 但しindexが最後、即ち最後の座標軸まで検索が成功した場合は同値な点があるので処理する。
						if(xyzindex == 0) {
//							mDebug() << "現在チェック中の点=" << *pt.get() << "と比較対象=" << *lowerItr->get() << "は等価なのでプールのポインタを代入";
							assert(lowerItr != upperItr); // poolに重複がない場合lowerとupperは一致しないはず
							pt = *lowerItr;
						}
					}
				}
				parray[vindex] = pt;
			} else {
				throw std::invalid_argument("Invalid vertex input line =" + buff);
			}
		}


		// ここまでで三角形の頂点データ3個(parray)がプールと重複無く得られたのでTriangleクラスを作成する。
		bool ccw = true;
		if(isReverse) ccw = !ccw;
		// ただし点の統合を行った結果、三角形が点や辺に縮退している可能性がある。その場合invalid_argumentが投げられる。
		try {
			// stlはcoutner-clock-wiseなので調整する。
			trianglePool.emplace_back(std::make_shared<Triangle>(std::string(),  parray, tolerance, ccw));
			// 同時に頂点をキーにした、その頂点を利用しているTriangleのマップも作成
			for(const auto &vertex: trianglePool.back()->vertices()) {
				auto pos = ptTriMap.find(vertex);
				if(pos == ptTriMap.end()) {
					ptTriMap.emplace(vertex, std::unordered_set<std::shared_ptr<Triangle>>{trianglePool.back()});
				} else {
					pos->second.insert(trianglePool.back());
				}
			}
		} catch (std::invalid_argument &e) {
			std::stringstream ss;
			ss << "{";
			for(size_t i = 0; i < 3; ++i) {
				ss << *parray.at(i).get();
			}
			ss << "}";
			mWarning() << "Triangle construction with points=" << ss.str() << " failed and ignored. " << e.what();
		}

		getlineLower(ifs, buff);  // endloop
        if(!std::regex_search(buff, endloopPattern)) {
            throw std::invalid_argument(std::string("keyword \"endloop\" is expected but actual=") + buff);
        }
		getlineLower(ifs, buff);  // endfacet
        if(!std::regex_search(buff, endfacetPattern)) {
            throw std::invalid_argument(std::string("keyword \"endloop\" is expected but actual=") + buff);
        }

		if(ifs.eof()) break;
	}

	// 多面体中心を計算しておく
	math::Point center = math::Point{0,0,0};
	for (const auto& pt: pointPool) {
		center += *pt.get();
	}
    center = center/static_cast<double>(pointPool.size());

	// ここから三角形同士の位相情報を構築する。
	std::vector<TriangleElement> elements;
	//std::set<TriangleEdge> edgeSet;
	elements.reserve(trianglePool.size());
	for(size_t i = 0; i < trianglePool.size(); ++i) {
		//mDebug() << "i=" << i << "triangle=" << trianglePool.at(i)->toString();
		elements.emplace_back(trianglePool.at(i));  // まず位相情報なしの状態でインsタンスを作成する。
		for(const auto& vertex: elements.back().triangle()->vertices()) {// vertex：新しく作成したElementのTriangleを構成する頂点
			for(const auto &tri: ptTriMap.at(vertex)) { // tri: そのvertexを使用しているTriangle
				elements.back().registerIfNeighbor(tri);
			}
		}
	}

	// sanityチェック
	for(const auto& elem: elements) {
        mDebug() << "elem =" << elem.triangle()->toString();
        mDebug() << "	neighbors=";
        for(const auto& ntri: elem.neighbors()) {
            mDebug() << "		" << ntri->toString();
        }
		if(elem.neighbors().size() != 3) {
			throw std::invalid_argument("Number of neighbor element is not 3."
			                            "Polyhedron may not be closed or constructing phase information failed.");
		}
	}
	std::unique_ptr<PolyHedron> poly (new PolyHedron(name, pointPool, elements));
	poly->setFileName(filename);
	return poly;
}


std::string geom::PolyHedron::toInputString() const
{
	mWarning() << "Polyhedron cannot be convert to input string.";
	return "";
}

std::string geom::PolyHedron::toString() const
{
	std::stringstream ss;
	ss << Surface::toString() << "elemnts = " << std::endl;
	for(const auto& elem: elements_) {
		ss << "		" << elem.toString() << std::endl;
	}
	return ss.str();
}

std::unique_ptr<geom::Surface> geom::PolyHedron::createReverse() const
{
	std::unique_ptr<Surface> reversedPoly(new PolyHedron(Surface::reverseName(name_), uniqueVertices_, elements_));
	reversedPoly->setID(-1*ID_);
	reversedPoly->setFileName(fileName_);
	return reversedPoly;
}

void geom::PolyHedron::transform(const math::Matrix<4> &matrix)
{
    Surface::transform(matrix);
	for(auto &elem: elements_) {
		elem.transform(matrix);
	}
}

// この内外判定で面の直上はForward扱いになるか？→Triangleの交点計算と法線ベクトルが正しいかどうかによる
bool geom::PolyHedron::isForward(const math::Point &point) const
{
	auto direction = math::Vector<3>{1.1, 0.1, -0.1};  // 適当な方向
	// 内外判定交点の偶奇で決める
	//std::vector<math::Point> intersections;
	int interSectionCount = 0;
	//mDebug() << "point=" << point << "から方向=" << direction << "の交点探索";
	for(const auto& elem: elements_) {
		auto pt = elem.getIntersection(point, direction);
		if(!math::isSamePoint(pt, math::Point::INVALID_VECTOR())) {
			++interSectionCount;
			//intersections.emplace_back(pt);
		}
	}
	bool result = interSectionCount%2 == 0; // 偶数なら外側。
	return reversed_ ? !result : result;
}

math::Point geom::PolyHedron::getIntersection(const math::Point &point, const math::Vector<3> &direction) const
{
	if(!bb_.hasIntersection(point, direction)) return math::Point::INVALID_VECTOR();
	// 交点を求める時境界直上を線が通る時の交点を1つは用意する必要がある。隣接triangleの位置を参考にしてどちらの三角が交点を返すか決める。
	std::vector<math::Point> intersections;
	for(const auto& elem: elements_) {
		auto pt = elem.getIntersection(point, direction);
		if(!math::isSamePoint(pt, math::Point::INVALID_VECTOR())) intersections.emplace_back(pt);
	}
	double nearestDist = std::numeric_limits<double>::max();
	math::Point nearestPt = math::Point::INVALID_VECTOR();
	for(const auto &pt: intersections) {
		double dist = math::distance(point, pt);
		if(nearestDist > dist) {
			nearestDist = dist;
			nearestPt = pt;
		}
	}
	return nearestPt;
}


class CenterLess {
	using sptr = std::shared_ptr<math::Vector<3>>;
public:
	CenterLess(const math::Vector<3> &c)
		: center_(c)
    {}
	void setIndex(size_t i) {index_ = i;}
	bool operator()(const sptr &p1, const sptr &p2) {
		return (p1->at(index_) - center_.at(index_)) < (p2->at(index_) - center_.at(index_));
	}
private:
	const math::Vector<3> center_;
	size_t index_;
};

geom::BoundingBox geom::PolyHedron::generateBoundingBox() const
{
	using Pt = math::Point;
	using sptr = std::shared_ptr<Pt>;


	// そもそもVTKがopenMPビルドされている場合どう頑張っても落ちるので検知して警告か終了かしたい。がそんなものはないのでむり。
	mWarning() << "Drawing polyhedron cell causes segmentation fault if vtk is built with openMP.";

	if(uniqueVertices_.empty()) return BoundingBox();
	std::vector<std::shared_ptr<Pt>> vertices = uniqueVertices_;
	if(reversed_) {
		// 外接BOX
		std::sort(vertices.begin(), vertices.end(), [](const sptr &p1, const sptr &p2) { return p1->z() < p2->z();});
		double zmin = vertices.front()->z(), zmax = vertices.back()->z();
		assert(zmin < zmax);

		std::sort(vertices.begin(), vertices.end(), [](const sptr &p1, const sptr &p2) { return p1->y() < p2->y();});
		double ymin = vertices.front()->y(), ymax = vertices.back()->y();
		assert(ymin < ymax);

		std::sort(vertices.begin(), vertices.end(), [](const sptr &p1, const sptr &p2) { return p1->x() < p2->x();});
		double xmin = vertices.front()->x(), xmax = vertices.back()->x();
		assert(xmin < xmax);

		return BoundingBox(xmin, xmax, ymin, ymax, zmin, zmax);
	} else {
		/*
		 * 内接BOXの作り方。
		 * 1．中心点が多面体外→体積ゼロボックス
		 * 2．中心点が多面体内の場合、最近接面までの距離を半径とした円を作る。
		 *    この内接球のさらに内接立方体を内接BBとして返す。
		 */
		// 内接Box
		Pt center{0, 0, 0};
		for(const auto &p: uniqueVertices_) {
			center += *p.get();
		}
		assert(!uniqueVertices_.empty());
        center = center/static_cast<double>(uniqueVertices_.size());

		if(this->isForward(center)) return BoundingBox::emptyBox();

		double dist2 = std::numeric_limits<double>::max();  // 距離の二乗値
		for(const auto &elem: elements_) {
			dist2 = std::min(dist2, math::dotProd(elem.triangle()->normal(),
												  *(elem.triangle()->vertices().front().get()) - center));
		}
		double wid = 0.5*std::sqrt(dist2);
		return geom::BoundingBox(center.x()-wid, center.x()+wid, center.y()-wid, center.y()+wid, center.z()-wid, center.z()+wid);
	}

}

/*
 * 多面体の構成面を逐一返していると膨大な数になるのは明白なので、
 * BoundingBoxの面を返すのみにする。
 * その代わりBoundingBox生成時に外向き内向きを区別して、
 * 内接or外接Boxを設定する。
 *
 */
std::vector<std::vector<geom::Plane> > geom::PolyHedron::boundingPlanes() const
{
	using Vec = math::Vector<3>;

	auto range = bb_.range();
    if(reversed_) {
        std::vector<Plane> plVec;
        plVec.emplace_back("", Vec{ 1,  0,  0},  range[0], false);
        plVec.emplace_back("", Vec{-1,  0,  0}, -range[1], false);
        plVec.emplace_back("", Vec{ 0,  1,  0},  range[2], false);
        plVec.emplace_back("", Vec{ 0, -1,  0}, -range[3], false);
        plVec.emplace_back("", Vec{ 0,  0,  1},  range[4], false);
        plVec.emplace_back("", Vec{ 0,  0, -1}, -range[5], false);
        return std::vector<std::vector<Plane>>{std::move(plVec)};
    } else {
		// 外向きの場合BBが確実に内包されるように、内接面を返す必要がある。
		// が、Polyhedronではbb_が表裏に応じて外接/内接boxを保持しているので
		// ここで区別する必要はない。
        return std::vector<std::vector<Plane>>{
            std::vector<Plane>{Plane("", Vec{-1,  0,  0}, -range[0], false)},
            std::vector<Plane>{Plane("", Vec{ 1,  0,  0},  range[1], false)},
            std::vector<Plane>{Plane("", Vec{ 0, -1,  0}, -range[2], false)},
            std::vector<Plane>{Plane("", Vec{ 0,  1,  0},  range[3], false)},
            std::vector<Plane>{Plane("", Vec{ 0,  0, -1}, -range[4], false)},
            std::vector<Plane>{Plane("", Vec{ 0,  0,  1},  range[5], false)},
        };
}
}

std::shared_ptr<geom::Surface> geom::PolyHedron::makeDeepCopy(const std::string &newName) const
{
    std::vector<std::shared_ptr<math::Point>> vertices;
    vertices.reserve(uniqueVertices_.size());
    for(const auto &vertex: uniqueVertices_) vertices.emplace_back(std::make_shared<math::Point>(*vertex.get()));
    return std::make_shared<PolyHedron>(newName, vertices, elements_);
}

std::unique_ptr<geom::PolyHedron> geom::PolyHedron::createPolyhedron(const std::string &name,
																	 const std::vector<double> &params,
                                                                     const std::map<std::string, std::string> &paramMap,
																	 const math::Matrix<4> &trMatrix,
																	 bool warnPhitsCompat)
{
	if(warnPhitsCompat) mWarning() << "Polyhedron is not phits compatible";
	if(!params.empty()) {
        throw std::invalid_argument("Polyhedron requires only filename.");
	}
	std::string stlFileName;
	bool isReverse = false;
	if(paramMap.find("stl") != paramMap.end()) {
		stlFileName = paramMap.at("stl");
	} else if(paramMap.find("-stl") != paramMap.end()) {
		stlFileName = paramMap.at("-stl");
		isReverse = true;
	}
	std::unique_ptr<PolyHedron> poly = PolyHedron::fromStlFile(name, stlFileName, isReverse);
	if(!math::isSameMatrix(trMatrix, math::Matrix<4>()) && !math::isSameMatrix(trMatrix, math::Matrix<4>::IDENTITY())) {
		poly->transform(trMatrix);
	}
	return poly;
}

#ifdef ENABLE_GUI
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkImplicitDataSet.h>
#include <vtkImageData.h>
#include <vtkDataSet.h>
#include <vtkSTLReader.h>
#include <vtkImplicitModeller.h>
#include <vtkImplicitPolyDataDistance.h>


/*
 * このあたりの議論を参照すること
 * http://vtk.1045678.n5.nabble.com/How-to-transform-my-PolyData-to-ImplicitFunction-td1233112.html
 * http://vtkusers.public.kitware.narkive.com/R2Y9hAX4/vtkpolydata-intersection
 *
 * 方法1：マニュアルが正しければvktPolyDataにscalarが関連付けられていればvtkImplicitDataSetで陰関数が作れるはず → うまくいかない。諦める
 * 方法2：vtkImplicitPolyData http://www.vtkjournal.org/browse/publication/726 を使う → ちょっと修正してビルド通ったがsegv
 * 方法3：適当にうまいことPlaneの演算でできないものか？凸はANDで凹はORでとかで。→ かなり大変多分凸多面体に分割してからの方がマシ
 * 方法4：やっぱり凸多面体分割してからPlaneの演算で作る。→ 粒子追跡の方は現状でそれなりにうまく行きそうなので凸分割はやりたくない
 * 方法5：vtkImplicitModellerが使えそう。https://www.vtk.org/Wiki/VTK/Examples/Cxx/PolyData/ImplicitModeller→だめだった
 * ○方法6：vtkImplicitPolyDataDistanceを使う。→それなりにうまくいく。ただSMP実行すると出力エラーやsegv
 *
 */

vtkSmartPointer<vtkImplicitFunction> geom::PolyHedron::generateImplicitFunction() const
{
//	if(fileName_.empty()) {
//		m2Fatal("File name is empty for making polyhedron implicit function.");
//		abort();
//	}
//	mWarning() << "Visualization of Polyhedron may cause error or segv if multithread-enabled vtk is used.";
//	auto reader = vtkSmartPointer<vtkSTLReader>::New();
//	reader->SetFileName(fileName_.c_str());
//	reader->Update();
//	auto polyData = vtkSmartPointer<vtkPolyData>::New();
//	polyData = reader->GetOutput();

	// ここでファイルからではなく、位相再計算済みデータから構築すれば精度upするかと思ったが変わらない
	// 計算時間の差ははどうせ陰関数からポリゴン生成するところに比べれば誤差の範囲だからどっちでも良い。
	auto points = vtkSmartPointer<vtkPoints>::New();
	for(const auto& pt: uniqueVertices_) {
		points->InsertNextPoint(pt->x(), pt->y(), pt->z());
	}
	auto polys = vtkSmartPointer<vtkCellArray>::New();
	for(size_t i = 0; i < elements_.size(); ++i) {
		polys->InsertNextCell(3);
		polys->InsertCellPoint(getVertexIndexFromSortedVector(uniqueVertices_, elements_.at(i).triangle()->vertices()[0]));
		polys->InsertCellPoint(getVertexIndexFromSortedVector(uniqueVertices_, elements_.at(i).triangle()->vertices()[1]));
		polys->InsertCellPoint(getVertexIndexFromSortedVector(uniqueVertices_, elements_.at(i).triangle()->vertices()[2]));
	}
	auto polyData = vtkSmartPointer<vtkPolyData>::New();
	polyData->SetPoints(points);
	polyData->SetPolys(polys);

    auto scalars = vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetNumberOfValues(polyData->GetNumberOfPoints());
    for (int i = 0; i < polyData->GetNumberOfPoints(); i++) {
        scalars->SetValue(i, 0);
    }
    polyData->GetPointData()->SetScalars(scalars);

	// ############# implicitPolyDataDistanceを使う方法
	auto polyDist = vtkSmartPointer<vtkImplicitPolyDataDistance>::New();
	polyDist->SetTolerance(1e-19);
	polyDist->SetInput(polyData);
	//polyDist->SetDebug(true);
	/*
	 * NOTE vtkでMulti threadが有効化されていると、誤差で粒が出たり、面に孔があいたりする。最悪segv
	 * NOTE (linux) マルチスレッド実行でvtkImplicitPolyDataDistance::->Locator->FindClosestPointでバグる。
	 * シングルスレッド debug/releaseは大丈夫
	 */

	if(reversed_) return Surface::getVtkCompliment(polyDist);
	return polyDist;

//	// ############# implicitDataSetを使う方法
//	// polyDataにscalarをセットしてみる
//	auto scalars = vtkSmartPointer<vtkDoubleArray>::New();
//	scalars->SetNumberOfValues(polyData->GetNumberOfPoints());
//	for (int i = 0; i < polyData->GetNumberOfPoints(); i++) {
//		scalars->SetValue(i, 0);
//	}
//	polyData->GetPointData()->SetScalars(scalars);
//	auto implicitDataSet = vtkSmartPointer<vtkImplicitDataSet>::New();
//	implicitDataSet->SetDataSet(polyData);
//	return implicitDataSet;



//	// ################## implicitModellerを使う方法
//	// vtkImplicitModelerでvtkPolyData→
//	// 要するにimplicitModer出力をimplicitDataSet::SetDataSetしてからupcastすれば良い。
//	// https://public.kitware.com/pipermail/vtkusers/2008-May/046323.html
//	// vtkImplicitModeller::GetOutputの返り値は*vtkImageData
//	// vtkImplicitDataSet::SetDataSetは*vtkDataSetを引数にとる。vtkDataSetははvtkImageDataの基底クラス
//	auto modeller = vtkSmartPointer<vtkImplicitModeller>::New();
//	modeller->SetSampleDimensions(50, 50, 50);
//	modeller->SetInputData(polyData);
//	modeller->AdjustBoundsOn();
//	modeller->SetAdjustDistance(.1); // Adjust by 10%
//	modeller->SetMaximumDistance(.1);
//	modeller->Update();
//	auto implicitDataSet = vtkSmartPointer<vtkImplicitDataSet>::New();
//	implicitDataSet->SetDataSet(modeller->GetOutput());
//	return implicitDataSet;

}
#endif

std::array<LessCompare, 3> tmpless {LessCompare(0, 1e-14), LessCompare(1, 1e-14), LessCompare(2, 1e-14)};
// z, y, x座標の順に昇順でソートされたvertices_から該当するスマートポインタのindexを返す
int geom::getVertexIndexFromSortedVector(const std::vector<std::shared_ptr<math::Point> > &vertices,
										 const std::shared_ptr<math::Point> &pt) {
	using Pt = math::Point;
	std::vector<std::shared_ptr<Pt>>::const_iterator lowerItr = vertices.begin(), upperItr = vertices.end();
	// STLではz座標で整列している可能性が高いのでz方向から検索を始めるのが効率がいい
	for(int xyzindex = 2; xyzindex >= 0; --xyzindex) {
		auto itrPair = std::equal_range(lowerItr, upperItr, pt, tmpless.at(xyzindex));
		lowerItr = itrPair.first;
		upperItr = itrPair.second;

		// ptがプールになかった場合 lowerとupperが一致したら//
		if(lowerItr == upperItr) {
			throw std::invalid_argument("No point in vertices");
		} else {
			if(xyzindex == 0) {
                return static_cast<int>(std::distance(vertices.begin(), lowerItr));
			}
		}
	}
	throw std::invalid_argument("no point in vertices");

//	for(int i = 0; i < static_cast<int>(vertices.size()); ++i) {
//		if(pt == vertices.at(i)) return i;
//	}
//	throw std::invalid_argument("no point in vertices");

}
