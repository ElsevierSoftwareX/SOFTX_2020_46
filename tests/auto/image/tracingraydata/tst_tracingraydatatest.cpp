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
#include <vector>

#include "core/utils/utils.hpp"
#include "core/image/tracingraydata.hpp"

using namespace img;

class TracingraydataTest : public QObject
{
	Q_OBJECT

public:
	TracingraydataTest();

private Q_SLOTS:
    void testCase1();
};

TracingraydataTest::TracingraydataTest()
{
    TracingRayData();
}

namespace {
const char undefName[] = "*C_u";
const char undefBdName[] = "*C_ub";
const char bdName[] = "*C_b";

}

void TracingraydataTest::testCase1()
{
    const double WIDTH = 100;
    const size_t HSIZE = 10;
    std::vector<std::string> cells{"C99", "C1", undefName, "C99"};
    std::vector<double> tracks0{22.5045, 54.4955, 0.495454, 22.5045};

    TracingRayData rayData(math::Point{0,0,0},0, cells, tracks0,
                           undefName, undefBdName, bdName);

    double pixWidth = WIDTH/HSIZE;
    std::vector<std::string> results;
    for(size_t i = 0; i < HSIZE; ++i) {
        double xpos = 0.5*pixWidth + i*pixWidth;
        results.push_back(rayData.getCellName(xpos, pixWidth));
    }

    std::vector<std::string> expected {"C99", "C99", bdName, "C1", "C1", "C1", "C1", undefBdName, "C99", "C99"};


    QCOMPARE(results.size(), expected.size());
    for(size_t i = 0; i < results.size(); ++i) {
        mDebug() << i << "result=" << results[i] << "expected=" << expected[i];
        if(expected.at(i) != results.at(i))
            mDebug() << "i=" << i << "result =" << results.at(i) << ", expected=" << expected.at(i);

        QCOMPARE(expected.at(i), results.at(i));
    }

}

QTEST_APPLESS_MAIN(TracingraydataTest)

#include "tst_tracingraydatatest.moc"
