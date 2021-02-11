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
#ifndef POLYHEDRON_HPP
#define POLYHEDRON_HPP

#include <array>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <set>
#include <string>
#include <vector>

#include "core/geometry/cell/boundingbox.hpp"
#include "core/math/nvector.hpp"
#include "surface.hpp"
#include "triangle.hpp"

namespace geom {

struct TriangleEdge {
	TriangleEdge(const std::shared_ptr<Triangle>& t1, const std::shared_ptr<Triangle> &t2)
	{

		if(t1 < t2) {
			tPair_ = std::make_pair(t1, t2);
		} else if (t2 < t1) {
			tPair_ = std::make_pair(t2, t1);
		} else {
			std::cerr << "t1 should be != t2" << std::endl;
			abort();
		}
	}
	// edgeで接している2つの三角形の二面角の多面体(中心がcenter)側が180度未満ならtrue
	bool isInterior() const;
	// tPair_は first < second となるように格納することで一意になるようにする。
	std::pair<std::shared_ptr<Triangle>, std::shared_ptr<Triangle>> tPair_;
};
inline
bool operator < (const TriangleEdge &te1, const TriangleEdge &te2) {
	return te1.tPair_ < te2.tPair_;
}


// 三角形要素。三角形に加えてエッジを共有する隣接三角形へのポインタを持っている。
class TriangleElement {
public:
	explicit TriangleElement(const std::shared_ptr<Triangle> &tri): triangle_(tri){;}
	bool registerIfNeighbor(const std::shared_ptr<const Triangle> tri){
		if(tri != triangle_ && triangle_->isNeighbor(*tri.get())) {
			neighbors_.insert(tri);
			return true;
		} else {
			return false;
		}
	}

	const std::shared_ptr<Triangle> triangle() const {return triangle_;}
	const std::unordered_set<std::shared_ptr<const Triangle>>& neighbors() const {return neighbors_;}
	std::string toString() const;
	void transform(const math::Matrix<4> &matrix) {triangle_->transform(matrix);}

	math::Point getIntersection(const math::Point &point, const math::Vector<3>& direction) const;
private:
	std::shared_ptr<Triangle> triangle_;
	std::unordered_set<std::shared_ptr<const Triangle>> neighbors_;
};




class PolyHedron : public Surface
{
public:
	PolyHedron(const std::string &name,
	           const std::vector<std::shared_ptr<math::Point>> vertices,
	           const std::vector<TriangleElement> &elems);

	size_t numPoints() {return elements_.size();}
	static std::unique_ptr<PolyHedron> fromStlFile(const std::string &name, std::string filename, double tolerance = 1e-6, bool isReverse = false);

	// virtualの実装
	std::string toInputString() const override;
	std::string toString() const override;
	std::unique_ptr<Surface> createReverse() const override;
	void transform(const math::Matrix<4> &matrix) override;
	bool isForward(const math::Point &point) const override;
	math::Point getIntersection(const math::Point &point, const math::Vector<3>& direction) const override;

	BoundingBox generateBoundingBox() const override;
	std::vector<std::vector<Plane>> boundingPlanes() const override;
    std::shared_ptr<Surface> makeDeepCopy(const std::string &newName) const override;

    static std::unique_ptr<PolyHedron> createPolyhedron(const std::string &name,
                                                const std::vector<double> &params,
                                                const std::map<std::string, std::string> &paramMap,
                                                const math::Matrix<4> &trMatrix,
                                                bool warnPhitsCompat);

#ifdef ENABLE_GUI
public:
	virtual vtkSmartPointer<vtkImplicitFunction> generateImplicitFunction() const override;
#endif


private:
	std::vector<std::shared_ptr<math::Point>> uniqueVertices_;  // 点を重複無く格納したvector
	std::vector<TriangleElement> elements_;
	BoundingBox bb_;
	std::set<TriangleEdge> edges_;
};


int getVertexIndexFromSortedVector(const std::vector<std::shared_ptr<math::Point>> &vertices,
			   const std::shared_ptr<math::Point> &pt);



}  // end namespace geom
#endif // POLYHEDRON_HPP
