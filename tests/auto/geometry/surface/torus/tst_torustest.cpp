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
#include <QMetaObject>

#include <cmath>
#include <memory>


#include "core/geometry/surface/torus.hpp"
#include "core/utils/message.hpp"
#include "core/utils/numeric_utils.hpp"
#include "core/utils/matrix_utils.hpp"

using namespace geom;
using namespace math;
using Pt = math::Point;

typedef math::Vector<3> Vec;
typedef math::Matrix<4> Mat;

Q_DECLARE_METATYPE(Point);
Q_DECLARE_METATYPE(std::vector<double>);





class TorusTest : public QObject
{
	Q_OBJECT

public:
	TorusTest();

private Q_SLOTS:
	void testTrTorusIntersection3();
	void testTrTorusIntersection1();
	void testTrTorusIntersection0();
	void testTrTorusIntersection2();
	void testTrTorusIntersection();
	void testTorusAtOrigin();
	void testTransformedTorus();
	void testEllipticTorus1();
	void testEllipticTorus2();
	//void testNormal1();
	void testIntersection1_data();
	void testIntersection1();
	void testIntersection0();
	void testTX();
	void testTZ();
	void testTrTx();
	void testTrTz();

};
#define COMPAREPOINTS(arg1, arg2, prec) \
	QVERIFY(std::abs(arg1.x() - arg2.x()) < prec); \
	QVERIFY(std::abs(arg1.y() - arg2.y()) < prec); \
	QVERIFY(std::abs(arg1.z() - arg2.z()) < prec);

#define PREC math::EPS*10000

TorusTest::TorusTest() {;}



void TorusTest::testTrTorusIntersection3()
{
	//
	Torus t1("t1", Pt{25, 0, 0}, math::Vector<3>{0, 1, 0}, 10, 3, 3);
	auto matrix = utils::generateSingleTransformMatrix("0 0 0  -1 3e-6 0  -3e-6 -1 0  0 0 1", false); // これをテスト
	t1.transform(matrix);

//	for(int i = -20; i <21; ++i) {
//		mDebug() << "y=" << i*0.5 << t1.getIntersection(Pt{-50, i*0.5, 0}, Vec{1, 0, 0});
//	}

	auto result1 = t1.getIntersection(Pt{-100, 2, 0}, Vec{1, 0, 0});
	auto expect1 = Pt{-35-std::sqrt(5.0), 2, 0};
	mDebug() << "result1=" << result1;
	mDebug() << "expect1=" << expect1;
	mDebug() << "diff=" << result1 - expect1;
	COMPAREPOINTS(result1, expect1, 1e-4);
}

void TorusTest::testTrTorusIntersection1()
{

	Torus t1("t1", Pt{10, 0, 0}, math::Vector<3>{0, 1, 0}, 10, 3, 3);
	auto matrix = utils::generateSingleTransformMatrix("0 0 0    5e-8 1 0  -1 5e-8 0    0 0 1", false); // これをテスト
	//auto matrix = utils::generateTransformMatrix("0 0 0  0 1 0  -1 0 0    0 0 1", false); // これがテストを通らなければ問題外
	t1.transform(matrix);
	// これで軸方向がx方向、z断面でトーラスの片方が原点中心にくる。
	mDebug() << "torusinfo=" << t1.toInputString();

//	for(size_t i = 0; i <50; ++i) {
//		mDebug() << "i=y=" << i<< t1.getIntersection(Pt{0, i, 0}, Vec{1, 0, 0});
//	}

	//y=12以下では交点を持たないのが正しい
	auto result0 = t1.getIntersection(Pt{-100, 11.375, 0}, Vec{1, 0, 0});
	COMPAREPOINTS(result0, Pt::INVALID_VECTOR(), 1e-5);

	auto result0x = t1.getIntersection(Pt{0.125, 17.997394, 0}, Vec{0, 1, 0});
	mDebug() << "result0x=" << result0x;


	auto result1 = t1.getIntersection(Pt{-100, 0, 0}, Vec{1, 0, 0});
	auto expect1 = Pt{-3, 0, 0};
	mDebug() << "result1=" << result1;
	mDebug() << "expect1=" << expect1;
	mDebug() << "diff=" << result1 - expect1;
	COMPAREPOINTS(result1, expect1, 1e-5);
}

