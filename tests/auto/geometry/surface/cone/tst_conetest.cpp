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

#include "../../../../../core/geometry/surface/plane.hpp"
#include "../../../../../core/geometry/surface/cone.hpp"
#include "../../../../../core/utils/utils.hpp"

using namespace geom;
using namespace math;
typedef math::Vector<3> Vec;

class ConeTest : public QObject
{
	Q_OBJECT

public:
	ConeTest();

private Q_SLOTS:
    void testBoundingSurfaces();
    void testCase0();
    void testCase1();
    void test2SheetsForward();
    void testPlusSheetForward();
    void testMinusSheetForward();
    void testIntersection1();
    void testIntersection2();

private:
	std::vector<std::unique_ptr<const Cone> > twoSheetCones_;
	std::vector<std::unique_ptr<const Cone> > plusSheetCones_;
	std::vector<std::unique_ptr<const Cone> > minusSheetCones_;
};

ConeTest::ConeTest()
{
    math::Matrix<4> KxToKy { 0, 1, 0, 0,  -1, 0, 0, 0,  0, 0, 1, 0, 0, 0, 0, 1};
    twoSheetCones_.emplace_back(new Cone ("cone1", Point{0, 2, 0}, Vector<3>{0, 1, 0}, 0.5, 0));
    twoSheetCones_.emplace_back(Cone::createCone("cone2", std::vector<double>{0, 2, 0, 0.5*0.5, 0}, math::Matrix<4>(), Cone::TYPE::KY, false));
    twoSheetCones_.emplace_back(Cone::createCone("cone3", std::vector<double>{2, 0.5*0.5, 0}, math::Matrix<4>(), Cone::TYPE::KYO, false));
    twoSheetCones_.emplace_back(Cone::createCone("cone4", std::vector<double>{2, 0, 0, 0.5*0.5, 0}, KxToKy, Cone::TYPE::KX, false));
    twoSheetCones_.emplace_back(Cone::createCone("cone5", std::vector<double>{2, 0.5*0.5, 0}, KxToKy, Cone::TYPE::KXO, false));


    plusSheetCones_.emplace_back(new Cone ("coneP1", Point{0, 2, 0}, Vector<3>{0, 1, 0}, 0.5, 1));
    plusSheetCones_.emplace_back(Cone::createCone("coneP2", std::vector<double>{0, 2, 0, 0.5*0.5, +10}, math::Matrix<4>(), Cone::TYPE::KY, false));
    plusSheetCones_.emplace_back(Cone::createCone("coneP3", std::vector<double>{2, 0.5*0.5, +10}, math::Matrix<4>(), Cone::TYPE::KYO, false));
    plusSheetCones_.emplace_back(Cone::createCone("cone4", std::vector<double>{2, 0, 0, 0.5*0.5, 1}, KxToKy, Cone::TYPE::KX, false));
    plusSheetCones_.emplace_back(Cone::createCone("cone5", std::vector<double>{2, 0.5*0.5, 1}, KxToKy, Cone::TYPE::KXO, false));

    minusSheetCones_.emplace_back(new Cone ("coneM2", Point{0, 2, 0}, Vector<3>{0, 1, 0}, 0.5, -1));
    minusSheetCones_.emplace_back(Cone::createCone("coneM2", std::vector<double>{0, 2, 0, 0.5*0.5, -1}, math::Matrix<4>(), Cone::TYPE::KY, false));
    minusSheetCones_.emplace_back(Cone::createCone("coneM3", std::vector<double>{2, 0.5*0.5, -1}, math::Matrix<4>(), Cone::TYPE::KYO, false));
    minusSheetCones_.emplace_back(Cone::createCone("cone4", std::vector<double>{2, 0, 0, 0.5*0.5, -1}, KxToKy, Cone::TYPE::KX, false));
    minusSheetCones_.emplace_back(Cone::createCone("cone5", std::vector<double>{2, 0.5*0.5, -1}, KxToKy, Cone::TYPE::KXO, false));

}

