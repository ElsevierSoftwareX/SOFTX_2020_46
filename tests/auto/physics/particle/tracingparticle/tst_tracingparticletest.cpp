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

#include <string>
#include <unordered_map>

#include "core/physics/particle/tracingparticle.hpp"
#include "core/math/nvector.hpp"
#include "core/geometry/surface/cone.hpp"
#include "core/geometry/surface/sphere.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surfacecreator.hpp"
#include "core/geometry/cellcreator.hpp"
#include "core/geometry/cell/cell.hpp"
#include "core/formula/logical/lpolynomial.hpp"
#include "core/utils/utils.hpp"



using namespace phys;
using namespace geom;
using namespace math;
using namespace lg;

namespace {
const double MAX_LENGTH = 100.0;
}

class TracingparticleTest : public QObject
{
	Q_OBJECT

public:
	TracingparticleTest();

private:
    SurfaceCreator screator;
    CellCreator cellObjects;
    std::shared_ptr<const Cell> cell1;
    std::shared_ptr<const Cell> cell2;
    std::shared_ptr<const Cell> cell3;


private Q_SLOTS:
    void testTracingSphereBound();
//    void testTracingPlaneBound();
//    void testNormalCellStartCase();
//    void testUndefinedCellStartCase();

};


TracingparticleTest::TracingparticleTest()
{
    const double sq3 = std::sqrt(3.0);
    Surface::map_type sMap {
        std::make_shared<Sphere>("S1", Point{-10, 0, 0}, 20),
        std::make_shared<Sphere>("S2", Point{10, 0, 0}, 20),
        std::make_shared<Sphere>("S3", Point{30, 0, 0}, 10),
        std::make_shared<Plane>("P",   Vector<3>{sq3, sq3, sq3}, 2.0),
        std::make_shared<Plane>("PX1", Vector<3>{1, 0, 0}, -10),
        std::make_shared<Plane>("PX2", Vector<3>{1, 0, 0},  10),
        std::make_shared<Plane>("PX3", Vector<3>{1, 0, 0},  20),
        std::make_shared<Plane>("PY1", Vector<3>{0, 1, 0}, -10),
        std::make_shared<Plane>("PY2", Vector<3>{0, 1, 0},  10),
        std::make_shared<Plane>("PZ1", Vector<3>{0, 0, 1}, -10),
        std::make_shared<Plane>("PZ2", Vector<3>{0, 0, 1},  10)
    };
    //utils::addReverseSurfaces(&surfaceMap);	// 裏面はsurfObjectsが作ってくれる。

    screator = SurfaceCreator(sMap);
    sMap = screator.map();

    cell1 = std::make_shared<const Cell>("C1", sMap,
                                     LogicalExpression<int>(std::vector<int>{sMap.getIndex("-S1"), sMap.getIndex("-S2"), sMap.getIndex("PZ1")}), 1.0);
    cell2 = std::make_shared<const Cell>("C2", sMap,
                                     LogicalExpression<int>(std::vector<int>{sMap.getIndex("S1"), sMap.getIndex("-S2"), sMap.getIndex("PZ1")}), 1.0);
    cell3 = std::make_shared<const Cell>("C3", sMap,
                                     LogicalExpression<int>(std::vector<int>{sMap.getIndex("S2"), sMap.getIndex("-S3"), sMap.getIndex("PZ1")}), 1.0);
    Cell::const_map_type cellMap{
        {cell1->cellName(), cell1},
        {cell2->cellName(), cell2},
        {cell3->cellName(), cell3}
    };

    //utils::updateCellSurfaceConnection(cellMap);	// cell-surface連結構造updateはCellObjectsが作ってくれる
    cellObjects = CellCreator(cellMap);
    screator.removeUnusedSurfaces(false);


}

// FIXME 面上での粒子発生を許容したい


/*
 * 原点に置かれた半径50の球S1を横切る時のチェック
 *
 */