void TorusTest::testTrTorusIntersection0()
{
	Torus t1("t1", Pt{0, 0, 0}, math::Vector<3>{0, 0, 1}, 10, 3, 3);
	//auto matrix = utils::generateTransformMatrix("0 0 0  1 5e-8 0  -5e-8 1 0    0 0 1", false); // これもまあOK
	//auto matrix = utils::generateTransformMatrix("0 0 0  1 0 0  0 1 0    0 0 1", false);  // これは問題なくテストを通る
	auto matrix = utils::generateSingleTransformMatrix("0 0 0  0 1 0  -1 0 0    0 0 1", false);

	mDebug() << "trusinfo=" << t1.toInputString();
	mDebug() << "trusinfo=" << t1.toString();
	t1.transform(matrix);
	mDebug() << "trusinfo=" << t1.toString();
	COMPAREPOINTS(t1.getIntersection(Pt{0, 0, -50}, Pt{0, 0, 1}), Pt::INVALID_VECTOR(), PREC);

	mDebug() << "-xから中心への交点result=" <<t1.getIntersection(Pt{-50, 0, 0}, Pt{1, 0, 0});
	mDebug() << "-yから中心への交点result=" <<t1.getIntersection(Pt{0, -50, 0}, Pt{0, 1, 0});
}


void TorusTest::testTrTorusIntersection2()
{
	// 方向ベクトルにわずかに斜め成分が入るとバグる→これは4次式の解の精度の問題
	Torus t1("t1", Pt{0, 0, 0}, math::Vector<3>{0, 0, 1}, 10, 3, 3);
	// 軸方向からの交点
//	auto result1 = t1.getIntersection(Pt{0, 10, -10}, Vec{0, 0, 1});
	auto result = t1.getIntersection(Pt{0, 10, -10}, Vec{0, 0.0000000001, 1});
	auto expect = Pt{0, 10, -3};
	mDebug() << "result=" << result;
	mDebug() << "expect=" << expect;
	auto diff = result - expect;
	mDebug() << "result-expect=" << diff;
	COMPAREPOINTS(expect, result, PREC);


	Torus t2("t2", Pt{0, 25, 0}, math::Vector<3>{1, 0, 0}, 10, 3, 3);
	auto matrix = utils::generateSingleTransformMatrix("0 0 0  1 5e-9 0  -5e-9 1 0    0 0 1", false);
//	auto matrix = utils::generateTransformMatrix("0 0 0  1 0    0   0    1 0    0 0 1", false);  // これは問題なくテストを通る
	t2.transform(matrix);
	mDebug() << "Torus inputstring=" << t2.toInputString();
	auto resultPoint = t2.getIntersection(Pt{-50, 0, 0}, Pt{1, 0, 0});
	COMPAREPOINTS(resultPoint, Pt::INVALID_VECTOR(), PREC);

	// 径方向からの交点はそれなりに合う
	auto result0 = t2.getIntersection(Pt{0, -5, 0}, Pt{0, 1, 0});
	auto expect0 = Pt{0, 12, 0};
	mDebug() << "result0=" << result0;
	mDebug() << "expect0=" << expect0;
	COMPAREPOINTS(expect0, result0, PREC);

//	for(size_t i = 0; i < 50; ++i) {
//		auto y = i*0.5;
//		mDebug() << "i=" << i << "y=" << std::setw(5) << std::setprecision(4) << y <<"での交点は" <<  t1.getIntersection(Pt{-10, y, 0}, Pt{1, 0, 0});
//	}

	// 軸方向からの交点
	auto result1 = t2.getIntersection(Pt{-10, 35, 0}, Pt{1, 0, 0});
	auto expect1 = Pt{-3, 35, 0};
	mDebug() << "result1=" << result1;
	mDebug() << "expect1=" << expect1;
	mDebug() << "result1 - expect1=" << result1 - expect1;
	COMPAREPOINTS(expect1, result1, 1e-6);
}