void ConeTest::testBoundingSurfaces()
{
//	std::unique_ptr<Cone> cone1 = Cone::createCone("cone1",
//								  std::vector<double>{-64.25000, 3.48444, 1},
//								  math::Matrix<4>::IDENTITY(),
//								  Cone::TYPE::KZO, false);
    std::unique_ptr<Cone> cone1 = Cone::createCone("cone1",
                                  std::vector<double>{1, 4, 0},
                                  math::Matrix<4>::IDENTITY(),
                                  Cone::TYPE::KZO, false);
    auto cone1r = cone1->createReverse();
    auto planeVecs = cone1->boundingPlanes();
    for(const auto& pvec: planeVecs) {
        mDebug() << "elements ===";
        for(const auto &pl: pvec) {
            mDebug() << "plane===" << pl.toString();
        }
    }

//	QCOMPARE(planeVecs.size(), 4);  // 1シートconeの外接面要素は(側面+頂点ペアが)4個。
}


void ConeTest::testCase0()
{
    // 軸平行coneに軸平行粒子が頂点へ入射した場合
    auto cone1 = Cone::createCone("cone1", std::vector<double>{0, 0.25, 1}, math::Matrix<4>(), Cone::TYPE::KXO, false);
    QVERIFY(cone1->isForward(Point{-10, 1, 0}));
    Point p1 = cone1->getIntersection(Point{-10, 0, 0}, Vec{1, 0, 0});
    auto ex1 = math::Point{0, 0, 0};
    mDebug() << "KXO inter section, result=" << p1 << "expected=" << ex1;
    QVERIFY(math::isSamePoint(p1, ex1));
}

void ConeTest::testCase1()
{
    /*
     * z=0平面でy=xより下かつy=-xより上かつx>0の領域が内側となる円錐
     */
    auto cone1 = Cone::createCone("c1", std::vector<double>{0, 1, 1}, math::Matrix<4>(), Cone::TYPE::KXO, false);
    QVERIFY(cone1->isForward(Point{-10, 1, 0}));

    Point p1 = cone1->getIntersection(Point{-10, 0.5, 0}, Vec{1, 0, 0});
    //mDebug() << "isect=" << p1;
    auto ex1 = math::Point{0.5, 0.5, 0};
    QVERIFY(math::isSamePoint(p1, ex1));
}



void ConeTest::test2SheetsForward()
{
    for(auto &cone: twoSheetCones_) {
//		mDebug() << cone->toInputString();
        //mDebug() << "Test 2Sheets forward" << cone->toString();
        // -側シート部分
        QVERIFY(!cone->isForward(Point{0, 0, 0}));     // 原点は-側シートbackWard
        QVERIFY(cone->isForward(Point{10, 0, 0}));     // y = 0 の外の方は-側シートのforWard
        QVERIFY(!cone->isForward(Point{0, -10, 0}));   // y軸上-yは常にbackWard(頂点は面上なのでforward扱い)
// TR生成した場合数値誤差で面直上に来ることは期待できないのでテストから面上チェックは外す
//		QVERIFY(cone->isForward(Point{-1, 0, 0}));     // sheet面上はおもて面なのでforward扱い
        // +側シート部分
        QVERIFY(!cone->isForward(Point{10, 100, 0}));  // yの上の方は+側シートbackWard
        QVERIFY(cone->isForward(Point{-1e+4, 10, 0})); // y = 0 の外の方は+側シートのforWard
        QVERIFY(!cone->isForward(Point{0, +10, 0}));   // y軸上+yは常にbackWard(頂点は面上なのでforward扱い)
// TR生成した場合数値誤差で面直上に来ることは期待できないのでテストから面上チェックは外す
//		QVERIFY(cone->isForward(Point{1, 4, 0}));     // sheet面上はおもて面なのでforward扱い

        math::Matrix<4> matrix{
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            2, 2, 0, 1
        };
        std::unique_ptr<Cone> trcone(new Cone(*cone));
        trcone->transform(matrix);
        //mDebug() << cone1.toString();
        // -側シート部分
        QVERIFY(!trcone->isForward(Point{2, 2, 0}));     // 原点は-側シートbackWard
        QVERIFY(trcone->isForward(Point{12, 2, 0}));     // y = 0 の外の方は-側シートのforWard
        QVERIFY(!trcone->isForward(Point{2, -8, 0}));   // y軸上-yは常にbackWard(頂点は面上なのでforward扱い)
// TR生成した場合数値誤差で面直上に来ることは期待できないのでテストから面上チェックは外す
//		QVERIFY(trcone->isForward(Point{1, 2, 0}));     // -sheet面上はおもて面なのでforward扱い
        // +側シート部分
        QVERIFY(!trcone->isForward(Point{12, 102, 0}));  // yの上の方は+側シートbackWard
        QVERIFY(trcone->isForward(Point{-1e+4, 12, 0})); // y = 0 の外の方は+側シートのforWard
        QVERIFY(!trcone->isForward(Point{2, +12, 0}));   // y軸上+yは常にbackWard(頂点は面上なのでforward扱い)
// TR生成した場合数値誤差で面直上に来ることは期待できないのでテストから面上チェックは外す
//		QVERIFY(trcone->isForward(Point{3, 6, 0}));     // -sheet面上はおもて面なのでforward扱い
    }
}

