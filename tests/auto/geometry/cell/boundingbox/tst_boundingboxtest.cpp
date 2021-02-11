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

#include <memory>
#include "core/geometry/surface/cone.hpp"
#include "core/geometry/surface/cylinder.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surface/sphere.hpp"
#include "core/geometry/surface/torus.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/utils/message.hpp"
#include "core/utils/numeric_utils.hpp"
using namespace geom;
using Pt = math::Point;
using Vec = math::Vector<3>;




class BoundingboxTest : public QObject
{
	Q_OBJECT

public:
	BoundingboxTest();

private Q_SLOTS:
    void testFromPlanes();
    void testTorus();
    void testSphere();
    void testCylinder();
    void testCone();
    void testPlane();
private:
	std::atomic_bool timeoutFlag;
};

#define MDEBUG(arg) mDebug() << #arg"=" << arg;
#define VERIFYRESULTS(arg1, arg2)  \
	for(size_t i = 0; i < arg1.size(); ++i) {QVERIFY(utils::isSameDouble(arg1.at(i), arg2.at(i)));}


BoundingboxTest::BoundingboxTest() { timeoutFlag.store(false);}

void BoundingboxTest::testFromPlanes()
{
    std::vector<Plane> pVec {
        Plane("p0", Vec{-0.4472,  -0.8944,  -0},  12),
        Plane("p1", Vec{ 0.8944,  -0.4472,   0},  10),
        Plane("p2", Vec{-0.8944,   0.4472,   0}, -14),
        Plane("p3", Vec{ 0,     0,   1},  10),
        Plane("p4", Vec{ 0,     0,  -1},  -14)
    };

    auto bb = BoundingBox::fromPlanes(nullptr, std::vector<std::vector<Plane>>{pVec});
    QVERIFY(bb.isUniversal(true));
}

void BoundingboxTest::testTorus()
{
    double XPOS = 10;
    Pt center{XPOS, 0, 0};
    Vec axis{1, 0, 0};
    double R = 10, a=1, b=2;
    Torus tor1("testTorus", center, axis, R, a, b);
    auto torR = tor1.createReverse();
    auto planeVectors = torR->boundingPlaneVectors();

    auto bb = geom::BoundingBox::fromPlanes(&timeoutFlag, planeVectors);
    auto result1 = bb.range();
    std::array<double, 6> expects{-a + XPOS, a + XPOS, -R-b, R+b, -R-b, R+b};
    for(size_t i =0; i < expects.size(); ++i) {
        //mDebug() << "i===" << i << result1.at(i) << expects.at(i);
        QVERIFY(utils::isSameDouble(expects.at(i), result1.at(i)));
    }
}



void BoundingboxTest::testSphere()
{
    const math::Point center{10, 20, -30};
    const double rad = 15;
    Sphere sp("sp1", center, rad);
    auto spr = sp.createReverse();
    auto planeVectors = spr->boundingPlaneVectors();
    auto bb = BoundingBox::fromPlanes(&timeoutFlag, planeVectors);
    auto result1 = bb.range();
    decltype(result1) expect1{-5, 25, 5, 35, -45, -15};

    VERIFYRESULTS(result1, expect1)
}

void BoundingboxTest::testCylinder()
{
    const double rad = std::sqrt(5);
    Cylinder cyl1("cyl1", Pt{2, 2, 0}, Vec{2, 1, 0}.normalized(), rad);
    auto cylr1 = cyl1.createReverse();
    const double X0 = -1, X1 = 5;
    Plane px0("px0", Vec{1, 0, 0}, X0);
    Plane px1("px1", Vec{-1, 0, 0}, -X1);
    auto planeVecs = cylr1->boundingPlaneVectors();
    planeVecs = BoundingBox::mergePlaneVectorsAnd(&timeoutFlag, planeVecs, px0.boundingPlanes());
    planeVecs = BoundingBox::mergePlaneVectorsAnd(&timeoutFlag, planeVecs, px1.boundingPlanes());
    auto bb = BoundingBox::fromPlanes(&timeoutFlag, planeVecs);
    auto result1 = bb.range();
    decltype(result1) expect1{X0, X1, -2, 6, -std::sqrt(5), std::sqrt(5)};
//    mDebug() << "result1=" << result1;
//    mDebug() << "expect1=" << expect1;
    for(size_t i = 0; i < result1.size(); ++i) {
//        mDebug() << "diff=" << result1.at(i) - expect1.at(i);
        QVERIFY(std::abs(result1.at(i) - expect1.at(i)) < 1e-6);
    }
}



