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

#include <vector>

#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/utils/message.hpp"

using namespace geom;
using namespace math;

class PlaneTest : public QObject
{
	Q_OBJECT

public:
	PlaneTest();

private Q_SLOTS:
	void testForward();
	void testTransform();
	void testIntersectionDistance();
	void testIntersectionFrontBack();
	void testCase1();
	void testBB1();

};

using Vec = Vector<3>;
using BB = geom::BoundingBox;

PlaneTest::PlaneTest() {;}

void PlaneTest::testForward()
{
	// 表面から。
	Plane p1("test1", Vec{1, 0, 0}, 20);
	QVERIFY(p1.isForward(Point{25, 0, 0}));
	QVERIFY(p1.isForward(Point{25, 25, 0}));
	QVERIFY(!p1.isForward(-1*Point{25, 25, 0}));

	auto p1r = p1.createReverse();
	QVERIFY(!p1r->isForward(Point{25, 0, 0}));
	QVERIFY(!p1r->isForward(Point{25, 25, 0}));
	QVERIFY(p1r->isForward(-1*Point{25, 25, 0}));

}

void PlaneTest::testBB1()
{
	Plane p1("test1", Vec{1, 0, 0}, 5);
	BB bb1 = p1.generateBoundingBox();
	BB expect1(5, BB::MAX_EXTENT, -BB::MAX_EXTENT, BB::MAX_EXTENT, -BB::MAX_EXTENT, BB::MAX_EXTENT );
	mDebug() << "bb1===" << bb1.toInputString();
	QVERIFY(isSameBB(bb1, expect1));
	auto p1r = p1.createReverse();
	BB bb1r = p1r->generateBoundingBox();
	BB expect1r(-BB::MAX_EXTENT, 5, -BB::MAX_EXTENT, BB::MAX_EXTENT, -BB::MAX_EXTENT, BB::MAX_EXTENT );
	mDebug() << "bb1r===" << bb1r.toInputString();

	QVERIFY(isSameBB(bb1r, expect1r));

	mDebug() << "p1 ===" << p1.toString();
	mDebug() << "p1r===" << p1r->toString();

}

void PlaneTest::testTransform()
{
	Plane p1("test1", Vec{0, 0, 1}, 15);
	Matrix<3> rot{1, 0, 1,   0, 1, 0,   -1, 0, 1};
	Matrix<3>::orthonormalize(&rot);
	Vector<3> vec{15, 0, 0};
	Matrix<4> matrix = Matrix<4>::IDENTITY();
	matrix.setRotationMatrix(rot);
	matrix.setTranslationVector(vec);
	mDebug() << "matrix=" << matrix;

//	Point pt1{5, 0, 0};
//	math::affineTransform(&pt1, matrix);
//	mDebug() << "pt1=" << pt1.toString();
	mDebug() << "before plane1 =" << p1.toString();
	p1.transform(matrix);
	mDebug() << "after plane1 =" << p1.toString();

}

void PlaneTest::testIntersectionDistance()
{
	using Vec = math::Vector<3>;

	Plane p1("test1", Vec{ 1, 0, 0},  10);  // 法線 1 0 0 d=10
	Plane p2("test2", Vec{-1, 0, 0},  10);  // 法線-1 0 0 d=10
	Plane p3("test3", Vec{ 1, 0, 0}, -10);  // 法線 1 0 0 d=-10
	Plane p4("test4", Vec{-1, 0, 0}, -10);  // 法線-1 0 0 d=-10
//	mDebug() << "test1 normal=" << p1.normal() << "d=" << p1.distance();
//	mDebug() << "test2 normal=" << p2.normal() << "d=" << p2.distance();
//	mDebug() << "test3 normal=" << p3.normal() << "d=" << p3.distance();
//	mDebug() << "test4 normal=" << p4.normal() << "d=" << p4.distance();
	auto result1 = p1.getIntersection(math::Point{-999, 0, 0}, Vec{1, 0, 0});
	auto result2 = p2.getIntersection(math::Point{-999, 0, 0}, Vec{1, 0, 0});
	auto result3 = p3.getIntersection(math::Point{-999, 0, 0}, Vec{1, 0, 0});
	auto result4 = p4.getIntersection(math::Point{-999, 0, 0}, Vec{1, 0, 0});
	/*
	 * normalとdistanceについて。
	 * 面の位置はnormal*distanceに存在すると扱う。
	 * 即ち
	 * Plane(Vec{1, 0, 0}, Point{10, 0, 0})と
	 * Plane(Vec{-1, 0, 0}, Point{-10, 0, 0})は
	 * 同じ位置に存在し、法線が逆向きとなる。
	 *
	 */
	QVERIFY(math::isSamePoint(result1, math::Point{10, 0, 0}));
	QVERIFY(math::isSamePoint(result2, math::Point{-10, 0, 0}));
	QVERIFY(math::isSamePoint(result3, math::Point{-10, 0, 0}));
	QVERIFY(math::isSamePoint(result4, math::Point{10, 0, 0}));

}