void ConeTest::testPlusSheetForward()
{
    for(auto &cone: plusSheetCones_) {
        // -側シート部分は常にforward
        QVERIFY(cone->isForward(Point{0, 0, 0}));      // 原点は-側シートbackWardなので+シートではforward
        QVERIFY(cone->isForward(Point{10, 0, 0}));     // y = 0 の外の方は-側シートのforWard
        QVERIFY(cone->isForward(Point{0, -10, 0}));   // y軸上-yは常にbackWard(頂点は面上なのでforward扱い)
        //QVERIFY(cone->isForward(Point{-1, 0, 0}));     // -sheet面上はおもて面なのでforward扱い
        // +側シート部分は2シートと同じ結果
        QVERIFY(!cone->isForward(Point{10, 100, 0}));  // yの上の方は+側シートbackWard
        QVERIFY( cone->isForward(Point{-1e+4, 10, 0})); // y = 0 の外の方は+側シートのforWard
        QVERIFY(!cone->isForward(Point{0, +10, 0}));   // y軸上+yは常にbackWard(頂点は面上なのでforward扱い)
        //QVERIFY( cone->isForward(Point{1, 4, 0}));     // -sheet面上はおもて面なのでforward扱い
    }
}

void ConeTest::testMinusSheetForward()
{
    for(auto &cone: minusSheetCones_) {
        // -側シート部分は2シートと同じ。
        QVERIFY(!cone->isForward(Point{0, 0, 0}));     // 原点は-側シートbackWard
        QVERIFY(cone->isForward(Point{10, 0, 0}));     // y = 0 の外の方は-側シートのforWard
        QVERIFY(!cone->isForward(Point{0, -10, 0}));   // y軸上-yは常にbackWard(頂点は面上なのでforward扱い)
//		QVERIFY(cone->isForward(Point{-1, 0, 0}));     // -sheet面上はおもて面なのでforward扱い
        // +側シート部分は常にforward
        QVERIFY(cone->isForward(Point{10, 100, 0}));  // yの上の方は+側シートbackWard
        QVERIFY(cone->isForward(Point{-1e+4, 10, 0})); // y = 0 の外の方は+側シートのforWard
        QVERIFY(cone->isForward(Point{0, +10, 0}));   // y軸上+yは常にbackWard(頂点は面上なのでforward扱い)
//		QVERIFY(cone->isForward(Point{1, 4, 0}));     // -sheet面上はおもて面なのでforward扱い
    }
}

