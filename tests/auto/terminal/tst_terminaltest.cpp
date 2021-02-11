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



#include "core/io/input/dataline.hpp"
#include "core/io/input/inputdata.hpp"
#include "core/io/input/mcmode.hpp"
#include "core/io/input/phits/transformsection.hpp"
#include "core/geometry/geometry.hpp"
#include "core/math/nmatrix.hpp"
#include "core/math/nvector.hpp"
#include "core/utils/utils.hpp"
#include "core/terminal/interactiveplotter.hpp"
#include "core/option/config.hpp"

#include "core/formula/logical/lpolynomial.hpp"
#include "core/geometry/cell/cell.hpp"
#include "core/geometry/surface/sphere.hpp"
#include "core/geometry/surface/surface.hpp"
#include "core/geometry/surf_utils.hpp"
#include "core/geometry/cell_utils.hpp"
#include "core/simulation.hpp"

class TerminalTest : public QObject
{
	Q_OBJECT

public:
	TerminalTest();

private Q_SLOTS:
    void testTerminal();
    void testCase1();
};

TerminalTest::TerminalTest() {}

void TerminalTest::testTerminal()
{
    term::CustomTerminal term("#");
}

// QT test アプリケーションはgdbで追えないことがあるのでdebug目的には不向きな場合がある。
void TerminalTest::testCase1()
{

    // 面マップの作成
    geom::Surface::map_type sMap {
        std::make_shared<geom::Sphere>("S1", math::Point{0, 0, 0}, 20)
//		{"S99",  std::make_shared<geom::Sphere>("S99", math::Point{0, 0, 0}, 100)},
    };
    utils::addReverseSurfaces(&sMap);

    geom::Cell::const_map_type cellMap{
        {"C1", std::make_shared<const geom::Cell>("C1", sMap,  lg::LogicalExpression<int>(sMap.getIndex("-S1")), 1.0)},
        {"C99", std::make_shared<const geom::Cell>("C99", sMap,  lg::LogicalExpression<int>(sMap.getIndex("S1")), 1.0)},
    };


    std::shared_ptr<const geom::Geometry> geometry = std::make_shared<const geom::Geometry>(sMap, cellMap);
    auto sim = std::make_shared<Simulation>();
    sim->setGeometry(geometry);

    term::InteractivePlotter plotter(sim, 1, false);
//	plotter.start();

}

QTEST_APPLESS_MAIN(TerminalTest)


#include "tst_terminaltest.moc"
