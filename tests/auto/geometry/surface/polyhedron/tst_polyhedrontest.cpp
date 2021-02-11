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
#include <fstream>
#include <string>

#include <QString>
#include <QtTest>

#include "core/geometry/surface/polyhedron.hpp"

using namespace math;
using namespace geom;
using Pt = math::Point;
using Vec = math::Vector<3>;

class PolyhedronTest : public QObject
{
	Q_OBJECT

public:
	PolyhedronTest();

private Q_SLOTS:
	void testSrcPit();
	void testCase1();

private:
	std::string testFileName_;
};

PolyhedronTest::PolyhedronTest()
	: testFileName_("test.stl")
{
	std::ofstream ofs(testFileName_.c_str());
	ofs <<
	R"(solid ascii
	facet normal 0.57735 0.57735 0.57735
	 outer loop
	  vertex 5 5 -14.802
	  vertex -14.802 5 5.0001
	  vertex 5 -14.802 5
	 endloop
	endfacet
	facet normal -0.57735 0.57735 0.57735
	 outer loop
	  vertex 5 -14.802 5
	  vertex 24.802 5 5
	  vertex 5 5 -14.802
	 endloop
	endfacet
	facet normal 0.57735 -0.57735 0.57735
	 outer loop
	  vertex 5 5 -14.802
	  vertex 5 24.802 5
	  vertex -14.802 5 5
	 endloop
	endfacet
	facet normal -0.57735 -0.57735 0.57735
	 outer loop
	  vertex 24.802 5 5
	  vertex 5 24.802 5
	  vertex 5 5 -14.802
	 endloop
	endfacet
	facet normal 0.57735 0.57735 -0.57735
	 outer loop
	  vertex 5 -14.802 5
	  vertex -14.802 5 5
	  vertex 5 5 24.802
	 endloop
	endfacet
	facet normal -0.57735 0.57735 -0.57735
	 outer loop
	  vertex 5 -14.802 5
	  vertex 5 5 24.802
	  vertex 24.802 5 5
	 endloop
	endfacet
	facet normal 0.57735 -0.57735 -0.57735
	 outer loop
	  vertex -14.802 5 5
	  vertex 5 24.802 5
	  vertex 5 5 24.802
	 endloop
	endfacet
	facet normal -0.57735 -0.57735 -0.57735
	 outer loop
	  vertex 24.802 5 5
	  vertex 5 5 24.802
	  vertex 5 24.802 5
	 endloop
	endfacet
	endsolid	)" << std::endl;
}

void PolyhedronTest::testSrcPit()
{
    std::unique_ptr<PolyHedron> poly = PolyHedron::fromStlFile("poly1", "test.stl", 1e-15,  true);
	auto result = poly->getIntersection(math::Point{-50, 0, 0}, math::Vector<3>{1, 0, 0});
	math::Point expect{-25, 0, 0};
	mDebug() << "result=" << result;
	mDebug() << "expect=" << expect;
	QVERIFY((result - expect).abs() < 1e-6);
	QVERIFY(poly->isForward(math::Point{-50, 0, 0}));
}


void PolyhedronTest::testCase1()
{
	// データには1e-4のズレを付けているのでそれ許容範囲がそれ以下の場合は例外発生になる。
	QVERIFY_EXCEPTION_THROWN(PolyHedron::fromStlFile("poly1", testFileName_, 1e-6,  true), std::invalid_argument);

	std::unique_ptr<PolyHedron> poly = PolyHedron::fromStlFile("poly1", testFileName_, 1e-3,  true);
	QVERIFY(poly->isForward(Pt{0, 0, 0}) == false);
	QVERIFY(poly->isForward(Pt{1000, 0, 0}) == true);
	mDebug() << "原点から{1, 0, 0}方向の交点は…=" << poly->getIntersection(Pt{0, 0, 0}, Vec{1, 0, 0});
	mDebug() << "ーxから{1, 0, 0}方向の交点は…=" << poly->getIntersection(Pt{-100, 0, 0}, Vec{1, 0, 0});
	mDebug() << "+yから{0, -1, 0}方向の交点は…=" << poly->getIntersection(Pt{0, 100, 0}, Vec{0, -1, 0});
	mDebug() << "-yから{0,  1, 0}方向の交点は…=" << poly->getIntersection(Pt{0, -100, 0}, Vec{0, 1, 0});
	mDebug() << "BoundingBox=" << poly->generateBoundingBox().toInputString();
}

QTEST_APPLESS_MAIN(PolyhedronTest)

#include "tst_polyhedrontest.moc"
