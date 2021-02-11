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
#include <utility>
#include <vector>
#include <QString>
#include <QtTest>


#include "core/geometry/surface/triangle.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/utils/utils.hpp"
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"
#include "core/utils/message.hpp"
#include "core/utils/numeric_utils.hpp"

using namespace geom;
using Pt = math::Point;
using Vec = math::Vector<3>;
using Array3 = std::array<Pt, 3>;


Q_DECLARE_METATYPE(Pt);
Q_DECLARE_METATYPE(Array3);

class TriangleTest : public QObject
{
	Q_OBJECT

public:
	TriangleTest();

private Q_SLOTS:
	void testNormal_data();
	void testNormal();
	void testForward();
	void testIntersection();

private:
	double axisIntersect_ = 15;
	Triangle tri1_, tri2_, tri3_;

};

#define COMPAREPOINTS(p1, p2) \
if(utils::isSameDouble(p1.abs(), 0, 1e-10)) { \
	QVERIFY((p1 - p2).abs() < 1e-10); \
} else { \
	QVERIFY((p1 - p2).abs()/p1.abs() < 1e-10); \
} \


TriangleTest::TriangleTest()
	: tri1_(Triangle("tri1", Array3{Pt{0, 0, 0}, Pt{axisIntersect_, 0, 0}, Pt{0, axisIntersect_, 0}})),
	  tri2_(Triangle("tri2", Array3{Pt{0, 0, axisIntersect_}, Pt{0, axisIntersect_, 0}, Pt{axisIntersect_, 0, 0}})),
	  tri3_(Triangle("tri3", Array3{Pt{0, 0, axisIntersect_}, Pt{axisIntersect_, 0, 0}, Pt{0, axisIntersect_, 0}}))
{
}


void TriangleTest::testNormal_data()
{
	QTest::addColumn<Pt>("p0");
	QTest::addColumn<Pt>("p1");
	QTest::addColumn<Pt>("p2");
	QTest::addColumn<Vec>("expectedNormal");

	QTest::newRow("Tri1 right triangle") << Pt{0, 0, 0}  <<  Pt{axisIntersect_, 0, 0} <<  Pt{0, axisIntersect_, 0} << Vec{0, 0, -1};
	QTest::newRow("Tri2 normal 1 1 1")   << Pt{0, 0, axisIntersect_} <<  Pt{0, axisIntersect_, 0} <<  Pt{axisIntersect_, 0, 0} << (Vec{1, 1, 1}).normalized();
	QTest::newRow("Tri3 reverse tri2")   << Pt{0, 0, axisIntersect_} <<  Pt{axisIntersect_, 0, 0} <<  Pt{0, axisIntersect_, 0} << (Vec{-1, -1, -1}).normalized();
}


void TriangleTest::testNormal()
{
	QFETCH(Pt, p0);
	QFETCH(Pt, p1);
	QFETCH(Pt, p2);
	QFETCH(Vec, expectedNormal);

	Triangle tri("tri", Array3{p0, p1, p2});
	QVERIFY(math::isSamePoint(tri.normal(), expectedNormal));
}



void TriangleTest::testForward()
{
	//
	std::vector<Pt> pts{Pt{0, 0, 0}, Pt{100, 100, 200}, Pt{-100, 0, 0}};
	std::array<bool, 3> ex1{true, false, true};  // 原点は面の直上なのでforward判定する。
	std::array<bool, 3> ex2{false, true, false};
	std::array<bool, 3> ex3{true, false, true};
	for(size_t i = 0; i < pts.size(); ++i) {
		//mDebug() << "i=" << i << "for tri1, expected=" << ex1.at(i) << "actual=" << tri1_.isForward(pts.at(i));
		QCOMPARE(tri1_.isForward(pts.at(i)), ex1.at(i));
		//mDebug() << "i=" << i << "for tri2, expected=" << ex2.at(i) << "actual=" << tri2_.isForward(pts.at(i));
		QCOMPARE(tri2_.isForward(pts.at(i)), ex2.at(i));
		//mDebug() << "i=" << i << "for tri3, expected=" << ex3.at(i) << "actual=" << tri3_.isForward(pts.at(i));
		QCOMPARE(tri3_.isForward(pts.at(i)), ex3.at(i));
	}
}