void TracingparticleTest::testTracingSphereBound()
{
    const double rad = 50;
    Surface::map_type sMap {
        std::make_shared<Sphere>("S1", Point{0, 0, 0}, rad)
    };
    SurfaceCreator screator(sMap);
    sMap = screator.map();

    auto c1 = std::make_shared<const Cell>("C1", screator.map(), lg::LogicalExpression<int>(sMap.getIndex("-S1")), 1.0);
    auto c99 = std::make_shared<const Cell>("C99", screator.map(), lg::LogicalExpression<int>(sMap.getIndex("S1")), 1.0);

    Cell::const_map_type cellMap{{c1->cellName(), c1},	{c99->cellName(), c99}};
    CellCreator ccreator(cellMap);
    screator.removeUnusedSurfaces(false);
    // ここでUNDEFIEDセルの初期化を実施。
    ccreator.initUndefinedCell(screator.map());
    const double MAX_LEN = 10*rad;

    // -X側から
    Point startPoint{-2*rad, 0, 0};
    TracingParticle p(1.0, startPoint, Vector<3>{1, 0, 0}, 1.0, nullptr, ccreator.cells(), MAX_LEN);
    p.trace();
    mDebug() << "passed=" << p.passedCells();
    mDebug() << "tracks=" << p.trackLengths();

    std::vector<std::string> expected;
    QCOMPARE(p.passedCells().size(), static_cast<size_t>(3));
    expected = std::vector<std::string>{"C99", "C1", "C99"};
    std::vector<double> expectedTracks{rad, 2*rad, MAX_LEN-3*rad};
    QCOMPARE(p.passedCells(), expected);
//    for(size_t i = 0; i < expectedTracks.size(); ++i) {
//        mDebug() << "i=" << i << "diff=" << expectedTracks.at(i) - p.trackLengths().at(i);
//    }
    for(size_t i = 0; i < expectedTracks.size(); ++i) {
//        mDebug() << "i=" << i << "diff=" << expectedTracks.at(i) - p.trackLengths().at(i);
        QVERIFY(std::abs(expectedTracks.at(i) - p.trackLengths().at(i)) < Point::EPS);
    }

    // ＋X側から
    Point startPoint2{2*rad, 0, 0};
    TracingParticle p2(1.0, startPoint2, Vector<3>{-1, 0, 0}, 1.0, nullptr, ccreator.cells(), MAX_LEN);
    p2.trace();
    //mDebug() << "passed=" << p2.passedCells();
    QCOMPARE(p2.passedCells().size(), static_cast<size_t>(3));
    expected = std::vector<std::string>{"C99", "C1", "C99"};
    QCOMPARE(p2.passedCells(), expected);
    for(size_t i = 0; i < expectedTracks.size(); ++i) {
        //mDebug() << "i=" << i << "diff=" << std::abs(expectedTracks.at(i) - p2.trackLengths().at(i));
        QVERIFY(std::abs(expectedTracks.at(i) - p2.trackLengths().at(i)) < Point::EPS);
    }

    // 球の外側に向かった場合の挙動は？
    TracingParticle p3{1.0, startPoint, Vector<3>{-1, 0, 0}, 0, nullptr, ccreator.cells(), MAX_LEN};
    p3.trace();
//	mDebug() << "passed=" << p3.passedCells();
//	mDebug() << "tracks=" << p3.trackLengths();
    expected = std::vector<std::string>{"C99"};
    expectedTracks = std::vector<double>{MAX_LEN};
    QCOMPARE(p3.passedCells(), expected);
    for(size_t i = 0; i < expectedTracks.size(); ++i) {
        //mDebug() << "i=" << i << "diff=" << std::abs(expectedTracks.at(i) - p3.trackLengths().at(i));
        QVERIFY(std::abs(expectedTracks.at(i) - p3.trackLengths().at(i)) < Point::EPS);
    }
}

//void TracingparticleTest::testTracingPlaneBound()
//{
//    double xpos = 10;
//    const double MAXLEN = 10*xpos;
//    Surface::map_type sMap {
//        std::make_shared<Plane>("PX1", Vector<3>{1, 0, 0}, -xpos),
//        std::make_shared<Plane>("PX2", Vector<3>{1, 0, 0},  xpos)
//    };
//    SurfaceCreator screator(sMap);
//    sMap = screator.map();