void PlaneTest::testIntersectionFrontBack()
{
	using dvec = std::vector<double>;
	const double XPOS1 = 10, XPOS2 = -20;
	std::shared_ptr<Plane> p1  = Plane::createPlane("p1",  dvec{XPOS1},  math::Matrix<4>::IDENTITY(), Plane::TYPE::PX, false);
	std::shared_ptr<Plane> p1r = Plane::createPlane("-p1", dvec{XPOS1},  math::Matrix<4>::IDENTITY(), Plane::TYPE::PX, false);
	std::shared_ptr<Plane> p2  = Plane::createPlane("p2",  dvec{XPOS2}, math::Matrix<4>::IDENTITY(), Plane::TYPE::PX, false);
	std::shared_ptr<Plane> p2r = Plane::createPlane("-p2", dvec{XPOS2}, math::Matrix<4>::IDENTITY(), Plane::TYPE::PX, false);
	math::Point result1 = p1->getIntersection(math::Point{-100, 0, 0}, math::Vector<3>{ 1, 0, 0});
	math::Point result2 = p1->getIntersection(math::Point{ 100, 0, 0}, math::Vector<3>{-1, 0, 0});
	math::Point result3 = p1r->getIntersection(math::Point{-100, 0, 0}, math::Vector<3>{ 1, 0, 0});
	math::Point result4 = p1r->getIntersection(math::Point{ 100, 0, 0}, math::Vector<3>{-1, 0, 0});
	math::Point result5 = p2->getIntersection(math::Point{-100, 0, 0}, math::Vector<3>{ 1, 0, 0});
	math::Point result6 = p2->getIntersection(math::Point{ 100, 0, 0}, math::Vector<3>{-1, 0, 0});
	math::Point result7 = p2r->getIntersection(math::Point{-100, 0, 0}, math::Vector<3>{ 1, 0, 0});
	math::Point result8 = p2r->getIntersection(math::Point{ 100, 0, 0}, math::Vector<3>{-1, 0, 0});

	math::Point expect1{XPOS1, 0, 0}, expect2{XPOS2, 0, 0};
	QVERIFY(math::isSamePoint(result1, expect1));
	QVERIFY(math::isSamePoint(result2, expect1));
	QVERIFY(math::isSamePoint(result3, expect1));
	QVERIFY(math::isSamePoint(result4, expect1));
	QVERIFY(math::isSamePoint(result5, expect2));
	QVERIFY(math::isSamePoint(result6, expect2));
	QVERIFY(math::isSamePoint(result7, expect2));
	QVERIFY(math::isSamePoint(result8, expect2));
}

void PlaneTest::testCase1()
{
	using namespace std;
	using Vec = math::Vector<3>;
	Plane p1("p1", Vec{-1,  0,  0}, -10);
	Plane p2("p2", Vec{ 1,  0,  0}, -10);
	Plane p3("p3", Vec{ 0, -1,  0}, -10);
	Plane p4("p4", Vec{ 0,  1,  0}, -10);
	Plane p5("p5", Vec{ 0,  0, -1}, -10);
	Plane p6("p6", Vec{ 0,  0,  1}, -10);
	Plane px0("px0", Vec{ 1,  0, 0}, 0);
	Plane py0("py0", Vec{ 0,  1, 0}, 0);
	Plane pz0("pz0", Vec{ 0,  0, 1}, 0);

//	mDebug() << p1.toString();
//	mDebug() << p2.toString();
//	mDebug() << p3.toString();
//	mDebug() << p4.toString();
//	mDebug() << p5.toString();
//	mDebug() << p6.toString();
//	mDebug() << px0.toString();
//	mDebug() << py0.toString();
//	mDebug() << pz0.toString();

//	auto result1 = Plane::intersection(p1, py0, pz0);
//	math::Point expect1{10, 0, 0};
////	mDebug() << "result1=" << result1;
////	mDebug() << "expect1=" << expect1;
//	QVERIFY(math::isSamePoint(result1, expect1));

//	auto result2 = Plane::intersection(p2, py0, pz0);
//	math::Point expect2{-10, 0, 0};
////	mDebug() << "result2=" << result2;
////	mDebug() << "expect2=" << expect2;
//	QVERIFY(math::isSamePoint(result2, expect2));


    math::Matrix<3> rotMat{0.5*std::sqrt(2), 0.5*std::sqrt(2), 0,
                          -0.5*std::sqrt(2), 0.5*std::sqrt(2), 0,
                          0,                 0,                1};
    math::Matrix<4> matrix = math::Matrix<4>::IDENTITY();
    matrix.setRotationMatrix(rotMat);
    p1.transform(matrix);
    p2.transform(matrix);
//    mDebug() << p1.toString();
//    mDebug() << p2.toString();
//    mDebug() << py0.toString();

    auto result3 = Plane::intersection(p1, py0, pz0);
    math::Point expect3{10*std::sqrt(2.0), 0, 0};
//    mDebug() << "result3=" << result3;
//    mDebug() << "expect3=" << expect3;
    QVERIFY(math::isSamePoint(result3, expect3));

    auto result4 = Plane::intersection(p2, py0, pz0);
    math::Point expect4{-10*std::sqrt(2.0), 0, 0};
//    mDebug() << "result4=" << result4;
    QVERIFY(math::isSamePoint(result4, expect4));


}



QTEST_APPLESS_MAIN(PlaneTest)

#include "tst_planetest.moc"