void TorusTest::testTrTorusIntersection()
{
	/*
	 * Y軸平行トーラスが (25, 0, 0)にあり、それを90度-α回転させ
	 * X軸平行トーラスが(0, 25, 0)にある場合のテスト
	 */
	using Pt = math::Point;
	Torus t1("t1", Pt{25, 0, 0}, math::Vector<3>{0, 1, 0}, 10, 3, 3);
//	auto matrix = utils::generateSingleTransformMatrix("0 0 0  0 1 0  -1 0 0  0 0 1", false);
	auto matrix = utils::generateSingleTransformMatrix("0 0 0  5e-9 1 0  -1 5e-9 0  0 0 1", false);
//	auto matrix = utils::generateSingleTransformMatrix("0 0 0  1e-7 1 0  -1 0 0  0 0 1", false);  // これは↑と等価なので試す意味なし
	t1.transform(matrix);
	COMPAREPOINTS(t1.getIntersection(Pt{-50, 0, 0}, Pt{1, 0, 0}), Pt::INVALID_VECTOR(), PREC);

	auto result0 = t1.getIntersection(Pt{0, -100, 0}, Pt{0, 1, 0});
	auto expect0 = Pt{0, 12, 0};
	mDebug() << "result0=" << result0;
	mDebug() << "expect0=" << expect0;
	COMPAREPOINTS(expect0, result0, PREC);

	auto result1 = t1.getIntersection(Pt{-100, 35, 0}, Pt{1, 0, 0});
	auto expect1 = Pt{-3, 35, 0};
	mDebug() << "result1=" << result1;
	mDebug() << "expect1=" << expect1;
	mDebug() << "result1 - expect1=" << result1 - expect1;
	COMPAREPOINTS(expect1, result1, 1e-5);  // 90-α回転なので少しずれる分緩めの比較にする。
}




void TorusTest::testTorusAtOrigin()
{
	// 原点中心に大半径10cm、小半径1cmの円形断面トーラス
	double R=10, r=1;
	auto tor1 = std::shared_ptr<const Torus>(new Torus("torus1", Point{0, 0, 0}, math::Vector<3>{0, 0, 1}, R, r, r));
	for(int i = -20; i < 1; ++i) {
		auto pos = Point{static_cast<double>(i), 0, 0};
		tor1->isForward(pos);
		//mDebug() << "i=" << i << "pos=" << pos << "isForward=" << tor1->isForward(pos);
		if(R-r < (double)std::abs(i) && (double)std::abs(i) < R+r) {
			QVERIFY(!tor1->isForward(pos));
		} else {
			QVERIFY(tor1->isForward(pos));
		}
	}

	//mDebug() << "surface=" << tor1->toString();
	Point isect, ex;
	// -x方向からの交点
	isect = tor1->getIntersection(Point{-100, 0, 0}, Vector<3>{1, 0, 0});
	ex = Point{-R-r, 0, 0};
//	mDebug() << "result=" << isect;
//	mDebug() << "expect=" << ex;
	COMPAREPOINTS(isect, ex, PREC);
	// トーラス内からの交点
	isect = tor1->getIntersection(Point{-R*1.01, 0, 0}, Vector<3>{1, 0, 0});
	ex = Point{-R+r, 0, 0};
	COMPAREPOINTS(isect, ex, PREC);
	// ドーナッツ空洞からの交点
	isect = tor1->getIntersection(Point{-0.4*R, 0, 0}, Vector<3>{1, 0, 0});
	ex = Point{R-r, 0, 0};
	COMPAREPOINTS(isect, ex, PREC);
	// +x方向からの交点
	isect = tor1->getIntersection(Point{100, 0, 0}, Vector<3>{-1, 0, 0});
	ex = Point{R+r, 0, 0};
	COMPAREPOINTS(isect, ex, PREC);

	// +y方向からの交点
	isect = tor1->getIntersection(Point{0, 100, 0}, Vector<3>{0, -1, 0});
	ex = Point{0, R+r, 0};
//	mDebug() << "result=" << isect;
//	mDebug() << "expect=" << ex;
//	mDebug() << "diff=" << isect-ex;
	COMPAREPOINTS(isect, ex, PREC);
	// ドーナッツ内
	isect = tor1->getIntersection(Point{0, R*0.99, 0}, Vector<3>{0, -1, 0});
	ex = Point{0, R-r, 0};
	COMPAREPOINTS(isect, ex, PREC);
	// +y方向から外側は交点なし
	isect = tor1->getIntersection(Point{0, 100, 0}, Vector<3>{0, 1, 0});
	ex = Point::INVALID_VECTOR();
	COMPAREPOINTS(isect, ex, PREC);

	// +z方向からの交点
	double tmp = 0.5*R*std::sqrt(2);
	isect = tor1->getIntersection(Point{tmp, tmp, 100}, Vector<3>{0, 0, -1});
	ex = Point{tmp, tmp, r};
	COMPAREPOINTS(isect, ex, PREC);
}


