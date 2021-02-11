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



#include "core/geometry/surface/surface.hpp"
#include "core/geometry/surface/sphere.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surface/cylinder.hpp"
#include "core/utils/utils.hpp"

using namespace geom;
using namespace math;

class SphereTest : public QObject
{
	Q_OBJECT
public:
	SphereTest();

private Q_SLOTS:
	void testSphereForward();
	void testSphereIntersection();
};

SphereTest::SphereTest()
{
}

void SphereTest::testSphereForward()
{
	//mDebug() << "eps=" << math::Vector<3>::EPS;
	Sphere s1("s1", Point{10, 0, 0}, 20);
	QVERIFY(s1.isForward(Point{3, 1, 0}) == false);
	QVERIFY(s1.isForward(Point{-100, -200, 1.5}) == true);
	QVERIFY(s1.isForward(Point{-10, 0, 0}) == true);  // 球面のジャスト直上は表扱い
	// 裏面
	Sphere s1r("-s1", Point{10, 0, 0}, 20);
	QVERIFY(s1r.isForward(Point{2, 3, 0}) == true);
	QVERIFY(s1r.isForward(Point{-100, -200, 1.5}) == false);
	QVERIFY(s1r.isForward(Point{-10, 0, 0}) == false);  // 裏球面のジャスト直上は内側扱い

}

void SphereTest::testSphereIntersection()
{
	Sphere s1("s1", Point{0, 0, 0}, 1);
	auto sect1 = s1.getIntersection(Point{-20, 20, 0}, (Vector<3>{1, -1, 0}).normalized());
	Point ex1{-0.5*std::sqrt(2), 0.5*std::sqrt(2), 0};
	for(size_t i = 0; i < ex1.size(); ++i) {
		QCOMPARE(sect1.at(i), ex1.at(i));
	}
	// 原点に直径10cmの球
	Sphere s2("s2", Point{0, 0, 0}, 10);
	Point ex2{-5*std::sqrt(2), 5*std::sqrt(2), 0};
	Point ex23{-5*std::sqrt(3), 5, 0};
	auto sect21 = s2.getIntersection(math::Point{-40, 40, 0}, (math::Vector<3>{1, -1, 0}));
	auto sect22 = s2.getIntersection(math::Point{-400, 400, 0}, (math::Vector<3>{1, -1, 0}));
	auto sect23 = s2.getIntersection(math::Point{-40, 5, 0}, (math::Vector<3>{1, 0, 0}));
	for(size_t i = 0; i < ex2.size(); ++i) {
		QCOMPARE(sect21.at(i), ex2.at(i));
		QCOMPARE(sect22.at(i), ex2.at(i));
		QCOMPARE(sect23.at(i), ex23.at(i));
	}
}



QTEST_APPLESS_MAIN(SphereTest)

#include "tst_spheretest.moc"
