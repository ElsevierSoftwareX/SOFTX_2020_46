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
#include "core/tally/pointdetector.hpp"
#include "core/utils/utils.hpp"
#include "core/io/input/phits/phitsinputsection.hpp"
#include "core/io/input/dataline.hpp"


using namespace tal;
using namespace inp;
using namespace phits;

class PointdetectorTest : public QObject
{
	Q_OBJECT
public:
	PointdetectorTest();

private Q_SLOTS:
	void testPds();


private:
    PhitsInputSection* tcrossPoints;
    PhitsInputSection* tcrossRings;
};

PointdetectorTest::PointdetectorTest()
{

    tcrossPoints = new PhitsInputSection("t-point",
                                        std::list<inp::DataLine>
    {
        {"file",  1, "title  = Example of Point Detector"},
        {"file",  2, "  point = 2                        "},
        {"file",  3, "non  x   y    z  r0                "},
        {"file",  4, "1   10   0   50   1                "},
        {"file",  4, "2   20   0   50   1                "},
        {"file",  5, "part =        photon              "},
        {"file",  6, "e-type = 3                         "},
        {"file",  7, " emin = 1.e-5                      "},
        {"file",  8, "emax = 10                          "},
        {"file",  9, "ne   = 60                          "},
        {"file", 10, "axis = eng                         "},
        {"file", 11, "file = point.out                   "},
        {"file", 12, "epsout = 1                         "},
        {"file", 13, "angel = ymin(1.e-9) ymax(1.e-4)    "}
    }, true ,true);


    tcrossRings = new PhitsInputSection("t-point",
                                        std::list<inp::DataLine>
    {
        {"file",  1, "title  = Example of Ring Detector"},
        {"file",  2, "  ring = 2                        "},
        {"file",  3, "axis ar rr r0                    "},
        {"file",  4, "z    50  10  1                     "},
        {"file",  4, "x    10  10.0  0                   "},
        {"file",  5, "part =  photon                     "},
        {"file",  6, "e-type = 3                         "},
        {"file",  7, " emin = 1.e-5                      "},
        {"file",  8, "emax = 10                          "},
        {"file",  9, "ne   = 60                          "},
        {"file", 10, "axis = eng                         "},
        {"file", 11, "file = ring.out                   "},
        {"file", 12, "epsout = 1                         "},
        {"file", 13, "angel = ymin(1.e-9) ymax(1.e-4)    "},
        {"file", 14, "c ! a-type = -2                    "},
        {"file", 15, "c ! na = 10                        "},
        {"file", 16, "c ! amin = 0                       "},
        {"file", 17, "c ! amax = 360                     "}
    }, true, true);

}

void PointdetectorTest::testPds()
{
    PointDetector pd(std::unordered_map<size_t, math::Matrix<4>>(), *tcrossPoints);
    std::vector<math::Point> expectedPoints {
        math::Point{10, 0, 50},
        math::Point{20, 0, 50}
    };
    for(size_t i = 0; i < pd.pointVectors().size(); ++i) {
        QVERIFY(math::isSamePoint(pd.pointVectors().at(i), expectedPoints.at(i)));
    }
}



QTEST_APPLESS_MAIN(PointdetectorTest)

#include "tst_pointdetectortest.moc"