//    std::shared_ptr<const Cell> c1 = std::make_shared<const Cell>("CP1", screator.map(),
//                                     lg::LogicalExpression<int>(std::vector<int>{sMap.getIndex("PX1"), sMap.getIndex("-PX2")}), 1.0);
//    std::shared_ptr<const Cell> c99 = std::make_shared<const Cell>("CP99", screator.map(),
//                                     lg::LogicalExpression<int>(std::vector<int>{sMap.getIndex("-PX1"), sMap.getIndex("PX2")}), 1.0);

//    Cell::const_map_type cellMap{{c1->cellName(), c1},	{c99->cellName(), c99}};
//    CellCreator cObjs(cellMap);
//    screator.removeUnusedSurfaces(false);
//    // ここでUNDEFIEDセルの初期化を実施。
//    cObjs.initUndefinedCell(screator.map());


//    /*
//     * PXの法線ベクトルはxの正の方向を向いており、面上は +PXのforward側となるため、
//     * C1  PX1 -PX2
//     * C99 -PX:PX2
//     * PX1 PX -10
//     * PX2 PX 10
//     * とした場合、
//     * x=10 の点では +PX2::Forward==trueとなるため+PX2で定義されているC99と判定され、
//     * x=-10の点では +PX1::Forward==trueとなるため+PX1で定義されているC1と判定される。
//     * 故に、x=10からマイナス側にトレースした場合と、x=-10から+側にトレースした場合とでは
//     * 通過セルの数が異なる。
//     */
//    // -X側から
//    Point startPoint{-xpos, 0, 0};
//    TracingParticle p(1.0, startPoint, Vector<3>{1, 0, 0}, 1.0, nullptr, cObjs.cells(), MAXLEN);
//    p.trace();
////	mDebug() << "passed=" << p.passedCells();
////	mDebug() << "tracks=" << p.trackLengths();
//    QCOMPARE(p.passedCells().size(), static_cast<size_t>(2));
//    auto expected = std::vector<std::string>{"CP1", "CP99"};
//    std::vector<double> expectedTracks{2*xpos, MAXLEN-2*xpos};
//    QCOMPARE(p.passedCells(), expected);
//    for(size_t i = 0; i < expectedTracks.size(); ++i) {
//        //mDebug() << "i=" << i << "diff=" << expectedTracks.at(i) - p.trackLengths().at(i);
//        QVERIFY(std::abs(expectedTracks.at(i) - p.trackLengths().at(i)) < Point::EPS);
//    }
//    // +X側から
//    Point startPoint2{xpos, 0, 0};
//    TracingParticle p2(1.0, startPoint2, Vector<3>{-1, 0, 0}, 1.0, nullptr, cObjs.cells(), MAXLEN);
//    p2.trace();
////	mDebug() << "passed=" << p2.passedCells();
////	mDebug() << "tracks=" << p2.trackLengths();
//    QCOMPARE(p2.passedCells().size(), static_cast<size_t>(3));
//    auto expected2 = std::vector<std::string>{"CP99", "CP1", "CP99"};
//    std::vector<double> expectedTracks2{0, 2*xpos, MAXLEN-2*xpos};
//    QCOMPARE(p2.passedCells(), expected2);
//    for(size_t i = 0; i < expectedTracks2.size(); ++i) {
//        //mDebug() << "i=" << i << "diff=" << expectedTracks.at(i) - p.trackLengths().at(i);
//        QVERIFY(std::abs(expectedTracks2.at(i) - p2.trackLengths().at(i)) < Point::EPS);
//    }
//}