void TorusTest::testTransformedTorus()
{
	// (5, 6, 10)中心に、軸方向z、大半径10、小半径0.4の円形断面トーラス
	double R=10, r=0.4;
	Point center{5, 6, 10};
	auto tor1 = std::shared_ptr<const Torus>(new Torus("torus2", center, math::Vector<3>{0, 0, 1}, R, r, r));
	for(int i = -20 + center.x(); i < 1 + center.x(); ++i) {
		double xpos = static_cast<double>(i) + center.x();
		auto pos = Point{xpos, center.y()+0.1, center.z()+0.1};
		//mDebug() << "pos=" << pos << "isForward=" << tor1->isForward(pos);
		if((R+center.x()-r < xpos && xpos < R+center.x()+r)
		|| (-(R-center.x()+r) < xpos && xpos < -(R-center.x()-r))) {
			QVERIFY(!tor1->isForward(pos));
		} else {
			QVERIFY(tor1->isForward(pos));
		}
	}
}

void TorusTest::testEllipticTorus1()
{
	// (5, 6, 10)中心に、軸方向z、大半径10、小半径0.4の円形断面トーラス
	double R = 10, a = 2, b = 0.6;
//	Point center{5, 6, 10};
	Point center{5, 6, 100};
	auto tor1 = std::shared_ptr<const Torus>(new Torus("torus2", center, math::Vector<3>{0, 0, 1}, R, a, b));
	for(int i = -20 + center.x(); i < 21 + center.x(); ++i) {
		double xpos = static_cast<double>(i) + center.x();
		auto pos = Point{xpos, center.y()+0.1, center.z()+0.1};
		tor1->isForward(pos);
//		mDebug() << "i=" << i << "pos=" << pos << "isForward=" << tor1->isForward(pos);
//		mDebug() << "range=(" << (R+center.x()-r) << "," << (R+center.x()+r) << "),("
//				 << -(R-center.x()-r) << "," << -(R-center.x()+r) << ")";
		if((R+center.x()-b < xpos && xpos < R+center.x()+b)
		|| (-(R-center.x()+b) < xpos && xpos < -(R-center.x()-b))) {
			QVERIFY(!tor1->isForward(pos));
		} else {
			QVERIFY(tor1->isForward(pos));
		}
	}
}

void TorusTest::testEllipticTorus2()
{
	// (5, 6, 10)中心に、軸方向z、大半径10、小半径0.6, 2の楕円断面トーラス
	double R = 10, a = 0.6, b = 2;
//	Point center{5, 6, 10};
	Point center{5, 6, 100};
	auto tor1 = std::shared_ptr<const Torus>(new Torus("torus2", center, math::Vector<3>{0, 0, 1}, R, a, b));
	for(int i = -20 + center.x(); i < 21 + center.x(); ++i) {
		double xpos = static_cast<double>(i) + center.x();
		auto pos = Point{xpos, center.y()+0.1, center.z()+0.1};
		tor1->isForward(pos);
//		mDebug() << "i=" << i << "pos=" << pos << "isForward=" << tor1->isForward(pos);
//		mDebug() << "range=(" << (R+center.x()-r) << "," << (R+center.x()+r) << "),("
//				 << -(R-center.x()-r) << "," << -(R-center.x()+r) << ")";
		if((R+center.x()-b < xpos && xpos < R+center.x()+b)
		|| (-(R-center.x()+b) < xpos && xpos < -(R-center.x()-b))) {
			QVERIFY(!tor1->isForward(pos));
		} else {
			QVERIFY(tor1->isForward(pos));
		}
	}
}





