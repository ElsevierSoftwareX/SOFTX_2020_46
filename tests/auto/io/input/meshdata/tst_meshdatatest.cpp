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
#include <QVector>
#include <QtTest>


#include "core/io/input/meshdata.hpp"
#include "core/io/input/dataline.hpp"
#include "core/utils/utils.hpp"

class MeshdataTest : public QObject
{
    Q_OBJECT

public:
    MeshdataTest();

private Q_SLOTS:
    void testConstructor1();
    void testConstructor2();
    void testConstructor3();
    void testFromPhitsList();
};

MeshdataTest::MeshdataTest()
{
}

using namespace inp;
using namespace std;

/*
 * ?-type = 1, 2/3, 4/5 のとりあえず123を実装する。
 *
 */
void MeshdataTest::testConstructor1()
{
	// 直接境界値指定
	vector<double> bds{0.0, 1.0, 2.5, 5.0};
	vector<double> cs{0.5, 1.75, 3.75};
	MeshData mesh1(bds);
	QCOMPARE(mesh1.bounds(), bds);
	QCOMPARE(mesh1.centers(), cs);
	// 境界値が１個とか昇順じゃないのは例外発生
	QVERIFY_EXCEPTION_THROWN(MeshData({1.0}), std::invalid_argument);
	QVERIFY_EXCEPTION_THROWN(MeshData({1.0, 2.0, 1.5}), std::invalid_argument);
}

void MeshdataTest::testConstructor2()
{
    // 上限下限指定
    MeshData mesh2(4, std::make_pair(-0.5, 1.5), MeshData::InterPolate::LIN);
    vector<double> expectedBounds2{-0.5, 0, 0.5, 1, 1.5};
    vector<double> expectedCenters2{-0.25, 0.25, 0.75, 1.25};
    QCOMPARE(mesh2.bounds(), expectedBounds2);
    QCOMPARE(mesh2.centers(),expectedCenters2);
    // 境界値が負の時、対数補間でmesh中心をセットしようとすると例外発生
    QVERIFY_EXCEPTION_THROWN(MeshData(4, std::make_pair(-0.5, 1.5), MeshData::InterPolate::LOG),
                             std::invalid_argument);
}

void MeshdataTest::testConstructor3()
{
    // Lin
    const std::pair<double, double> range(1e+1, 1e+5);
    MeshData mesh3Lin(2, range, MeshData::InterPolate::LIN);
    vector<double> expectedCenters3Lin{0.5*(range.first + 0.5*(range.first + range.second)),
                                        0.5*(0.5*(range.first + range.second) + range.second)};
    vector<double> expectedBounds3Lin{range.first, 0.5*(range.first + range.second), range.second};
    auto resultCenter3Lin = mesh3Lin.centers();
    auto resultBounds3Lin = mesh3Lin.bounds();
    QCOMPARE(resultBounds3Lin.size(), expectedBounds3Lin.size());
    for(size_t i = 0; i < resultBounds3Lin.size(); ++i) {
        QCOMPARE(resultBounds3Lin.at(i), expectedBounds3Lin.at(i));
    }
    for(size_t i = 0; i < resultCenter3Lin.size(); ++i) {
        QCOMPARE(resultCenter3Lin.at(i), expectedCenters3Lin.at(i));
    }

    // log
    MeshData mesh3Log(2, range, MeshData::InterPolate::LOG);
    vector<double> expectedBounds3Log{1e+1,  1e+3,  1e+5};
    vector<double> expectedCenters3Log{1e+2, 1e+4};
    auto resultBounds3Log = mesh3Log.bounds();
    auto resultCenter3Log = mesh3Log.centers();
    // QCOMPARE(vector<double>)は要素差の相対値ではなく絶対値で評価するようで、
    // 浮動小数点数の値が大きくなると必ずfalseになる。
//	mDebug() << "resultC=" << resultCenter3Log;
//	mDebug() << "expected=" << expectedCenters3Log;
    QCOMPARE(resultBounds3Log.size(), expectedBounds3Log.size());
    for(size_t i = 0; i < expectedBounds3Log.size(); ++i) {
        QCOMPARE(resultBounds3Log.at(i), expectedBounds3Log.at(i));
    }
    for(size_t i = 0; i < expectedCenters3Log.size(); ++i) {
        QCOMPARE(resultCenter3Log.at(i), expectedCenters3Log.at(i));
    }
}

template <class T>
void QCompareVec(const std::vector<T> &vec1, const std::vector<T> &vec2)
{
    QCOMPARE(vec1.size(), vec2.size());
    for(size_t i = 0; i < vec1.size(); ++i) {
        QCOMPARE(vec1.at(i), vec2.at(i));
    }
}

void MeshdataTest::testFromPhitsList()
{
    std::list<DataLine> sourceList;
    sourceList.push_back(DataLine("file", 29, " r-type = 1"));
    sourceList.push_back(DataLine("file", 30, " nr = 5"));
    sourceList.push_back(DataLine("file", 31, " 0 1 2  3 4 5   "));
    sourceList.push_back(DataLine("file", 32, " z-type = 2"));
    sourceList.push_back(DataLine("file", 33, " nz = 3"));
    sourceList.push_back(DataLine("file", 34, " zmin = 0.1"));
    sourceList.push_back(DataLine("file", 35, " zmax = 0.4"));
    sourceList.push_back(DataLine("file", 37, " e-type = 3       "));
    sourceList.push_back(DataLine("file", 38, " ne = 3"));
    sourceList.push_back(DataLine("file", 39, "          emin=1e-9"));
    sourceList.push_back(DataLine("file", 40, " emax=1e-3"));

    MeshData rmesh = MeshData::fromPhitsInput(sourceList, MeshData::KIND::R);
    MeshData zmesh = MeshData::fromPhitsInput(sourceList, MeshData::KIND::Z);
    MeshData emesh = MeshData::fromPhitsInput(sourceList, MeshData::KIND::E);

    std::vector<double> rCenterExpected{0.5, 1.5, 2.5, 3.5, 4.5};
    std::vector<double> rBoundsExpected{0, 1, 2, 3, 4, 5};
    std::vector<double> zCenterExpected{0.15, 0.25, 0.35};
    std::vector<double> zBoundsExpected{0.1, 0.2, 0.3, 0.4};
    std::vector<double> eCenterExpected{1e-8, 1e-6, 1e-4};
    std::vector<double> eBoundsExpected{1e-9, 1e-7, 1e-5, 1e-3};

    QCompareVec(rmesh.centers(), rCenterExpected);
    QCompareVec(rmesh.bounds(), rBoundsExpected);
    QCompareVec(zmesh.centers(), zCenterExpected);
    QCompareVec(zmesh.bounds(), zBoundsExpected);
    QCompareVec(emesh.centers(), eCenterExpected);
    QCompareVec(emesh.bounds(), eBoundsExpected);


}



QTEST_APPLESS_MAIN(MeshdataTest)

#include "tst_meshdatatest.moc"
