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



#include "core/physics/particle/particle.hpp"
#include "core/physics/particleexception.hpp"
#include "core/math/nvector.hpp"
#include "core/geometry/surface/sphere.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/cell/cell.hpp"
#include "core/geometry/surf_utils.hpp"
#include "core/geometry/cell_utils.hpp"
#include "core/formula/logical/lpolynomial.hpp"
#include "core/utils/utils.hpp"

using namespace phys;
using namespace geom;
using namespace math;
using namespace lg;

typedef lg::LogicalExpression<int> Lpolynomial;

class Patricle_testTest : public QObject
{
	Q_OBJECT

public:
	Patricle_testTest();


private Q_SLOTS:
    void testTransportInSphere();
    void testTransportInPlane();
    void testTransportInDoubleSphere();
    void testComplexCell();
};

Patricle_testTest::Patricle_testTest() {}

void Patricle_testTest::testTransportInSphere()
{
    mDebug() << "eps=" << math::Point::eps() << ", delta=" << math::Point::delta();
    // cell構築にはsurface名、surfaceスマポのサーフェイスマップと、
    // Bool多項式が必要
    std::unordered_map<std::string, std::shared_ptr<Surface>>  hashMap {
        {"S1",  std::make_shared<Sphere>("S1", Point{-10, 0, 0}, 20)},
        {"S2",  std::make_shared<Sphere>("S2", Point{10, 0, 0}, 20)},
        {"PX1", std::make_shared<Plane>("PX1", Vector<3>{1, 0, 0}, -10)},
        {"PX2", std::make_shared<Plane>("PX2", Vector<3>{1, 0, 0},  10)},
        {"PX3", std::make_shared<Plane>("PX2", Vector<3>{1, 0, 0},  20)},
        {"PY1", std::make_shared<Plane>("PY1", Vector<3>{0, 1, 0}, -10)},
        {"PY2", std::make_shared<Plane>("PY2", Vector<3>{0, 1, 0},  10)},
        {"PZ1", std::make_shared<Plane>("PZ1", Vector<3>{0, 0, 1}, -10)},
        {"PZ2", std::make_shared<Plane>("PZ2", Vector<3>{0, 0, 1},  10)}
    };
    Surface::map_type gsMap;
    for(auto &surfPair: hashMap) {
        gsMap.registerSurface(surfPair.second->getID(), surfPair.second);
    }


    // 裏面を作成
    utils::addReverseSurfaces(&gsMap);

    //Cell作成時にsurfaceオブジェクトには利用したcellを登録する。
    const std::shared_ptr<const Cell> cell1
            = std::make_shared<const Cell>("C1", gsMap, lg::LogicalExpression<int>(std::vector<int>{gsMap.getIndex("-S1"),gsMap.getIndex("S2")}), 1.0);
    const std::shared_ptr<const Cell> cell2
            = std::make_shared<const Cell>("C2", gsMap, lg::LogicalExpression<int>(std::vector<int>{gsMap.getIndex("-S2")}), 1.0);

    Cell::const_map_type cellMap{
        {cell1->cellName(), cell1},
        {cell2->cellName(), cell2}
    };

    // cellMapとsurfaceMapの接続関係を構築する。
    // cellには既にcontactSurfaceが登録されているので、
    // surfaceのcontactCellを更新する。
    utils::updateCellSurfaceConnection(cellMap);
    Cell::initUndefinedCell(gsMap);


    // 裏面も登録されていることを確認
    QVERIFY(gsMap.at(gsMap.getIndex("-S1"))->contactCellsMap().find("C1")
             != gsMap.at(gsMap.getIndex("-S1"))->contactCellsMap().end());
    QVERIFY(gsMap.at(gsMap.getIndex("S1"))->contactCellsMap().find("C1")
             != gsMap.at(gsMap.getIndex("S1"))->contactCellsMap().end());
    QVERIFY(gsMap.at(gsMap.getIndex("S2"))->contactCellsMap().find("C1")
             != gsMap.at(gsMap.getIndex("S2"))->contactCellsMap().end());

    // Particle派生クラスは発生位置と現在cellが不一致なら例外発生
    QVERIFY_EXCEPTION_THROWN(Particle(1.0, Point{1000, 0, 0}, Vector<3>{1, 0, 0}, 1.0, cell1.get(), cellMap), phys::InvalidSource);
    // 位置(-15, 0, 0)、x軸方向1MeV粒子をcellセル内に発生
    Particle *p = new Particle (1.0, Point{-15, 0, 0}, Vector<3>{1, 0, 0}, 1.0, cell1.get(), cellMap);

    p->moveToCellBound();
    Point expectedPoint1{-10, 0, 0};
    QVERIFY(math::isSamePoint(p->position(), expectedPoint1));


    p->enterCell();
    //mDebug() << "currentCell=" << p->currentCell()->cellName();
    QCOMPARE(p->currentCell()->cellName(), std::string("C2"));

    p->moveToCellBound();
    Point expectedPoint2{30, 0, 0};
    QVERIFY(math::isSamePoint(p->position(), expectedPoint2));


}