//void TorusTest::testNormal1()
//{
//	// 原点中心に大半径10cm、の円形断面トーラス
//	double R=10, r=1;
//	auto tor1 = std::unique_ptr<const Torus>(new Torus("torus1", Point{0, 0, 0}, math::Vector<3>{0, 0, 1}, R, r, r));
//	auto ex1 = Point{1, 0, 0};
//	auto re1 = tor1->normal(Point{R+r, 0, 0});
//	QCOMPARE(re1, ex1);
//	auto ex2 = Vector<3>{-1, 0, 0};
//	auto re2 = tor1->normal(Point{R-r, 0, 0});
//	QCOMPARE(re2, ex2);
//	QCOMPARE(tor1->normal(Point{-(R+r), 0, 0}), ex2);
//	QCOMPARE(tor1->normal(Point{-(R-r), 0, 0}), ex1);
//	// Z方向
//	auto ex3 = Vector<3>{0, 0, 1};
//	auto re3 = tor1->normal(Point{R, 0, r});
//	QCOMPARE(re3, ex3);
//	QCOMPARE(tor1->normal(Point{R, 0, -r}), -ex3);
//	QCOMPARE(tor1->normal(Point{-R, 0, -r}), -ex3);
//	QCOMPARE(tor1->normal(Point{-R, 0, r}), ex3);
//	// Y方向
//	auto ex4 = Vector<3>{0, 1, 0};
//	auto re4 = tor1->normal(Point{0, R+r, 0});
//	QCOMPARE(re4, ex4);
//	QCOMPARE(tor1->normal(Point{0, R-r, 0}), -ex4);
//	QCOMPARE(tor1->normal(Point{0, -(R+r), 0}), -ex4);
//	QCOMPARE(tor1->normal(Point{0, -(R-r), 0}), ex4);

//	//(2/sqrt2, 0, 2/sqrt2)になる場合
//	auto pos5 =Point{R+r*0.5*std::sqrt(2), 0, r*0.5*std::sqrt(2)};
//	auto ex5 = Vector<3>{0.5*std::sqrt(2), 0, 0.5*std::sqrt(2)};
//	auto re5 = tor1->normal(pos5);
//	mDebug() << "pos=" << pos5;
//	mDebug() << "ex=" << ex5;
//	mDebug() << "re=" << re5;
//	QCOMPARE(re5, ex5);

//	// OK
//	auto ex6 = Vector<3>{0.5*std::sqrt(2), 0.5*std::sqrt(2), 0};
//	auto re6 = tor1->normal(Point{(R+r)*0.5*std::sqrt(2), (R+r)*0.5*std::sqrt(2), 0});
//	mDebug() << "ex=" << ex6;
//	mDebug() << "re=" << re6;
//	QCOMPARE(re6, ex6);
//}




void TorusTest::testIntersection1_data()
{
	double Cx=5, Cy=6, Cz=10;
//	double Cx=0, Cy=0, Cz=0.5;
	double R=10, a=2, b=1;
	std::vector<double>param{R, a, b};
	Point c{Cx, Cy, Cz};
	Point ax{0, 0, 1};
	QTest::addColumn<Point>("center");
	QTest::addColumn<Point>("axis");
	QTest::addColumn<std::vector<double>>("params");
	QTest::addColumn<Point>("point");
	QTest::addColumn<Point>("direction");
	QTest::addColumn<Point>("expected");

	QTest::newRow("from x-infty to x+ dir")<<c<<ax<<param<< c+Point{-100,0,0} << Point{1,0,0} << c+Point{-R-b, 0, 0};
	QTest::newRow("from inside1 to x+ dir")<<c<<ax<<param<< c+Point{-R,0,0} << Point{1,0,0} << c+Point{-R+b, 0, 0};
	QTest::newRow("from inside1 to x- dir")<<c<<ax<<param<< c+Point{-R,0,0} << Point{-1,0,0} << c+Point{-R-b, 0, 0};
	QTest::newRow("from inside2 to x+ dir")<<c<<ax<<param<< c+Point{R,0,0} << Point{1,0,0} << c+Point{R+b, 0, 0};

	double rxy = 0.5*R*std::sqrt(2);
	Point zm{0, 0, -1};
	QTest::newRow("from above1 to z- dir")<<c<<ax<<param<< Point{c.x()+R, c.y(), 100} << zm << Point{c.x()+R, c.y(), c.z()+a};
	QTest::newRow("from above2 to z- dir")<<c<<ax<<param<< Point{c.x()+rxy,c.y()+rxy,100} << zm << Point{c.x()+rxy, c.y()+rxy, c.z()+a};
}

void TorusTest::testIntersection1()
{
	QFETCH(Point, center);
	QFETCH(Point, axis);
	QFETCH(std::vector<double>, params);
	QFETCH(Point, point);
	QFETCH(Point, direction);
	QFETCH(Point, expected);
	Torus tor("torus", center, axis, params.at(0), params.at(1), params.at(2));
//	mDebug() << "from=" << point << ", dir=" << direction;
	auto is = tor.getIntersection(point, direction);
//	mDebug() << "isect =" << is;
//	mDebug() << "expect=" << expected;
	COMPAREPOINTS(is, expected, PREC);
}

