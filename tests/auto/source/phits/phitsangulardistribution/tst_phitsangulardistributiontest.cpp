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
#include "core/source/abstractdistribution.hpp"
#include "core/source/multigroupdistribution.hpp"
#include "core/source/proportionaldistribution.hpp"
#include "core/source/discretedistribution.hpp"
#include "core/source/phits/phitssource.hpp"
#include "core/source/phits/phitsangulardistribution.hpp"
#include "core/utils/utils.hpp"

using namespace inp;
using namespace src;


class PhitsAngulardistributionTest : public QObject
{
	Q_OBJECT

public:
    PhitsAngulardistributionTest();

private:
    std::list<DataLine> angularInput1_;  // 正常データ
    std::list<DataLine> angularInput2_;  // 例外
    std::list<DataLine> angularInput3_;  // 例外発生
private Q_SLOTS:
    void testCase1();
    void testCase2();
    void testCase3();
};

PhitsAngulardistributionTest::PhitsAngulardistributionTest()
{
    angularInput1_.push_back(DataLine("file", 0, "na = 9"));
    angularInput1_.push_back(DataLine("file", 1, "-1.0  0.1"));
    angularInput1_.push_back(DataLine("file", 2, "-0.6  0.2"));
    angularInput1_.push_back(DataLine("file", 3, "-0.5  0.3"));
    angularInput1_.push_back(DataLine("file", 4, "-0.3  0.05"));
    angularInput1_.push_back(DataLine("file", 5, "-0.1  0.1"));
    angularInput1_.push_back(DataLine("file", 6, "0.3  0.11    0.5"));  // 自由形式入力なので1行2データとは限らない。
    angularInput1_.push_back(DataLine("file", 7, "0.111"));
    angularInput1_.push_back(DataLine("file", 8, "0.7  0.2"));
    angularInput1_.push_back(DataLine("file", 9, "0.8  0.21"));
    angularInput1_.push_back(DataLine("file",10, "1.0 "));

    angularInput2_ = angularInput1_;
    angularInput2_.pop_back();
    angularInput2_.pop_back();
    angularInput2_.push_back(DataLine("file", 9, "0.7  0.21"));  // 角度入力が単調増加ではない
    angularInput2_.push_back(DataLine("file",10, "0.9"));

    angularInput3_ = angularInput1_;
    angularInput3_.pop_back();
    angularInput3_.pop_back();
    angularInput3_.pop_back();
    angularInput3_.push_back(DataLine("file", 8, "0.7  -0.2"));
    angularInput3_.push_back(DataLine("file", 9, "0.8  0.21"));
    angularInput3_.push_back(DataLine("file",10, "1.0 "));
}


void PhitsAngulardistributionTest::testCase1()
{
//	for(auto &x: angularInput1_) {
//		mDebug() << x.pos() << x.data;
//	}
    // type 1
    auto itr = angularInput1_.cbegin();
    auto dist1 = src::phits::createPhitsAngularDistribution("1", angularInput1_, itr);

    std::vector<double> expectedAngles{-1.0, -0.6, -0.5, -0.3, -0.1, 0.3,  0.5,  0.7,  0.8, 1.0};
    std::vector<double> expected{          0.1,  0.2,  0.3,  0.05, 0.1, 0.11, 0.111, 0.2, 0.21};
    expected = utils::normalize(expected);
    for(size_t i = 0; i < expected.size(); ++i) {
        expected[i] = expected.at(i)/(expectedAngles.at(i+1) - expectedAngles.at(i));

    }
//	mDebug() << "a-type=1\n" << dist1->toString();
//	mDebug() << "expected pdf=" << expected;
    // 積分範囲が群内の場合
//	mDebug() << "ex1=" << expected.at(1) << "ex=" << expected.at(1)*0.03;
    QCOMPARE(dist1->getProbability(-0.55, 0.01, 0.02), expected.at(1)*0.03);
    QCOMPARE(dist1->getProbability(0, 0.02, 0.01), expected.at(4)*0.03);

    // 積分範囲が群をまたぐ場合
    QCOMPARE(dist1->getProbability(0, 0.2, 0),  expected.at(3)*0.1 + expected.at(4)*0.1);  // 下側またぎ
    QCOMPARE(dist1->getProbability(0, 0, 0.4),  expected.at(4)*0.3 + expected.at(5)*0.1);  // 上側
    QCOMPARE(dist1->getProbability(-0.5, 0.4, 0.6),
             expected[0]*0.3 + expected[1]*0.1 + expected[2]*0.2 + expected[3]*0.2 + expected[4]*0.2);  // 両側

    // 積分範囲が群境界と一致する場合
    QCOMPARE(dist1->getProbability(0, 0, 0.3), expected[4]*0.3);  // 上側一致
    QCOMPARE(dist1->getProbability(0, 0.3, 0), expected[3]*0.2 + expected[4]*0.1);  // 下側一致
    QCOMPARE(dist1->getProbability(-0.5, 0.5, 0.8),
             expected[0]*0.4 + expected[1]*0.1 + expected[2]*0.2 + expected[3]*0.2 + expected[4]*0.4);  // 両側

    // 積分範囲がが定義域全体を含む場合
    double total = 0;
    for(size_t i = 0; i < expected.size(); ++i) {
        total += (expectedAngles.at(i+1) - expectedAngles.at(i))*expected.at(i);
    }
    mDebug() << "total=" << total;
    QCOMPARE(dist1->getProbability(0, 1, 1), total);
}

void PhitsAngulardistributionTest::testCase2()
{
//	for(auto &x: angularInput2_) {
//		mDebug() << x.pos() << x.data;
//	}
    auto itr = angularInput2_.cbegin();
    QVERIFY_EXCEPTION_THROWN(src::phits::createPhitsAngularDistribution("1",  angularInput2_, itr), std::invalid_argument);
    try{
        itr = angularInput2_.cbegin();
        src::phits::createPhitsAngularDistribution("1", angularInput2_, itr);
    }catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}

void PhitsAngulardistributionTest::testCase3()
{
//	for(auto &x: angularInput3_) {
//		mDebug() << x.pos() << x.data;
//	}
    // type1 エラー例外発生テスト
    auto itr = angularInput3_.cbegin();
    QVERIFY_EXCEPTION_THROWN(src::phits::createPhitsAngularDistribution("1",  angularInput3_, itr), std::out_of_range);
    try {
        itr = angularInput3_.cbegin();
        src::phits::createPhitsAngularDistribution("1",  angularInput3_, itr);
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}


QTEST_APPLESS_MAIN(PhitsAngulardistributionTest)

#include "tst_phitsangulardistributiontest.moc"
