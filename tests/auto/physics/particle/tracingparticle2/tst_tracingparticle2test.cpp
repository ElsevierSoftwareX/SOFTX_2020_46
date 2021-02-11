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

#include <algorithm>
#include <string>
#include <unordered_map>


#include "../../../../../core/physics/particle/tracingparticle.hpp"
#include "../../../../../core/math/nvector.hpp"
#include "../../../../../core/geometry/surface/sphere.hpp"
#include "../../../../../core/geometry/surface/plane.hpp"
#include "../../../../../core/geometry/surface/torus.hpp"
#include "../../../../../core/geometry/surfacecreator.hpp"
#include "../../../../../core/geometry/cellcreator.hpp"
#include "../../../../../core/geometry/cell/cell.hpp"
#include "../../../../../core/formula/logical/lpolynomial.hpp"
#include "../../../../../core/utils/utils.hpp"
#include "../../../../../core/utils/numeric_utils.hpp"

#include "../../../../../core/image/bitmapimage.hpp"
#include "../../../../../core/image/tracingraydata.hpp"
#include "../../../../../core/image/xpmcolors.hpp"
#include "../../../../../core/image/color.hpp"

using namespace phys;
using namespace geom;
using namespace math;
using namespace img;
using namespace lg;


typedef lg::LogicalExpression<int> LPolynomial;

#define DBG std::cout << __FILE__ << ":" << __LINE__ << "!!!!!!!!!!!!!!!!!" << std::endl;

class Tracingparticle2Test : public QObject
{
	Q_OBJECT

public:
	Tracingparticle2Test();


private Q_SLOTS:
    void testTorusTangent();
    void testPlaneSection();
    void testHorizontalLine();
    void testSphereImage();
};

Tracingparticle2Test::Tracingparticle2Test() {}

void Tracingparticle2Test::testTorusTangent()
{

    SurfaceCreator screator(Surface::map_type{
        std::make_shared<Torus>("tor", Point{0, 0, 0}, math::Vector<3>{0, 1, 0}, 10, 3, 3)
    });

    auto sMap = screator.map();
    auto cell1 = std::make_shared<const Cell>("Ctorus",  screator.map(), LPolynomial(std::vector<int>{sMap.getIndex("-tor")}), 1.0);
    auto cell9 = std::make_shared<const Cell>("Cvoid", screator.map(), LPolynomial(std::vector<int>{sMap.getIndex( "tor")}), 1.0);
    Cell::const_map_type cellMap{
        {cell1->cellName(), cell1},
        {cell9->cellName(), cell9},
    };
    std::vector<std::pair<std::string, std::string>> cellMatList;
    cellMatList.push_back(std::make_pair("Ctorus", "m1"));
    cellMatList.push_back(std::make_pair("Cvoid", "m9"));

    CellCreator ccreator(cellMap);
    screator.removeUnusedSurfaces(false);
    ccreator.initUndefinedCell(screator.map());

    /*
     * 大半径10cm、小半径3、軸方向=y軸方向 のトーラスとの交点をテスト。
     * y=3の位置でx方向に粒子走査するとエラーの生じる場合がある。
     *
     * FIXME トーラスと接する軌道で入射した場合の処理がおかしい。
     * 1．通過セル数は奇数[1(トーラスに入射しない)、3(トーラスをかする)、5(トーラス中心穴部を通過)]
     *   になるはずだがそうでない場合がある。
     *
     * 微妙に掠り判定してトーラス内に入った後、次の面が見つからないためと思われる。
     *
     * 掠るとしたらトーラス大半径位置±誤差 (x^2 + z^2 = 100上)くらいのはずである
     * →これは合っている
     */
//	// ここからテスト開始
//	double zmin = -15, zmax = 15;
//	int nmax = 300;
//	double dz = (zmax - zmin)/nmax;
//	for(int i = 0; i < nmax; ++i) {
//		Point start = Point{-50, 3, zmin + dz*i};
//		//mDebug() << "tracing start at " << start;
//		TracingParticle p(1.0, start, Vector<3>{1, 0, 0}, 1.0, nullptr, ccreator.cells(), 100, true, true);
//		p.trace();
//		mDebug() << p.trackLengths();
//		mDebug() << p.passedCells();
//		// トーラスを横切る場合通過セル数は奇数になるはず
//		if(p.trackLengths().size()%2 != 1) mDebug() << "i===" << i << "z===" << zmin+dz*i;
//		QVERIFY(p.trackLengths().size()%2 == 1);
//	}

    /*
     * 最初の交点位置はちゃんとx^2 + z^2 = 100上にある。
     *
     * 考えられるケースは2通り、
     * 1．かすってトーラス内に入ったあと次の交点がみつからない
     * 2．トーラスとの交点が見つかったが、セル内に入ったと思ったら飛び越えてトーラス外に出ていた。
     */
    Point start = Point{-50, 3, -9.6};
    //mDebug() << "tracing start at " << start;
    TracingParticle p(1.0, start, Vector<3>{1, 0, 0}, 1.0, nullptr, ccreator.cells(), 100, true, true);
    p.trace();
    mDebug() << p.trackLengths();
    mDebug() << p.passedCells();
    QVERIFY(p.trackLengths().size()%2 == 1); // トーラスを横切る場合通過セル数は奇数になるはず

}

