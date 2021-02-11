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
#include "core/utils/message.hpp"
#include "core/math/nmatrix.hpp"
#include "core/math/nvector.hpp"

using namespace geom;
using namespace math;

class Surface_testTest : public QObject
{
	Q_OBJECT

public:
	Surface_testTest();

private Q_SLOTS:
	void testExtractSurfaceNames();
	void testPlane();
	void testReversedPlane();
	void testMap();
};

Surface_testTest::Surface_testTest() {}

void Surface_testTest::testExtractSurfaceNames()
{
	std::string srcStr = "-S1:S2 #2  (S3.2 S999 (98)) -99.1";
	std::vector<std::string> expected{"-S1", "S2", "S3.2", "S999", "98", "-99.1"};
	auto result = Surface::extractSurfaceNames(srcStr);
	std::sort(expected.begin(), expected.end());
	std::sort(result.begin(), result.end());
	mDebug() << "src   =" << srcStr;
	mDebug() << "result=" << result;
	mDebug() << "expect=" << expected;
	QCOMPARE(result, expected);
}



void Surface_testTest::testPlane()
{
	Plane p1{"p1", Vector<3>{1, 0, 0}, 10};
	QVERIFY(p1.isForward(Point{11, 0, 0}) == true);  // xの正側の点はforwardg側
	QVERIFY(p1.isForward(Point{10, 0, 0}) == true);  // 境界直上は表側(forward側)に判定

	Plane p2{"p2", Vector<3>{0, 1, 0}, 10};
	QVERIFY(p2.isForward(Point{11, 10.5, 0}) == true); // yの正側の点はfowrad側

	Plane pz{"pz", Vector<3>{0, 0, 1}, 10};
	QVERIFY(pz.isForward(Point{0, 0, 0}) == false);
}

void Surface_testTest::testReversedPlane()
{
	// マイナス名で構築した場合、裏面扱いとなる。
	// が、今の実装ではコンストラクタで与えられた法線等は裏返さず、
	// 面の直上がforwardになるかどうか、という違いしか無い。
	Plane p1{"-p1", Vector<3>{1, 0, 0}, 10};
	QVERIFY(p1.isForward(Point{11, 0, 0}) == true);   // 裏返っていないsurfaceと同じ結果。
	QVERIFY(p1.isForward(Point{10, 0, 0}) == false);   // 裏面の境界直上は内側(backward側)に判定される
	// ウラ面のウラ面を生成しようとすると例外発生
	QVERIFY_EXCEPTION_THROWN(p1.createReverse(), std::invalid_argument);
	QVERIFY(p1.isReversed());

	Plane p2{"-p2", Vector<3>{0, 1, 0}, 10};
	QVERIFY(p2.isForward(Point{11, 10.5, 0}) == true);

	Plane pz{"-pz", Vector<3>{0, 0, 1}, 10};
	QVERIFY(pz.isForward(Point{0, 0, 0}) == false);
}


void Surface_testTest::testMap()
{
	mDebug() << "########## Map test";

	auto s2  = std::make_shared<Sphere>("s2", Point{0, 10, 0}, 20);
	auto s3  = std::make_shared<Sphere>("s3", Point{0, 10, 0}, 20);
	auto s3m = std::make_shared<Sphere>("-s3", Point{0, 10, 0}, 20);
	s3m->setID(-s3->getID());
	auto s4   = std::make_shared<Sphere>("s4", Point{10,10, 0}, 15.0);
	auto px1  = std::make_shared<Plane>("px1", Vector<3>{1.0, 0.0, 0.0}, 10.0);
	auto px1m = std::make_shared<Plane>("-px1", Vector<3>{1.0, 0.0, 0.0}, 10.0);
	px1m->setID(-px1->getID());

	Surface::map_type map;
	map.registerSurface(s2->getID(), s2);
	map.registerSurface(s3->getID(), s3);
	map.registerSurface(s3m->getID(), s3m);
	map.registerSurface(s4->getID(), s4);
	map.registerSurface(px1->getID(), px1);
	map.registerSurface(px1m->getID(), px1m);


	QVERIFY_EXCEPTION_THROWN(map.at(100), std::out_of_range);
	QVERIFY_EXCEPTION_THROWN(map.at(-100), std::out_of_range);
	QVERIFY_EXCEPTION_THROWN(map.at(0), std::out_of_range);

	QVERIFY(map.at(s3m->getID())->toString().find(Surface::reversedChars()) != std::string::npos);
	QVERIFY(map.at(px1m->getID())->toString().find(Surface::reversedChars()) != std::string::npos);

}



QTEST_APPLESS_MAIN(Surface_testTest)

#include "tst_surfacetest.moc"
