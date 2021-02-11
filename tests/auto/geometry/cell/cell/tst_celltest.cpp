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


#include "core/formula/logical/lpolynomial.hpp"
#include "core/geometry/cell/cell.hpp"
#include "core/geometry/surf_utils.hpp"
#include "core/geometry/surface/sphere.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surface/quadric.hpp"
#include "core/utils/message.hpp"


using namespace geom;
using namespace math;


class Cell_testTest : public QObject
{
	Q_OBJECT

public:
	Cell_testTest();
private:
    Surface::map_type smap;  // globalなsurfacemapを作成

private Q_SLOTS:
    void testEmptyCell();
    void testCase1();
    void testCase2();
};

Cell_testTest::Cell_testTest()
{
    // surfaceの作成
    std::shared_ptr<Surface> s1 = std::make_shared<Sphere>("s1", Point{0, 0, 0}, 20);          // id=1
    std::shared_ptr<Surface> s2 = std::make_shared<Sphere>("s2", Point{10, 0, 0}, 20);         // id = 2
    std::shared_ptr<Surface> s3 = std::make_shared<Sphere>("s3", Point{-10, 0, 0}, 20);        // id = 3
    std::shared_ptr<Surface> pzm10 = std::make_shared<Plane>("pzm10", Vector<3>{0, 0, 1}, -5); // id = 4
    const double X=1, radius = 4;
    const std::vector<double> params{1, 1, 1, 0, 0, 0, -radius*radius, X, 0, 0};

    std::shared_ptr<Surface> sq1 = Quadric::createQuadric("sq1", params, Matrix<4>(), Quadric::TYPE::SQ, false);


    mDebug() << s1->getID() << s2->getID() << s3->getID() << pzm10->getID();
    // map_type は std::unordered_map<name_type, std::shared_ptr<Surface>>
    smap.registerSurface(s1->getID(), s1);
    smap.registerSurface(s2->getID(), s2);
    smap.registerSurface(s3->getID(), s3);
    smap.registerSurface(pzm10->getID(), pzm10);
    smap.registerSurface(sq1->getID(), sq1);
    /*
     * -sを+sと表裏逆の別surfaceとして扱う場合、std::cerr << "Surface Constructor!!! name=" << name << std::endl;
     * cell定義に-sが使われていた場合隣接surfaceには
     * "-s"だけでなく"s"も自動的に生成してsurfaceマップに加える必要がある。
     * メモリ使用量は増えるが仕方ない。
     */
    utils::addReverseSurfaces(&smap);
}

void Cell_testTest::testEmptyCell()
{

}

void Cell_testTest::testCase1()
{
    using namespace std;
    // 論理式の作成
    auto poly = lg::LogicalExpression<int>::fromString("-s2 -s3 pzm10", smap.nameIndexMap());
    // (10, 0, 0)の半径20cmの球(s2)内側かつ、(-10, 0, 0)の半径20cmの球(s3)内部かつ、 z=-5より上の部分
    Cell cell("testcell", smap, poly, 1.0);
    //mDebug() << "SMAP=\n" << smap;
    QVERIFY(cell.isInside(Point{0, 0, 0})  == true);  // 原点は当然内側。
    QVERIFY(cell.isInside(Point{11, 0, 0})  == false); // x=11は球の外に出るのでfalse
    QVERIFY(cell.isInside(Point{0, 0, -11}) == false); // z=-11は外

    /*
     * cellは -s2で定義されているので -s2のforwardならセル内側、-s2のbackwardならセル外になる。
     * (-10, 0, 0)は-s2の境界上なので「裏面境界上はbackward」ルールでbackward扱いになり、
     * 結果はセル外となる。
     * しかし境界直上の扱いは演算が入ると誤差でほぼ不定になることに注意。
     * 要は一貫した解釈によって未定義空間が生じないことが重要。
     */
    //QVERIFY(cell.isInside(math::Point{-10, 0, 0})== false);
}

void Cell_testTest::testCase2()
{
    auto poly = lg::LogicalExpression<int>::fromString("-sq1", smap.nameIndexMap());
    // SQで定義した中心(1, 0, 0), 半径4の球
    Cell cell("testcell", smap, poly, 1.0);
//	QVERIFY( cell.isInside(Point{0, 0, 0}));
//	QVERIFY(!cell.isInside(Point{-10, 0, 0}));
    QVERIFY(!cell.isInside(Point{-3.5, 0, 0}));
    QVERIFY(cell.isInside(Point{4.9, 0, 0}));
}

QTEST_APPLESS_MAIN(Cell_testTest)

#include "tst_celltest.moc"