void Tracingparticle2Test::testPlaneSection()
{
    // 面構築
    SurfaceCreator screator(Surface::map_type {
        std::make_shared<Sphere>("S1", Point{0, 0, 0}, 40),
        std::make_shared<Plane>("P1", Vector<3>{1, 0, 0}, 20)
    });
    auto sMap = screator.map();  // 裏面作成などを済ます
    // セル構築
    std::shared_ptr<const Cell> cell1 = std::make_shared<const Cell>("C1", screator.map(), LPolynomial(std::vector<int>{sMap.getIndex("-S1"), sMap.getIndex("-P1")}), 1.0);
    auto cell2 = std::make_shared<const Cell>("C2", screator.map(), LPolynomial(std::vector<int>{sMap.getIndex("-S1"), sMap.getIndex("P1")}), 1.0);
    auto cell99 = std::make_shared<const Cell>("C99", screator.map(), LPolynomial(std::vector<int>{sMap.getIndex("S1")}), 1.0);
    Cell::const_map_type cellMap{
        {cell1->cellName(), cell1},
        {cell2->cellName(), cell2},
        {cell99->cellName(), cell99}
    };
    std::vector<std::pair<std::string, std::string>> cellMatList;
    cellMatList.push_back(std::make_pair("C1", "m1"));
    cellMatList.push_back(std::make_pair("C2", "m99"));
    cellMatList.push_back(std::make_pair("C99", "m99"));

    CellCreator ccreator(cellMap);
    screator.removeUnusedSurfaces(false);
    ccreator.initUndefinedCell(screator.map());

    // 点のセル内外チェック
    QVERIFY(cell1->isInside(Point{-20, 0, 0}));

    // ここからテスト開始
    Point start = Point{-50, 0, 0};
    //mDebug() << "tracing start at " << start;
    TracingParticle p(1.0, start, Vector<3>{1, 0, 0}, 1.0, nullptr, ccreator.cells(), 50, true, true);
    p.trace();

    // 正解値
    double totalLength = 0;
    std::vector<double> expect_lens{10, 40};
    std::vector<std::string> expect_cnames{"C99", "C1"};

    // tracklengthのチェック
    QCOMPARE(expect_lens.size(), p.trackLengths().size());
    for(size_t i = 0; i < expect_lens.size(); ++i) {
        QVERIFY(utils::isSameDouble(p.trackLengths().at(i), expect_lens.at(i)));
        totalLength += p.trackLengths().at(i);
    }
    QVERIFY(utils::isSameDouble(totalLength, std::accumulate(expect_lens.cbegin(), expect_lens.cend(), 0)));
    // セル名チェック
    QCOMPARE(expect_cnames.size(), p.passedCells().size());
    for(size_t i = 0; i < expect_cnames.size(); ++i) {
        mDebug() << "result ===" << p.passedCells().at(i);
        mDebug() << "expect ===" << expect_cnames.at(i);
        QCOMPARE(p.passedCells().at(i), expect_cnames.at(i));
    }
}