void ConeTest::testIntersection1()
{
    // 2シートコーン
    //geom::Cone *cone = new geom::Cone("cone4", Point{0, 2, 0}, Vec{0, 1, 0}, 0.5, 0);
    for(auto &cone: twoSheetCones_) {
        // 円錐外の場合
        Point p1 = cone->getIntersection(Point{-100, 0, 0}, Vec{1, 0, 0});
        auto p1_ex = Point{-1, 0, 0};
//		mDebug() << "cone=" << cone->name() << cone->toString();
//		mDebug() << "result=" << p1;
//		mDebug() << "expect=" << p1_ex;
        QVERIFY(math::isSamePoint(p1, p1_ex));
        // 円錐内の場合
        Point p2 = cone->getIntersection(Point{-0.5, 0, 0}, Vec{1, 0, 0});
        auto p2_ex = Point{1, 0, 0};
        QVERIFY(math::isSamePoint(p2, p2_ex));
        Point p3 = cone->getIntersection(Point{0.6, 0, 0}, Vec{1, 0, 0});
        auto p3_ex = Point{1, 0, 0};
        QVERIFY(math::isSamePoint(p3, p3_ex));
        // 交点なし
        Point p4 = cone->getIntersection(Point{20, 5, 0}, (Vec{10, 1, 0}).normalized());
        QVERIFY(math::isSamePoint(p4, Point::INVALID_VECTOR()));

        // 頂点は面の直上なのでsurfaceの表面前側として扱われる。
        // よって頂点から外側へ交点を探すと交点なし、内側へ探すと頂点が返る。
        // のだが2シートコーンの場合は

        // 頂点から外側向きの場合は交点なし
        Point p5 = cone->getIntersection(Point{0, 2, 0}, (Vec{10, 1, 0}).normalized());
        QVERIFY(math::isSamePoint(p5, Point::INVALID_VECTOR()));
        // 頂点から軸方向の交点は頂点
        Point p6 = cone->getIntersection(Point{0, 2, 0}, Vec{0, 1, 0});
        auto p6_ex = Point{0, 2, 0};
        QVERIFY(math::isSamePoint(p6, p6_ex));
    }
}

void ConeTest::testIntersection2()
{
    for(auto &cone: plusSheetCones_) {
        // +側1シートコーン。軸ベクトル側を＋に定義する。
        // 横方向楕円断面入射   -側へ入射した場合は交点なしとなる。
        auto p1 = cone->getIntersection(Point{-100, 0, 0}, (Vec{1, 0, 0}).normalized());
        //mDebug() << "p1=" << p1;
        QVERIFY(math::isSamePoint(p1, Point::INVALID_VECTOR()));
        // 縦方向放物断面入射 -側への入射は交点なし
        auto p2 = cone->getIntersection(Point{-3, 3, 0}, (Vec{1, 3, 0}).normalized());
        auto p2_ex = Point{-2, 6, 0};
        QVERIFY(math::isSamePoint(p2, p2_ex));
        p2 = cone->getIntersection(Point{-3, 3, 0}, (Vec{1, -10, 0}).normalized());
        QVERIFY(math::isSamePoint(p2, Point::INVALID_VECTOR()));
        // 縦方向双極断面入射、-側への入射は交点なし
        auto p3 = cone->getIntersection(Point{-2, 2, 0}, (Vec{0, 1, 0}).normalized());
        auto p3_ex = Point{-2, 6, 0};
        QVERIFY(math::isSamePoint(p3, p3_ex));
        p3 = cone->getIntersection(Point{-2, 2, 0}, (Vec{0, -1, 0}).normalized());
        QVERIFY(math::isSamePoint(p3, Point::INVALID_VECTOR()));
    }

    for(auto &cone: minusSheetCones_) {
        // -側1シートコーン。定義ではrefPoint_を含む方をマイナス側と定義する。
        // 横方向楕円断面入射   +側へ入射した場合は交点なしとなる。
        auto p4 = cone->getIntersection(Point{100, 2, 0}, (Vec{-1, 0, 0}).normalized());
        QVERIFY(math::isSamePoint(p4, Point::INVALID_VECTOR()));
        // 縦方向放物断面入射 +側への入射は交点なし
        auto p5 = cone->getIntersection(Point{3, 3, 0}, (Vec{-1, -5, 0}).normalized());
        auto p5_ex = Point{2, -2, 0};
        QVERIFY(math::isSamePoint(p5, p5_ex));
        p5 = cone->getIntersection(Point{3, 3, 0}, (Vec{-1, 3, 0}).normalized());
        QVERIFY(math::isSamePoint(p5, Point::INVALID_VECTOR()));
        // 縦方向双極断面入射、+側への入射は交点なし
        auto p6 = cone->getIntersection(Point{2, 2, 0}, (Vec{0, -1, 0}).normalized());
        auto p6_ex = Point{2, -2, 0};
        QVERIFY(math::isSamePoint(p6, p6_ex));
        p6 = cone->getIntersection(Point{-2, 2, 0}, (Vec{0, 1, 0}).normalized());
        QVERIFY(math::isSamePoint(p6, Point::INVALID_VECTOR()));
    }
}

QTEST_APPLESS_MAIN(ConeTest)

#include "tst_conetest.moc"
