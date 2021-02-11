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
#include <QString>
#include <QtTest>

#include "core/utils/message.hpp"
#include "core/geometry/surface/cylinder.hpp"

using namespace std;
using namespace geom;
using namespace math;

class CylinderTest : public QObject
{
	Q_OBJECT
public:
	CylinderTest();

private Q_SLOTS:
	void testZCylinderForward();
	void testCylinderForward();
	void testIntersection();
	void testIntersection2();
	void testIntersection3();
};


CylinderTest::CylinderTest() {}

void CylinderTest::testZCylinderForward()
{
	// z方向上向き、半径10cmの円筒
	std::unique_ptr<Cylinder> cyl1(new Cylinder("cy1", Point{0, 0, 0}, Vector<3>{0, 0, 1}, 10));
	auto cyl2 = cyl1->createReverse();
	QCOMPARE(cyl1->isForward(Point{5,  0,   0}), false);  // (5,0,0)は当然内側=backward
	QCOMPARE(cyl2->isForward(Point{5,  0,   0}), true);   //
	QCOMPARE(cyl1->isForward(Point{5,  0, 100}), false);  // z方向に動いた点でも結果は同じ
	QCOMPARE(cyl2->isForward(Point{5,  0, 100}), true);   //
	QCOMPARE(cyl1->isForward(Point{10, 0, 100}), true);   // 円筒面上の場合表面前側
	QCOMPARE(cyl2->isForward(Point{10, 0, 100}), false);  //               裏面後方
}

void CylinderTest::testCylinderForward()
{
	//z=10の高さに(1, 1, 0)方向の半径10cm円筒がある。
	std::unique_ptr<Cylinder> cyl1(new Cylinder("cy1", Point{0, 0, 10}, Vector<3>{1, 1, 0}, 10));
	auto cyl2 = cyl1->createReverse();
	QCOMPARE(cyl1->isForward(Point{5,  5,  5}), false);  // (5,5,5)は当然表面後側
	QCOMPARE(cyl2->isForward(Point{5,  5,  5}), true);   //
	QCOMPARE(cyl1->isForward(Point{100, 100, 0}), true);   // 円筒面上の場合表面前側
	QCOMPARE(cyl2->isForward(Point{100, 100, 0}), false);  //               裏面後方
}

void CylinderTest::testIntersection()
{
	// z方向上向き、半径10cmの円筒
	std::unique_ptr<Cylinder> cyl1(new Cylinder("cy1", Point{0, 0, -20}, Vector<3>{0, 0, 1}, 10));
	math::Point sect1 = cyl1->getIntersection(math::Point{-100, 5, 0}, math::Vector<3>{1, 0, 0});
	math::Point expect1{-5*std::sqrt(3), 5, 0};
	for(size_t i = 0; i < expect1.size(); ++i) {
		QCOMPARE(sect1.data()[i], expect1.data()[i]);
	}
	math::Point sect2 = cyl1->getIntersection(math::Point{-1, 5, 0}, math::Vector<3>{1, 0, 0});
	math::Point expect2{5*std::sqrt(3), 5, 0};
	for(size_t i = 0; i < expect1.size(); ++i) {
		QCOMPARE(sect2.data()[i], expect2.data()[i]);
	}
}

void CylinderTest::testIntersection2()
{
	// (1, 1, 0)向き、半径20cmの円筒
	std::unique_ptr<Cylinder> cyl1(new Cylinder("cy1", Point{0, 0, 0}, Vector<3>{1, 1, 0}, 20));
	math::Point pt{-40*std::sqrt(2.0), 40*std::sqrt(2.0), 0};
	math::Point sect1 = cyl1->getIntersection(pt, math::Vector<3>{1, 0, 0});
	math::Point expect1{20.0*std::sqrt(2), 40*std::sqrt(2), 0};
	for(size_t i = 0; i < expect1.size(); ++i) {
		QCOMPARE(sect1.at(i), expect1.at(i));
	}

	math::Point expect2{-10*std::sqrt(2), 10*std::sqrt(2), 0};
	auto sect2 = cyl1->getIntersection(pt, math::Vector<3>{1, -1, 0});
	for(size_t i = 0; i < expect2.size(); ++i) {
		QCOMPARE(sect2.at(i), expect2.at(i));
	}
	auto sect3 = cyl1->getIntersection(20*pt, math::Vector<3>{1, -1, 0});
	for(size_t i = 0; i < expect2.size(); ++i) {
		QCOMPARE(sect3.at(i), expect2.at(i));
	}
}

void CylinderTest::testIntersection3()
{
	// (1, 0, 0)向き 半径10cmの円筒
	std::unique_ptr<Cylinder> cyl1(new Cylinder("cy1", Point{0, 0, 0}, Vector<3>{1, 0, 0}, 10));
	Point pt{-20, 20, 0};
	Point section = cyl1->getIntersection(pt, Vector<3>{0, -1, 0});
	Point expected = Point{-20, 10, 0};
	mDebug() << "交点=" << section;
	for(size_t i = 0; i < expected.size(); ++i) {
		QCOMPARE(section.at(i), expected.at(i));
	}
}

QTEST_APPLESS_MAIN(CylinderTest)

#include "tst_cylindertest.moc"
