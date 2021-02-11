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

#include <regex>


#include "core/io/input/cellcard.hpp"
#include "core/io/input/filldata.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"

using namespace inp;
using namespace geom;


class CellcardTest : public QObject
{
	Q_OBJECT

public:
	CellcardTest();

private Q_SLOTS:
    void testCase6();
    void testCase5();
    void testCase0();
    void testCase1();
    void testCase2();
    void testCase3();
    void testCase4();
    void testGetSurfNames();
    void testFillParam();
    void testQuotedName();
    void testCosTRCL();
};

CellcardTest::CellcardTest() {;}

void CellcardTest::testCase6()
{
    const std::string str = "17  0   -19 +20 imp:n=1";
    auto card = CellCard::fromString(str);
    QCOMPARE(card.equation, "-19 20");
}


void CellcardTest::testCase5()
{
    const std::string str = "17  0   -19 fill=13 (-8.039 8.989 0) imp:n=1";
    auto card = CellCard::fromString(str);
    QVERIFY(card.parameters.find("fill") != card.parameters.end());
    QCOMPARE(card.parameters.at("fill"), "13(-8.039 8.989 0)");
}


void CellcardTest::testCase0()
{
    auto card1 = CellCard::fromString("1 0    (-1.1 -1.2 -1.3 -1.4 -1.5 -1.6) fill=1 (11 22 33)  u= 2");
    //mDebug() << card1.toString();
    QCOMPARE(card1.equation, std::string("(-1.1 -1.2 -1.3 -1.4 -1.5 -1.6)"));
    QCOMPARE(card1.parameters.at("fill"), std::string("1(11 22 33)"));

    auto card2 = CellCard::fromString("301   2 -1.0  -91:90    u= 3");
    //mDebug() << card2.toString();
    QCOMPARE(card2.equation, std::string("-91:90"));
    QCOMPARE(card2.parameters.at("u"), std::string("3"));

    // univ入力数が足りないと例外発生
    QVERIFY_EXCEPTION_THROWN(CellCard::fromString("101 0 -11 lat=1 fill= -1:1 0:0 0:0 2 2 u=1"), std::invalid_argument);

    auto card4 = CellCard::fromString("0       1 -2 -4 fill=1 (-6 -6.5 0) imp:p=1");
    //mDebug() << card4.toString();
    QCOMPARE(card4.equation, std::string("-4"));
    QCOMPARE(card4.parameters.at("fill"), std::string("1(-6 -6.5 0)"));

    auto card5 = CellCard::fromString("7 3 -2.7 -11 12 -13 14 -15 16 u=2 lat=1 imp:p=1 fill=0:0 0:0 0:0 3");
    //mDebug() << card5.toString();
    QCOMPARE(card5.equation, std::string("-11 12 -13 14 -15 16"));
    QCOMPARE(card5.parameters.at("fill"), std::string("0:0 0:0 0:0  3"));

    auto card6 = CellCard::fromString("4 0       -21 *fill=2 (2 4 0) imp:p=1");
    //mDebug() << card6.toString();
    QCOMPARE(card6.equation, std::string("-21"));
    QCOMPARE(card6.parameters.at("fill"), std::string("2(*2 4 0)"));
}

void CellcardTest::testCase1()
{
    // セルパラメータはジオメトリ記述のあと、最後に来ることになっている(see manual 1-5)
    auto card = CellCard::fromString("C1 like 1 but trcl=(0 0 0) vol = 1 ");
    //mDebug() << "params===" << card.parameters;
    QVERIFY(card.parameters.find("vol") != card.parameters.end());
    QVERIFY(!card.trcl.empty());
    QCOMPARE(card.trcl, std::string("0 0 0"));
    QVERIFY(card.equation.empty());
    std::vector<std::string> dep_expect{"1"};
    QCOMPARE(card.depends, dep_expect);

    // セルパラメータは末尾に来るべきなので、そうでない場合は例外発生にする。
    // material名にasciiを許容しているのでパラメータ名として予約されているワードと比較して例外を出す。
//	auto excard = CellCard::fromString("C1 vol = 1 like 1 but trcl=(0 0 0)");
//	mDebug() << "Excard===" << excard.toInputString();
    QVERIFY_EXCEPTION_THROWN(CellCard::fromString("C1 vol = 1 like 1 but trcl=(0 0 0)", true), std::invalid_argument);
}