void TriangleTest::testIntersection()
{
	double yoffset = 6;
	std::vector<std::pair<Pt, Vec>> pdPair {
		{Pt{-100, 0, 0}, Vec{1, 0, 0}},  // x軸上負から正方向
		{Pt{-100, yoffset, 0}, Vec{ 1, 0, 0}},  // x軸上負から正方向y方向へ少しずれる
		{Pt{222,  yoffset, 0}, Vec{-1, 0, 0}},  // x軸上正から負方向
		{Pt{9999, yoffset, 0}, Vec{ 1, 0, 0}},  // x軸上正から正方向とおく
		{Pt{1000, 1000, 1000}, (Vec{1, 1, 1}).normalized()}, // {1,1,1}軸上 正から正方向
		{Pt{1000, 1000, 1000}, (Vec{-1, -1, -1}).normalized()}, // {1,1,1}軸上 正から負方向
	};


	std::vector<Pt> tri1exp{
		Pt::INVALID_VECTOR(),
		Pt::INVALID_VECTOR(),
		Pt::INVALID_VECTOR(),
		Pt::INVALID_VECTOR(),
		Pt::INVALID_VECTOR(),
		Pt{0, 0, 0}
	};
	assert(pdPair.size() == tri1exp.size());
	for(size_t i = 0; i < pdPair.size(); ++i){
		auto result = tri1_.getIntersection(pdPair.at(i).first, pdPair.at(i).second);
		//mDebug() << "i=" << i << "expect=" << tri1exp.at(i) << ", result=" << result;
        COMPAREPOINTS(result, tri1exp.at(i))
	}
//	auto tri1Rev = tri1_.createReverse();
//	auto pt1r = Pt{10-yoffset, yoffset, 0};
//	auto rResult = tri1Rev->getIntersection(pdPair.at(1).first, pdPair.at(1).second);
//	mDebug() << "rexp = " << pt1r << "rResult=" << rResult;
//	QVERIFY((rResult - pt1r).abs() < rResult.abs()*1e-10);

	std::vector<Pt> tri2exp{
		Pt{axisIntersect_, 0, 0},
		Pt{axisIntersect_ - yoffset, yoffset, 0},
		Pt{axisIntersect_ - yoffset, yoffset, 0},
		Pt::INVALID_VECTOR(),
		Pt::INVALID_VECTOR(),
		Pt{axisIntersect_/3, axisIntersect_/3, axisIntersect_/3}
	};
	assert(pdPair.size() == tri2exp.size());
	for(size_t i = 0; i < pdPair.size(); ++i){
		auto result = tri2_.getIntersection(pdPair.at(i).first, pdPair.at(i).second);
		//mDebug() << "i=" << i << "expect=" << tri2exp.at(i) << ", result=" << result;
		COMPAREPOINTS(result, tri2exp.at(i));
//		QVERIFY((result-tri2exp.at(i)).abs() < result.abs()*1e-10);
	}

	// tri2とtri3は表裏逆なだけなので交点は同じ
	for(size_t i = 0; i < pdPair.size(); ++i){
		auto result = tri3_.getIntersection(pdPair.at(i).first, pdPair.at(i).second);
		//mDebug() << "i=" << i << "expect=" << tri2exp.at(i) << ", result=" << result;
		COMPAREPOINTS(result, tri2exp.at(i));
//		QVERIFY((result-tri2exp.at(i)).abs() < result.abs()*1e-10);
	}
}

QTEST_APPLESS_MAIN(TriangleTest)

#include "tst_triangletest.moc"