void Tracingparticle2Test::testHorizontalLine()
{
    SurfaceCreator screator(Surface::map_type{
        std::make_shared<Sphere>("S1", Point{0, 0, 0}, 30),
        std::make_shared<Plane>("P", Vector<3>{1, 0, 0}, 0),
        std::make_shared<Sphere>("S2", Point{0, -15, 0}, 30),
        std::make_shared<Sphere>("S99", Point{0, 0, 0}, 1000)
    });
    auto sMap = screator.map();  // 裏面作成などを済ます
    auto cell1 = std::make_shared<const Cell>("C1", screator.map(), LPolynomial(std::vector<int>{sMap.getIndex("-S1"), sMap.getIndex("-P")}), 1.0);
    auto cell2 = std::make_shared<const Cell>("C2", screator.map(), LPolynomial(std::vector<int>{sMap.getIndex("-S1"), sMap.getIndex("P")}), 1.0);
    auto cell99 = std::make_shared<const Cell>("C99", screator.map(), LPolynomial(std::vector<int>{sMap.getIndex("S1"), sMap.getIndex("-S99")}), 1.0);

// NG
//	cell1 = std::make_shared<const Cell>("C1", surfObjects.map(), Polynominal{Term{sMap.getIndex("-S1"), sMap.getIndex("-P")},
//																			  Term{sMap.getIndex("-S2"), sMap.getIndex("-P")}});
//	cell2 = std::make_shared<const Cell>("C99", surfObjects.map(), Polynominal{Term{sMap.getIndex("S1"), sMap.getIndex("S2"),
//																					sMap.getIndex("-S99")}});
    Cell::const_map_type cellMap{
        {cell1->cellName(), cell1},
        {cell2->cellName(), cell2},
        {cell99->cellName(), cell99}
    };
    std::vector<std::pair<std::string, std::string>> cellMatList;
    cellMatList.push_back(std::make_pair("C1", "m1"));
    cellMatList.push_back(std::make_pair("C2", "m99"));
    cellMatList.push_back(std::make_pair("C99", "m99"));

    CellCreator ccreator(cellMap);
    screator.removeUnusedSurfaces(false);
    ccreator.initUndefinedCell(screator.map());

    //mDebug() << "undefinedセルの情報：" << geom::Cell::UNDEFINED_CELL_PTR()->toString();
    //mDebug() << "eps=" << math::EPS << "delta=" << math::Point::delta();
    Point start = Point{-50, 0, 0};
    //mDebug() << "tracing start at " << start;
    TracingParticle p(1.0, start, Vector<3>{1, 0, 0}, 1.0, nullptr, ccreator.cells(), 100, true, true);
    p.trace();
    double totalLength = 0;
    for(auto &elem: p.trackLengths()) {
        totalLength += elem;
    }


    mDebug() << "start=" << start << ", Diff=" << totalLength - 100.0 << ", cells=" << p.passedCells() << ", tls=" << p.trackLengths();
    std::vector<double> diffVec{p.trackLengths().at(0)-20, p.trackLengths().at(1)-30, p.trackLengths().at(2)-30, p.trackLengths().at(3)-20};
    mDebug() << "diffvec=" << diffVec;
    //p.dumpEvents(std::cout);
    QVERIFY(std::abs(totalLength - 100) < math::EPS);
}



