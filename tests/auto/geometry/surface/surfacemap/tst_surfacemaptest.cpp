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


#include "core/geometry/surface/surfacemap.hpp"
#include "core/geometry/surface/sphere.hpp"
#include "core/geometry/surf_utils.hpp"
#include "core/utils/message.hpp"

using namespace geom;
using namespace math;

class SurfacemapTest : public QObject
{
	Q_OBJECT

public:
	SurfacemapTest();

private Q_SLOTS:
    void testCase1();
    void testNameAndIdSame();
    void testNameAndNextIdSame();
};

SurfacemapTest::SurfacemapTest() {}


void SurfacemapTest::testCase1()
{
    Surface::initID();
    SurfaceMap smap {
        std::make_shared<Sphere>("s1", Point{0, 10, 0}, 20),
        std::make_shared<Sphere>("s2", Point{0, 10, 0}, 20),
        std::make_shared<Sphere>("s5.1", Point{0, 10, 0}, 20)
    };
    utils::addReverseSurfaces(&smap);

    std::string eqStr = "((s1 -s2):-s5.1 ) s1";
    std::string expected = "((1 -2):-3 ) 1";
    auto indexEqStr = SurfaceMap::makeIndexEquation(eqStr, smap);
    mDebug() << "nameEq =" << eqStr;
    mDebug() << "result =" << indexEqStr;
    mDebug() << "expect =" << expected;
    QCOMPARE(indexEqStr, expected);
}

// surface名がsurfaceIDに一致している場合
void SurfacemapTest::testNameAndIdSame()
{
    Surface::initID();
    SurfaceMap smap {
        std::make_shared<Sphere>("1", Point{0, 10, 0}, 20),
        std::make_shared<Sphere>("s2", Point{0, 10, 0}, 20),
        std::make_shared<Sphere>("s5.1", Point{0, 10, 0}, 20)
    };
    utils::addReverseSurfaces(&smap);
    std::string eqStr = "((1 -s2):-s5.1 ) 1";
    std::string expected = "((1 -2):-3 ) 1";
    auto indexEqStr = SurfaceMap::makeIndexEquation(eqStr, smap);
    mDebug() << "nameEq =" << eqStr;
    mDebug() << "result =" << indexEqStr;
    mDebug() << "expect =" << expected;
    QCOMPARE(indexEqStr, expected);
}


// surface名が次のsurfaceIDに一致ししている場合
void SurfacemapTest::testNameAndNextIdSame()
{
    Surface::initID();
    SurfaceMap smap {
        std::make_shared<Sphere>("2", Point{0, 10, 0}, 20),
        std::make_shared<Sphere>("s2", Point{0, 10, 0}, 20),
        std::make_shared<Sphere>("s5.1", Point{0, 10, 0}, 20)
    };
    utils::addReverseSurfaces(&smap);
    std::string eqStr = "((2 -s2):-s5.1 ) 2";
    std::string expected = "((1 -2):-3 ) 1";
    auto indexEqStr = SurfaceMap::makeIndexEquation(eqStr, smap);
    mDebug() << "\nnameEq =" << eqStr;
    mDebug() << "result =" << indexEqStr;
    mDebug() << "expect =" << expected;
    QCOMPARE(indexEqStr, expected);
}

QTEST_APPLESS_MAIN(SurfacemapTest)

#include "tst_surfacemaptest.moc"