void BoundingboxTest::testCone()
{
    double RAD = std::sqrt(3.0)/3.0;
    int POSITIVE=2, NEGATIVE=-5;
    // surface名、 頂点、 頂点から底面方向への方向ベクトル、頂点から単位長さ動いた位置での半径、 正負どちらのシートか
    const double CONEVERTEX_Y = 5, PLANE_Y1 = 20, PLANE_Y2 = -20;
    // 単一シートからのBB作成
    Cone cone1("cone1", Pt{0, CONEVERTEX_Y, 0}, Vec{0, 1, 0}, RAD, POSITIVE);
    auto cone1r = cone1.createReverse();
    auto conePlaneVectors = cone1r->boundingPlaneVectors();

    Plane plane1("p1", math::Vector<3>{0, -1, 0}, -PLANE_Y1);
//    mDebug() << "plane=" << plane1.toString();
    auto planePlaneVectors = plane1.boundingPlanes();

    auto resultPlanes = BoundingBox::mergePlaneVectorsAnd(&timeoutFlag, conePlaneVectors, planePlaneVectors);
    auto bb = BoundingBox::fromPlanes(&timeoutFlag, resultPlanes);
    auto result1 = bb.range();
    const double val1 = RAD*(PLANE_Y1 - CONEVERTEX_Y);
    decltype(result1) expect1{-val1, val1, CONEVERTEX_Y, PLANE_Y1, -val1, val1};
//	mDebug() << "result1=" << result1;
//	mDebug() << "expect1=" << expect1;
    for(size_t i = 0; i < result1.size(); ++i) {
        //mDebug() << "diff=" << result1.at(i) - expect1.at(i);
        QVERIFY(std::abs(result1.at(i) - expect1.at(i)) < 1e-6);
    }

    Cone cone2("cone2", Pt{0, CONEVERTEX_Y, 0}, Vec{0, 1, 0}, RAD, NEGATIVE);
    auto cone2r = cone2.createReverse();
    Plane plane2("", math::Vector<3>{0, 1, 0}, PLANE_Y2);
    resultPlanes = BoundingBox::mergePlaneVectorsAnd(&timeoutFlag, cone2r->boundingPlaneVectors(), plane2.boundingPlaneVectors());
    bb = BoundingBox::fromPlanes(&timeoutFlag, resultPlanes);
    auto result2 = bb.range();
    const double val2 = RAD*(CONEVERTEX_Y - PLANE_Y2);
    decltype(result2) expect2{-val2, val2, PLANE_Y2, CONEVERTEX_Y, -val2, val2};
//	mDebug() << "result2=" << result2;
//	mDebug() << "expect2=" << expect2;
    for(size_t i = 0; i < result1.size(); ++i) {
        QVERIFY(std::abs(result2.at(i) - expect2.at(i)) < 1e-6);
    }


    Cone cone3("cone3", Pt{0, CONEVERTEX_Y, 0}, Vec{0, 1, 0}, RAD, 0);
    auto cone3r = cone3.createReverse();
    resultPlanes = BoundingBox::mergePlaneVectorsAnd(&timeoutFlag,
                                                     BoundingBox::mergePlaneVectorsAnd(&timeoutFlag,
                                                                                       cone3r->boundingPlaneVectors(),
                                                                                       plane1.boundingPlaneVectors()), plane2.boundingPlaneVectors());
    bb = BoundingBox::fromPlanes(&timeoutFlag, resultPlanes);
    const double val3 = std::max(val1, val2);
    const double ymax = std::max(PLANE_Y1, PLANE_Y2), ymin = std::min(PLANE_Y1, PLANE_Y2);
    auto result3 = bb.range();
    decltype(result3) expect3{-val3, val3, ymin, ymax, -val3, val3};
//	mDebug() << "result3=" << result3;
//	mDebug() << "expect3=" << expect3;
    for(size_t i = 0; i < result1.size(); ++i) {
        QVERIFY(std::abs(result3.at(i) - expect3.at(i)) < 1e-6);
    }
}