void Patricle_testTest::testTransportInPlane()
{
    // cell構築にはsurface名、surfaceスマポのサーフェイスマップと、
    // Bool多項式が必要
    std::vector<std::shared_ptr<Surface>> surfVector{
        std::make_shared<Sphere>("S1", Point{-10, 0, 0}, 20),
        std::make_shared<Sphere>("S2", Point{10, 0, 0}, 20),
        std::make_shared<Plane>("PX1", Vector<3>{1, 0, 0}, -10),
        std::make_shared<Plane>("PX2", Vector<3>{1, 0, 0},  10),
        std::make_shared<Plane>("PX3", Vector<3>{1, 0, 0},  20),
        std::make_shared<Plane>("PY1", Vector<3>{0, 1, 0}, -10),
        std::make_shared<Plane>("PY2", Vector<3>{0, 1, 0},  10),
        std::make_shared<Plane>("PZ1", Vector<3>{0, 0, 1}, -10),
        std::make_shared<Plane>("PZ2", Vector<3>{0, 0, 1},  10)
    };

    Surface::map_type gsMap;
    for(auto &surf: surfVector) {
        gsMap.registerSurface(surf->getID(), surf);
    }

    // 裏面を作成
    utils::addReverseSurfaces(&gsMap);

    const std::shared_ptr<const Cell> cell1
            = std::make_shared<const Cell>("C1", gsMap,
                                     Lpolynomial(std::vector<int>{gsMap.getIndex("PX1"), gsMap.getIndex("-PX2"), gsMap.getIndex("PY1"),
                                                      gsMap.getIndex("-PY2"), gsMap.getIndex("PZ1"), gsMap.getIndex("-PZ2")}), 1.0);
    const std::shared_ptr<const Cell> cell2
            = std::make_shared<const Cell>("C2", gsMap,
                                     Lpolynomial(std::vector<int>{gsMap.getIndex("PX2"), gsMap.getIndex("-PX3"), gsMap.getIndex("PY1"),
                                                      gsMap.getIndex("-PY2"), gsMap.getIndex("PZ1"), gsMap.getIndex("-PZ2")}), 1.0);

    Cell::const_map_type cellMap{
        {cell1->cellName(), cell1},
        {cell2->cellName(), cell2}
    };
    // surfaceにcellを登録
    utils::updateCellSurfaceConnection(cellMap);
    Cell::initUndefinedCell(gsMap);

    // 位置(-5, 0, 0)、x軸方向1MeV粒子をcellセル内に発生
    Particle *p = new Particle (1.0, Point{-5, 0, 0}, Vector<3>{1, 0, 0}, 1.0, cell1.get(), cellMap);

    p->moveToCellBound();
    Point expectedPoint1{10, 0, 0};
    QVERIFY(math::isSamePoint(p->position(), expectedPoint1));

    p->enterCell();
    QCOMPARE(p->currentCell()->cellName(), std::string("C2"));

    p->moveToCellBound();
    Point expectedPoint2{20, 0, 0};
    QVERIFY(math::isSamePoint(p->position(), expectedPoint2));
    //QVERIFY(!p->enterCell()); // この先にcellなしなのでfalseが返る
}