void CellcardTest::testCase2()
{
    std::string str1 = "1 1 -0.1  -10000.1 -10000.2 -10000.3 -10000.4 -10000.5 -10000.6 trcl=(-10 -10 0)";
    auto card = CellCard::fromString(str1);
//	mDebug() << "cardeq=" << card.equation;
//	mDebug() << "parm=" << card.parameters;
    QCOMPARE(card.trcl, std::string("-10 -10 0"));
}

void CellcardTest::testCase3()
{
    std::regex re(R"(([ :(]|^)([-+\d\w]+)([ :)]|$))");
    std::smatch sm;
    std::string eq = "-2";
//	if(std::regex_search(eq, sm, re)) {
//		for(size_t i = 0 ; i < sm.size(); ++i) {
//			mDebug() << "i=" << i << "sm=" << sm.str(i);
//		}
//	}
}

void CellcardTest::testCase4()
{
    auto card = CellCard::fromString("  1   0        -1  -2  fill= 123 u=2");
    //mDebug() << "test4 card=" << card.toString();
}


void CellcardTest::testGetSurfNames()
{
    std::string teststr("-1 2 3 (4:5) ((-6:7 (8 #9)))");
    std::set<std::string> expected{"1", "2", "3", "4", "5", "6", "7", "8"};
    auto result = GetSurfaceNames(teststr);
//	for(auto &str: result) {
//		mDebug() << "surface=" << str;
//	}
    QCOMPARE(result.size(), expected.size());
    QCOMPARE(result, expected);
}

void CellcardTest::testFillParam()
{
    auto card = CellCard::fromString("101   0   -10.1 -10.2 -10.3 -10.4 LAT=1 fill= 0:1 -1:0  0:0  12 34  32 21 U=1");
    QCOMPARE(card.parameters.at("fill"), std::string("0:1 -1:0 0:0  12 34 32 21"));

    auto card2 = CellCard::fromString("101   0   -10.1 -10.2 -10.3 -10.4 LAT=1 fill= 0:1 -1:0  0:0  2(10 -20 30) 33  3(7 7 7) 20   U=1");
    //mDebug() << card2.toString();
    QCOMPARE(card2.parameters.at("fill"), std::string("0:1 -1:0 0:0  2(10 -20 30) 33 3(7 7 7) 20"));

    auto card3 = CellCard::fromString("101 0 -10 LAT=1 fill= 0:10 -1:0  0:0 "
                                       " 1 2 3 4 5 6 7 8 9 10 11"
                                      " 11 10 9 8 7 6 5 4 3 2 1" " U=1");
    //mDebug() << card3.toString();
    QCOMPARE(card3.parameters.at("fill"), std::string("0:10 -1:0 0:0  1 2 3 4 5 6 7 8 9 10 11 11 10 9 8 7 6 5 4 3 2 1"));
}

void CellcardTest::testQuotedName()
{
    auto card1 = CellCard::fromString("C1 0   -S1");
//	mDebug() << "result=" << card1.name;
//	mDebug() << "expect=" << "C1";
    QCOMPARE(card1.name, std::string("C1"));

    auto card2 = CellCard::fromString("C1 M1 {1.2*00001}   -S1");
//	mDebug() << "result=" << card2.name;
//	mDebug() << "expect=" << "C1";
    QCOMPARE(card2.name, std::string("C1"));

    // 今の仕様ではクオートでくくろうが空白を認めないことにした。
//	auto card3 = CellCard::fromString("\"C1<L[1 0 1]<2\"   \"M 1\"   {1.2*00001}   -S1");
//	mDebug() << "result=" << card3.name;
//	mDebug() << "expect=" << "C1<L[1 0 1]<2";
//	QCOMPARE(card3.name, std::string("C1<L[1 0 1]<2"));
    QVERIFY_EXCEPTION_THROWN(CellCard::fromString("\"C1<L[1 0 1]<2\"   \"M 1\"   {1.2*00001}   -S1"), std::invalid_argument);

    auto card4 = CellCard::fromString("11 2 -18   #8 #9 #10 imp:n=1 u=1");
//	mDebug() << "result=" << card4.name;
//	mDebug() << "expect=" << "11";
    QCOMPARE(card4.name, std::string("11"));
    QCOMPARE(card4.equation, std::string("#8 #9 #10"));
}