void TorusTest::testIntersection0()
{
	double Cx=6, Cy=5, Cz=6;
	double R=10, a=2, b=1;

	auto center = Point{Cx, Cy, Cz};
	Torus tor("torus", center, Vector<3>{0, 0, 1}, R, a, b);
	//mDebug() << "surface=" << tor.toString();
	double tmp = R*0.5*std::sqrt(2);
	double Sx=Cx+tmp, Sy=Cy+tmp, Sz=100;
//	double Sx=Cx+R, Sy=Cy, Sz=100;
	auto from = Point{Sx, Sy, Sz};

	//mDebug() << "点" << from << "から" << Vector<3>{0, 0, -1} << "方向の交点を求める";
	auto is = tor.getIntersection(from, Vector<3>{0, 0, -1});
	auto ex = Point{from.x(), from.y(), Cz+a};
//	mDebug() << "from=" << from << "dir= " << Vector<3>{0, 0, 1};
//	mDebug() << "result=" << is;
//	mDebug() << "expect=" << ex;
	QVERIFY(std::abs(is.x() - ex.x()) < 1e-10);
	QVERIFY(std::abs(is.y() - ex.y()) < 1e-10);
	QVERIFY(std::abs(is.z() - ex.z()) < 1e-10);
}

void TorusTest::testTZ()
{
	double R=10, a=2, b=1;
	double x=5, y=6, z=7;
	std::vector<double> params{x, y, z, R, a, b};

	auto tz = Torus::createTorus("z-torus", params, Mat(), Torus::TYPE::TZ, false);
//	mDebug() << tz->toString();
//	mDebug() << tz->toInputString();
	Vector<3> az{0, 0, 1};
	Point ctz{x, y, z};
//	mDebug() << "realAxis=" << tz->axis();
//	mDebug() << "realCenter=" << tz->center();
	QVERIFY(math::isSamePoint(tz->axis(), az));
	QVERIFY(math::isSamePoint(tz->center(), ctz));
	// +z方向からの交点
	auto from = Point{x+R, y, z+100};
	auto dir = Vector<3>{0, 0, -1};
	auto is = tz->getIntersection(from, dir);
	auto ex = Point{from.x(), from.y(), z+a};
//	mDebug() << "from=" << from << ", dir=" << dir;
//	mDebug() << "result=" << is;
//	mDebug() << "expect=" << ex;
	QVERIFY(math::isSamePoint(is, ex));
}


void TorusTest::testTX()
{
	double R=10, a=2, b=1;
	double x=1, y=0, z=0;
	std::vector<double> params{x, y, z, R, a, b};
	auto tx = Torus::createTorus("tor1", params, Mat(), Torus::TYPE::TX, false);
//	mDebug() << tx->toString();
//	mDebug() << tx->toInputString();
	Vector<3> ax{1, 0, 0};
	Point ctx{x, y, z};
//	mDebug() << "realAxis=" << tx->axis() << "expect=" << ax;
//	mDebug() << "realCenter=" << tx->center();
	QVERIFY(math::isSamePoint(tx->axis(), ax));
	QVERIFY(math::isSamePoint(tx->center(), ctx));

	Point from, dir, is, ex;
	// +y方向から(径方向)
	from = Point{x, y+100, z,};
	dir = Vector<3>{0, -1, 0};
	is = tx->getIntersection(from, dir);
	ex = Point{x, y+R+b, z};
//	mDebug() << "from=" << from << ", dir=" << dir;
//	mDebug() << "result=" << is;
//	mDebug() << "expect=" << ex;
	QVERIFY(math::isSamePoint(is, ex));

	// -x方向(軸方向)からの交点
	from = Point{x-100, y, z+R};
	dir = Vector<3>{1, 0, 0};
	is = tx->getIntersection(from, dir);
	ex = Point{x-a, y, z+R};
	QVERIFY(math::isSamePoint(is, ex));

	//  ドーナッツ内側から+Z方向(径方向)の交点
	from = Point{x, y, z,};
	dir = Vector<3>{0, 0, 1};
	is = tx->getIntersection(from, dir);
	ex = Point{x, y, z+R-b};
//	mDebug() << "from=" << from << ", dir=" << dir;
//	mDebug() << "result=" << is;
//	mDebug() << "expect=" << ex;
	QVERIFY(math::isSamePoint(is, ex));
}