void Patricle_testTest::testTransportInDoubleSphere()
{
    Surface::map_type gsMap {
        std::make_shared<Sphere>("S0", Point{0, 0, 0}, 100),
        std::make_shared<Sphere>("S1", Point{-10, 0, 0}, 20),
        std::make_shared<Sphere>("S2", Point{10, 0, 0}, 20)
    };


    utils::addReverseSurfaces(&gsMap);	// 裏面を作成


    const std::shared_ptr<const Cell> cell1
            = std::make_shared<const Cell>("C1", gsMap,
                                     Lpolynomial(std::vector<Lpolynomial>{Lpolynomial(gsMap.getIndex("-S1")),Lpolynomial(gsMap.getIndex("-S2"))
                                            }
                                      ), 1.0);

    Cell::const_map_type cellMap{{cell1->cellName(), cell1}};

    utils::updateCellSurfaceConnection(cellMap);	// surfaceにcellを登録
    utils::removeUnusedSurfaces(&gsMap);  // ここでS1とS2は削除され、-S1と-S2が残る。
    Cell::initUndefinedCell(gsMap);


    //Particle *p = new Particle (Point{-30, 0, 0}, Vector<3>{-1, 0, 0}, 1.0, cell1);
    Particle *p = new Particle(1.0, Point{-25, 0, 0}, Vector<3>{1, 0, 0}, 1.0, cell1.get(), cellMap);

    //mDebug() << globalSurfaceMap;
    p->moveToCellBound();
    Point expectedPoint1{30, 0, 0};
    //mDebug() << "Particle result pos = " << p->position() << ", expected position =" << expectedPoint1;
    QVERIFY(math::isSamePoint(p->position(), expectedPoint1));
}

void Patricle_testTest::testComplexCell()
{
    Surface::map_type gsMap {
        std::make_shared<Sphere>("S1", Point{-10, 0, 0}, 20),
        std::make_shared<Sphere>("S2", Point{10, 0, 0}, 20),
        std::make_shared<Plane>("P", Vector<3>{1, 1, 0}, 2.0),
        std::make_shared<Plane>("PX1", Vector<3>{1, 0, 0}, -10),
        std::make_shared<Plane>("PX2", Vector<3>{1, 0, 0},  10),
        std::make_shared<Plane>("PX3", Vector<3>{1, 0, 0},  20),
        std::make_shared<Plane>("PY1", Vector<3>{0, 1, 0}, -10),
        std::make_shared<Plane>("PY2", Vector<3>{0, 1, 0},  10),
        std::make_shared<Plane>("PZ1", Vector<3>{0, 0, 1}, -10),
        std::make_shared<Plane>("PZ2", Vector<3>{0, 0, 1},  10)
    };

    utils::addReverseSurfaces(&gsMap);	// 裏面を作成
    const std::shared_ptr<const Cell> cell1
            = std::make_shared<const Cell>("C1", gsMap,
                                     Lpolynomial(std::vector<int>{gsMap.getIndex("-S1"), gsMap.getIndex("-S2"), gsMap.getIndex("P")}), 1.0);
    const std::shared_ptr<const Cell> cell2
            = std::make_shared<const Cell>("C2", gsMap,
                                     Lpolynomial(std::vector<int>{gsMap.getIndex("S1"), gsMap.getIndex("-S2"), gsMap.getIndex("P")}), 1.0);
    Cell::const_map_type cellMap{
        {cell1->cellName(), cell1},
        {cell2->cellName(), cell2}
    };
    utils::updateCellSurfaceConnection(cellMap);	// surfaceにcellを登録
    Cell::initUndefinedCell(gsMap);

    Particle *p = new Particle (1.0, Point{std::sqrt(2.0), std::sqrt(2.0), 0}, Vector<3>{1, 1, 0}, 1.0, cell1.get(), cellMap);

    p->moveToCellBound();
    QCOMPARE(p->currentCell()->cellName(), cell1->cellName());
    p->enterCell();
    QCOMPARE(p->currentCell()->cellName(), cell2->cellName());
    p->moveToCellBound();
    p->enterCell();
}

QTEST_APPLESS_MAIN(Patricle_testTest)

#include "tst_particletest.moc"