void Tracingparticle2Test::testSphereImage()
{
    SurfaceCreator screator(Surface::map_type{
        std::make_shared<Sphere>("S1", Point{0, 0, 0}, 30),
        std::make_shared<Plane>("P", Vector<3>{1, 0, 0}, 0),
        std::make_shared<Sphere>("S2", Point{0, -15, 0}, 30),
        std::make_shared<Sphere>("S99", Point{0, 0, 0}, 1000)
    });

    auto sMap = screator.map();

// OK
//	cell1 = std::make_shared<const Cell>("C1", screator.map(), Polynominal{Term{sMap.getIndex("-S1")}});
//	cell2 = std::make_shared<const Cell>("C99", screator.map(), Polynominal{Term{sMap.getIndex("S1")}});

// OK
//	cell1 = std::make_shared<const Cell>("C1", screator.map(), Polynominal{Term{sMap.getIndex("-P")}});
//	cell2 = std::make_shared<const Cell>("C99", screator.map(), Polynominal{Term{sMap.getIndex("P")}});


// NG
    auto cell1 = std::make_shared<const Cell>("C1", screator.map(), LPolynomial(std::vector<int>{sMap.getIndex("-S1"), sMap.getIndex("-P")}), 1.0);
    auto cell2 = std::make_shared<const Cell>("C2", screator.map(), LPolynomial(std::vector<int>{sMap.getIndex("-S1"), sMap.getIndex("P")}), 1.0);
    auto cell99 = std::make_shared<const Cell>("C99", screator.map(), LPolynomial(std::vector<int>{sMap.getIndex("S1"), sMap.getIndex("-S99")}), 1.0);

// NG
//	cell1 = std::make_shared<const Cell>("C1", surfObjects.map(), Polynominal{Term{sMap.getIndex("-S1"), sMap.getIndex("-P")},
//																			  Term{sMap.getIndex("-S2"), sMap.getIndex("-P")}});
//	cell2 = std::make_shared<const Cell>("C99", surfObjects.map(), Polynominal{Term{sMap.getIndex("S1"), sMap.getIndex("S2"),
//																					sMap.getIndex("-S99")}});
    Cell::const_map_type cellMap{
        {cell1->cellName(), cell1},
        {cell2->cellName(), cell2},
        {cell99->cellName(), cell99}
    };
    std::vector<std::pair<std::string, std::string>> cellMatList;
    cellMatList.push_back(std::make_pair("C1", "m1"));
    cellMatList.push_back(std::make_pair("C2", "m99"));
    cellMatList.push_back(std::make_pair("C99", "m99"));
    img::CellColorPalette palette;
    for(size_t i = 0; i < cellMatList.size(); ++i) {
        palette.registerColor(cellMatList.at(i).first, cellMatList.at(i).second, img::Color::getDefaultColor(i));
    }

    CellCreator ccreator(cellMap);
    screator.removeUnusedSurfaces(true);
    ccreator.initUndefinedCell(screator.map());

    mDebug() << "undefinedセルの情報：" << geom::Cell::UNDEFINED_CELL_PTR()->toString();


    const double WIDTH = 100, HEIGHT = 100;
    const size_t HSIZE = 100, VSIZE = 100;
//	const size_t HSIZE = 1000, VSIZE = 1000;

    double E = 1.0;
    const Vector<3> XDIR{1, 0, 0}, YDIR{0, 1, 0};


    Point ORIGIN = Point{-0.5*WIDTH, -0.5*HEIGHT, 0};
    mDebug() << "image size in cm w,h=" << WIDTH << "," << HEIGHT;
    mDebug() << "eps=" << math::EPS << "delta=" << math::Point::delta();

    //size_t numAdded = 0;
    Point deltaY{0, HEIGHT/VSIZE, 0};
    std::vector<double> hRayLength;
    std::vector<img::TracingRayData> hRayVector;
    for(size_t i = 0; i < VSIZE; ++i) {
        Point start = ORIGIN + i*deltaY;
        //mDebug() << "tracing start at " << start;
        TracingParticle p(1.0, start, XDIR, E, nullptr, ccreator.cells(), WIDTH);
        p.trace();
        double totalLength = 0;
        for(auto &elem: p.trackLengths()) {
            totalLength += elem;
        }
        hRayLength.push_back(totalLength);
        hRayVector.push_back(img::TracingRayData(start, i, p.passedCells(), p.trackLengths(),
                                                 geom::Cell::UNDEF_CELL_NAME, geom::Cell::UBOUND_CELL_NAME, geom::Cell::BOUND_CELL_NAME));
    }
    img::BitmapImage(img::DIR::H, HSIZE, VSIZE, WIDTH, HEIGHT, hRayVector, palette).exportToXpmFile("sphereh.xpm");

    for(size_t i = 0; i < VSIZE; ++i) {
        //mDebug() << "i=" << i << "Diff=" << hRayLength.at(i) - WIDTH;
        QVERIFY(std::abs(hRayLength.at(i) - WIDTH) < math::EPS);
    }



    std::vector<img::TracingRayData> vRayVector;
    Point deltaX{WIDTH/HSIZE, 0, 0};
    for(size_t j = 0; j < HSIZE; ++j) {
        Point start = ORIGIN + j*deltaX;
        //mDebug() << "tracing start at" << start;
        TracingParticle p(1.0, start, YDIR, E, nullptr, ccreator.cells(), WIDTH);
        p.trace();
        auto lengths = p.trackLengths();
        double totalLength = 0;
        for(auto &elem: lengths) {
            totalLength += elem;
        }

        //QCOMPARE(totalLength, WIDTH);
        vRayVector.push_back(img::TracingRayData(start, j, p.passedCells(), p.trackLengths(),
                                                 geom::Cell::UNDEF_CELL_NAME, geom::Cell::UBOUND_CELL_NAME, geom::Cell::BOUND_CELL_NAME));
    }

    img::BitmapImage(img::DIR::V, HSIZE, VSIZE, WIDTH, HEIGHT, vRayVector,  palette).exportToXpmFile("spherev.xpm");
    img::BitmapImage::merge(img::BitmapImage(img::DIR::V, HSIZE, VSIZE, WIDTH, HEIGHT, vRayVector,  palette),
                           img::BitmapImage(img::DIR::H,HSIZE, VSIZE, WIDTH, HEIGHT, hRayVector,  palette),
                            std::vector<std::string>{geom::Cell::UBOUND_CELL_NAME, geom::Cell::BOUND_CELL_NAME},
                            geom::Cell::DOUBLE_CELL_NAME)
            .exportToXpmFile("sphere.xpm");
}

QTEST_APPLESS_MAIN(Tracingparticle2Test)

#include "tst_tracingparticle2test.moc"