void BoundingboxTest::testPlane()
{
    using namespace std;
    using Vec = math::Vector<3>;
    const double XPOS1 = 10, XPOS2=-10, YPOS1=20, YPOS2=-20, ZPOS1=30, ZPOS2=-30;
    Plane p1("p1", Vec{-1,  0,  0},  -XPOS1);
    Plane p2("p2", Vec{ 1,  0,  0},   XPOS2);
    Plane p3("p3", Vec{ 0, -1,  0},  -YPOS1);
    Plane p4("p4", Vec{ 0,  1,  0},   YPOS2);
    Plane p5("p5", Vec{ 0,  0, -1},  -ZPOS1);
    Plane p6("p6", Vec{ 0,  0,  1},   ZPOS2);

//    mDebug() << "p1, n=" << p1.normal() << "d=" << p1.distance();
//    mDebug() << "p2, n=" << p2.normal() << "d=" << p2.distance();
//    mDebug() << "p3, n=" << p3.normal() << "d=" << p3.distance();
//    mDebug() << "p4, n=" << p4.normal() << "d=" << p4.distance();
//    mDebug() << "p5, n=" << p5.normal() << "d=" << p5.distance();
//    mDebug() << "p6, n=" << p6.normal() << "d=" << p6.distance();
    auto bb = BoundingBox::fromPlanes(&timeoutFlag, vector<vector<Plane>>{{p1, p2, p3, p4, p5, p6}});
//    mDebug() << "bb===" << bb.toInputString();
    auto result1 = bb.range();
    std::array<double, 6> expect1{XPOS2, XPOS1, YPOS2, YPOS1, ZPOS2, ZPOS1};
    QCOMPARE(result1.size(), expect1.size());
    for(std::size_t i = 0; i < result1.size(); ++i) {
//        mDebug() << "i=" << i << "diff=" << result1[i]-expect1[i];
        QVERIFY(std::abs(result1[i]-expect1[i]) < math::EPS);
    }

    // 斜め面によるBB
    math::Matrix<3> rotMat{0.5*std::sqrt(2), 0.5*std::sqrt(2), 0,   -0.5*std::sqrt(2), 0.5*std::sqrt(2), 0,    0, 0, 1};
    math::Matrix<4> matrix = math::Matrix<4>::IDENTITY();
    matrix.setRotationMatrix(rotMat);
    p1.transform(matrix);
    p2.transform(matrix);
    p3.transform(matrix);
    p4.transform(matrix);
//	mDebug() << "p1, n=" << p1.normal() << "d=" << p1.distance();
//	mDebug() << "p2, n=" << p2.normal() << "d=" << p2.distance();
//	mDebug() << "p3, n=" << p3.normal() << "d=" << p3.distance();
//	mDebug() << "p4, n=" << p4.normal() << "d=" << p4.distance();
    bb = BoundingBox::fromPlanes(&timeoutFlag, vector<vector<Plane>>{{p1, p2, p3, p4, p5, p6}});
    //mDebug() << "bb=" << bb.toInputString();
    auto result2 = bb.range();
    double val = 15*std::sqrt(2);
    std::array<double, 6> expect2{-val, val, -val, val, ZPOS2, ZPOS1};
    for(size_t i = 0; i < result2.size(); ++i) {
        //mDebug() << "diff=" << result2[i] - expect2[i];
        QVERIFY(std::abs(result2[i] - expect2[i]) < math::EPS);
    }
}

QTEST_APPLESS_MAIN(BoundingboxTest)

#include "tst_boundingboxtest.moc"