void TorusTest::testTrTx()
{
	double x=0, y=0, z=0;
	double r = 2;
	double R=5, a=r, b=r;

	Mat mat{1, 1, 0, 0,  -1, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1};
//	Mat mat{1, 0, 0, 0,   0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1};
	math::Matrix<4>::orthonormalize(&mat);
//	mDebug() << "mat=" << mat;
	std::vector<double> params{x, y, z, R, a, b};
	auto tx = Torus::createTorus("tor1", params, mat, Torus::TYPE::TX, false);
	auto txr = tx->createReverse();
//	mDebug() << tx->toString();
//	mDebug() << txr->toString();

	Vector<3> ax{1, 0, 0};
	ax = ax*mat.rotationMatrix();
	Point ctx{x, y, z};
//	mDebug() << "ctx=" << ctx;
	math::affineTransform(&ctx, mat);
//	mDebug() << "ctx after=" << ctx;
//	mDebug() << "realAxis=" << tx->axis();
//	mDebug() << "realCenter=" << tx->center();
	QVERIFY(math::isSamePoint(tx->axis(), ax));
	QVERIFY(math::isSamePoint(tx->center(), ctx));
	QVERIFY(math::isSamePoint(dynamic_cast<Torus*>(txr.get())->axis(), tx->axis()));
	QVERIFY(math::isSamePoint(dynamic_cast<Torus*>(txr.get())->center(), tx->center()));

	Point from, dir, is, ex;
	from = Point{-10, 0.1, 5};
	dir = Point{1, 0, 0};
	is = tx->getIntersection(from, dir);
//	mDebug() << "is===============" << is;

//	from = Point{-100, 0.5*R*std::sqrt(2), 0};
//	dir = Point{1, 0, 0};
//	is = tx->getIntersection(from, dir);
//	ex = Point{-0.5*R*std::sqrt(2)-r,from.y(), from.z()};
//	COMPAREPOINTS(is, ex);
//	for(int i = -100; i < 1; ++i) {
//		auto pos = Point{0.1*i, 0.5*R*std::sqrt(2), 0};
//		mDebug() << "i=" << i << "pos=" << pos << txr->isForward(pos);
//	}
}


// Tzをz軸回りに回転させた時の問題は数値誤差のみだった。
void TorusTest::testTrTz()
{
	double R=10, a=2, b=1;
	double x=0, y=0, z=0;
	// xy平面で45度回転
	Mat mat{1, 1, 0, 0,  -1, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1};
//	Mat mat{1, 0, 0, 0,   0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1};
	math::Matrix<4>::orthonormalize(&mat);
//	mDebug() << "mat=" << mat;
	std::vector<double> params{x, y, z, R, a, b};
	auto tz = Torus::createTorus("tor1", params, mat, Torus::TYPE::TZ, false);
//	mDebug() << tz->toString();
	Vector<3> az{0, 0, 1};
	az = az*mat.rotationMatrix();
	Point ctz{x, y, z};
	math::affineTransform(&ctz, mat);
//	mDebug() << "realAxis=" << tz->axis();
//	mDebug() << "realCenter=" << tz->center();
	QVERIFY(math::isSamePoint(tz->axis(), az));
	QVERIFY(math::isSamePoint(tz->center(), ctz));
	// -x交点
	Point from, dir, is, ex;
	from = Point{-100, ctz.y(), ctz.z()};
	dir = Vector<3>{1, 0, 0};
	ex = Point{ctz.x()-R-b, from.y(), from.z()};
	is = tz->getIntersection(from, dir);
//	mDebug() << "is=" << is;
//	mDebug() << "ex=" << ex;
//	mDebug() << "diff=" << is - ex;
	COMPAREPOINTS(is, ex, PREC);
	// 軸方向
	from = Point{ctz.x(), ctz.y()+R, 100};
	dir = Vector<3>{0, 0, -1};
	ex = Point{from.x(), from.y(), ctz.z()+a};
	is = tz->getIntersection(from, dir);
	QVERIFY(math::isSamePoint(is, ex));
}


QTEST_APPLESS_MAIN(TorusTest)

#include "tst_torustest.moc"