//void TracingparticleTest::testNormalCellStartCase()
//{
//    /*
//     * UNDEFINEDセルはCellのstaticメンバなので他のテストと共有されてしまう！！！！！！
//     * テストごとに呼ぶか、ジオメトリを正しく共有する必要がる。
//     */
//    // ここでUNDEFIEDセルの初期化を再度実施。testがマルチスレッドで実行されたらアウト。
//    cellObjects.initUndefinedCell(screator.map());
//    //mDebug() << "undefinedセルの情報：" << geom::Cell::UNDEFINED_CELL_PTR()->toString();
//    TracingParticle p(1.0, Point{0, 0, 0}, Vector<3>{1, 0, 0}, 1.0, cell1.get(), cellObjects.cells(), MAX_LENGTH, false, false);
//    p.trace();
//    double totalLength = 0;
//    for(auto &x: p.trackLengths()) {
//        totalLength += x;
//    }
////	mDebug() << "eps=" << Point::eps() << ", delta =" << Point::delta();
////	mDebug() << "passed =" << p.passedCells();
////	mDebug() << "track lengths =" << p.trackLengths();
//    //mDebug() << "expected length =" << MAX_LENGTH << ", result length =" << totalLength;

//    std::vector<double> expectedTrackLengths{10.0, 20.0, 10, 60.0};
//    std::vector<std::string> expectedPassedCells{"C1", "C2", "C3", "*C_u"};
//    QCOMPARE(MAX_LENGTH, totalLength);
////	mDebug() << "passed cells=" << p.passedCells();
////	mDebug() << "expected cells=" << expectedPassedCells;
////	mDebug() << "passed tracks=" << p.trackLengths();
////	mDebug() << "expected tracks=" << expectedTrackLengths;
//    QCOMPARE(p.passedCells(), expectedPassedCells);
//    for(size_t i = 0; i < p.trackLengths().size(); ++i) {
//        //mDebug() << "i = " << i << "diff=" << std::abs(expectedTrackLengths.at(i) - p.trackLengths().at(i));
//        QVERIFY(std::abs(expectedTrackLengths.at(i) - p.trackLengths().at(i)) < Point::EPS);
//    }
//}

//// 出発点がundefinedCellだった場合。
//void TracingparticleTest::testUndefinedCellStartCase()
//{
//    /*
//     * UNDEFINEDセルはCellのstaticメンバなので他のテストと共有されてしまう！！！！！！
//     * テストごとに呼ぶ必要がある。
//     */
//    // ここでUNDEFIEDセルの初期化を再度実施。testがマルチスレッドで実行されたらアウト。
//    cellObjects.initUndefinedCell(screator.map());
//    mDebug() << "undefinedセルの情報：" << geom::Cell::UNDEFINED_CELL_PTR()->toString();

//    TracingParticle p2(1.0, Point{-25, 0, 0}, Vector<3>{1, 0, 0}, 1.0, nullptr, cellObjects.cells(), MAX_LENGTH, true, true);
//    try {
//        p2.trace();
//    } catch (std::exception &e) {
//        p2.dumpEvents(std::cout);
//        mFatal() << e.what();
//    }

////	// 合計飛程長のチェック
////	double totalLength2 = 0;
////	for(auto &x: p2.trackLengths()) { totalLength2 += x;}
////	//mDebug() << "expected length =" << MAX_LENGTH << ", result length =" << totalLength2;
////	QCOMPARE(MAX_LENGTH, totalLength2);

////	// 通過セルチェック
////	//mDebug() << "passed =" << p2.passedCells();
////	std::vector<std::string> expectedPassedCells2{"*C_u", "C1", "C2", "C3", "*C_u"};
////	QCOMPARE(p2.passedCells(), expectedPassedCells2);

////	// 通過セル長さのチェック
////	//mDebug() << "track lengths =" << p2.trackLengths();
////	std::vector<double> expectedTrackLengths2{15, 20.0, 20.0, 10, 35.0};
////	for(size_t i = 0; i < expectedTrackLengths2.size(); ++i) {
////		//mDebug() << "i=" << i << "dif=" << std::abs(expectedTrackLengths2.at(i) - p2.trackLengths().at(i));
////		QVERIFY(std::abs(expectedTrackLengths2.at(i) - p2.trackLengths().at(i)) < Point::EPS);
////	}
//}



QTEST_APPLESS_MAIN(TracingparticleTest)

#include "tst_tracingparticletest.moc"