void CellcardTest::testCosTRCL()
{
    std::string str = "c1  m1  -1.0  -10  vol=1 trcl=(1 1 1) *trcl=(0 0 0  0 -90 -90  90 0 -90  90 {45 + 45} 0, 2 2 2)";
    auto card1 = CellCard::fromString(str);
//	mDebug() << "expect ===" << "1 1 1,*0 0 0 0 -90 -90 90 0 -90 90 {45 + 45} 0,*2 2 2";
//	mDebug() << "result ===" << card1.trcl;
    QCOMPARE(card1.trcl, std::string("1 1 1,*0 0 0 0 -90 -90 90 0 -90 90 {45 + 45} 0,*2 2 2"));

    auto card2 = CellCard::fromString("1 0    (-1.1 -1.2 -1.3 -1.4 -1.5 -1.6) *fill=1 ({abs(-1.0)} {exp(0)} 33)  u= 2");
    //mDebug() << card2.toString();
    QCOMPARE(card2.equation, std::string("(-1.1 -1.2 -1.3 -1.4 -1.5 -1.6)"));
    QCOMPARE(card2.parameters.at("fill"), std::string("1(*{abs(-1.0)} {exp(0)} 33)"));




    auto card3 =CellCard::fromString("101   0   -1 -2 -3 -4 "
                                     " fill= 0:0 0:0  0:1  2  (*{10+10} {-20+1}  {30*abs(0.1)})      12    u=98 lat=1 ");
    //mDebug() << card3.toString();
    QCOMPARE(card3.equation, std::string("-1 -2 -3 -4"));

    std::string expectedfillarg3 = "0:0 0:0 0:1  2(*{10+10} {-20+1} {30*abs(0.1)}) 12";
//	mDebug() << "expectedfillarg=" << expectedfillarg3;
//	mDebug() << "resultfillarg  =" << card3.parameters.at("fill");
    QCOMPARE(card3.parameters.at("fill"), expectedfillarg3);
    QCOMPARE(card3.parameters.at("u"), std::string("98"));
    QCOMPARE(card3.parameters.at("lat"), std::string("1"));

    /*
     * DimensionDeclaratorでは4つのunivでfillされており、
     * [0,-1]はuniv2にTR(20, -19, 30*exp(0.1))を適用したもの
     * [0,0]はuniv31
     * [1,-1]はuniv3にTR(7,7,7)を適用したもの
     * [1,0]はuniv12
     * で充填される。TRは何れも角度をcos指定としているが回転変換は無いのであくまでテスト用の冗長表現
     */

    auto card4 = CellCard::fromString("101   0   -10.1 -10.2 -10.3 -10.4 LAT=1 "
                                      " *fill= 0:1 -1:0  0:0  2  (*{10+10} {-20+1}  {30*abs(0.1)})  31 3(*7 7 7)     12    u=98");
    std::string expectedfillarg = "0:1 -1:0 0:0  2(*{10+10} {-20+1} {30*abs(0.1)}) 31 3(*7 7 7) 12";
//	mDebug() << card4.toString();
//	mDebug() << "expectedfillarg=" << expectedfillarg;
//	mDebug() << "result         =" << card4.parameters.at("fill");
//	mDebug() << "Card4 eq=" << card4.equation;
    QCOMPARE(card4.equation, std::string("-10.1 -10.2 -10.3 -10.4"));
    QCOMPARE(card4.parameters.at("fill"), expectedfillarg);
    QCOMPARE(card4.parameters.at("u"), std::string("98"));

    // Exception
    std::string invalidcardstr = "101   0   -10.1 -10.2 -10.3 -10.4 LAT=1 "
                                 " *fill= 0:1 -1:0  0:0  2  (*{10+10} {-20+1}"
                                 "  {30*exp(0.1)}) 3(*7 7 7)     12    u=98";
    QVERIFY_EXCEPTION_THROWN(CellCard::fromString(invalidcardstr), std::invalid_argument);
}



QTEST_APPLESS_MAIN(CellcardTest)

#include "tst_cellcardtest.moc"
