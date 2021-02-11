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


#include "core/geometry/surface/surface.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"

using namespace geom;

class SurfacecardTest : public QObject
{
	Q_OBJECT

public:
	SurfacecardTest();

private Q_SLOTS:
	void testCosTr();
	void testCase0();
	void testExtract();
	void testSplit();
	void testCard();
	void testCard2();
	void testCard3();
	void testCard4();
};

SurfacecardTest::SurfacecardTest() {;}

void SurfacecardTest::testCosTr()
{
	std::string str = "\"_12_12\" ca -0.3 0.3 0 0 0 1 0.1 *trsf=(.1 {-.9*exp(0)} {abs(-1.1)}, 0 0 0) trsf=(3)";
	auto card = inp::SurfaceCard::fromString(str);
	//mDebug() << "card=" << card.toString();
	std::vector<double> expected{-0.3, 0.3, 0, 0, 0, 1, 0.1};
	std::string expectName = "_12_12", expectSymbol = "ca";
	QCOMPARE(card.name, expectName);
	QCOMPARE(card.symbol, expectSymbol);
	std::string expect = "*.1 {-.9*exp(0)} {abs(-1.1)},*0 0 0,3";
//	mDebug() << "actual=" << card.trStr;
//	mDebug() << "expect=" << expect;
	QCOMPARE(card.trStr, expect);
	QCOMPARE(card.params.size(), expected.size());
	for(size_t i = 0; i < expected.size(); ++i) {
		QVERIFY(std::abs(card.params.at(i) - expected.at(i)) < 1e-10);
	}
}

void SurfacecardTest::testCase0()
{
	std::string str = "\"_12_12\" ca -0.3 0.3 0 0 0 1 0.1 trsf=((.1 -.9 0)) trsf=(3)";
	auto card = inp::SurfaceCard::fromString(str);
	mDebug() << "card=" << card.toString();
	std::vector<double> expected{-0.3, 0.3, 0, 0, 0, 1, 0.1};
	QCOMPARE(card.name, std::string("_12_12"));
	QCOMPARE(card.symbol, std::string("ca"));
	QCOMPARE(card.trStr, std::string(".1 -.9 0,3"));
	QCOMPARE(card.params.size(), expected.size());
	for(size_t i = 0; i < expected.size(); ++i) {
		QVERIFY(std::abs(card.params.at(i) - expected.at(i)) < 1e-10);
	}
	QVERIFY_EXCEPTION_THROWN(inp::SurfaceCard::fromString(str, true), std::invalid_argument);
}

void SurfacecardTest::testExtract()
{
	std::vector<std::string> ex1{"-9999", "-1", "5", "6", "-7", "+8", "abc"};
	std::vector<std::string> ex2{"-1", "-10", "11", "-9999"};
	auto res1 = Surface::extractSurfaceNames(" -9999  -1 #1 #2 #3 5  (6  (-7:+8)) abc");
//	mDebug() << "result=" << res1;
//	mDebug() << "expect=" << ex1;
	QCOMPARE(res1, ex1);
	auto res2 = Surface::extractSurfaceNames("-1 -10 11 -9999");
	QCOMPARE(res2, ex2);
}

void SurfacecardTest::testSplit()
{
//	mDebug() << utils::split(' ', "0 {ab c} {sss}", '{', '}', true);
	std::vector<std::string> expected{"0", "{ab c}", "{sss}"};
	QCOMPARE(utils::splitString(std::make_pair('{', '}')," ", "0 {ab c} {sss}", true), expected);
}

void SurfacecardTest::testCard()
{
	auto card = inp::SurfaceCard::fromString("s1 1  S 0 0 0 {exp(2) + 1}");
	std::vector<double> expected{0, 0, 0, std::exp(2.0)+1};
	QCOMPARE(card.name, std::string("s1"));
	QCOMPARE(card.params.size(), expected.size());
	for(size_t i = 0; i < expected.size(); ++i) {
		QVERIFY(std::abs(card.params.at(i) - expected.at(i)) < 1e-12);
	}
}

void SurfacecardTest::testCard2()
{
	auto card = inp::SurfaceCard::fromString("s1 1  S 0 0 0 {exp(2) + 1} trsf=(10 20 30  1 2 3 4 5 6 7 8 9 0)");
	std::vector<double> expected{0, 0, 0, std::exp(2.0)+1};
	QCOMPARE(card.name, std::string("s1"));
	QCOMPARE(card.params.size(), expected.size());
	for(size_t i = 0; i < expected.size(); ++i) {
		QVERIFY(std::abs(card.params.at(i) - expected.at(i)) < 1e-12);
	}
	QCOMPARE(card.trStr, std::string("10 20 30 1 2 3 4 5 6 7 8 9 0"));
}

void SurfacecardTest::testCard3()
{
	auto card = inp::SurfaceCard::fromString("\"s1_C1<L[0,1,2]<C2\" 1  S 0 0 0 {exp(2) + 1} trsf=(10 20 30  1 2 3 4 5 6 7 8 9 0)", false);
	std::vector<double> expected{0, 0, 0, std::exp(2.0)+1};
	QCOMPARE(card.name, std::string("s1_C1<L[0,1,2]<C2"));
	QCOMPARE(card.params.size(), expected.size());
	for(size_t i = 0; i < expected.size(); ++i) {
		QVERIFY(std::abs(card.params.at(i) - expected.at(i)) < 1e-12);
	}
	QCOMPARE(card.trStr, std::string("10 20 30 1 2 3 4 5 6 7 8 9 0"));
	// ユーザー入力としては不適格
	QVERIFY_EXCEPTION_THROWN(inp::SurfaceCard::fromString("\"s1_C1<L[0,1,2]<C2\" 1  S 0 0 0 {exp(2) + 1}", true), std::invalid_argument);
	// surface名に空白は認められない
	QVERIFY_EXCEPTION_THROWN(inp::SurfaceCard::fromString("\"s1_C1<L[0 1 2]<C2\" 1  S 0 0 0 {exp(2) + 1} trsf=(10)"), std::invalid_argument);
}

void SurfacecardTest::testCard4()
{
	auto card1 = inp::SurfaceCard::fromString("19 sph 10 5 3");
	mDebug() << "card1=" << card1.toString();
	QCOMPARE(card1.symbol, std::string("sph"));
	auto card2 = inp::SurfaceCard::fromString("19 c/z 10 5 3");
	mDebug() << "card2=" << card2.toString();
	QCOMPARE(card2.symbol, std::string("c/z"));
}

QTEST_APPLESS_MAIN(SurfacecardTest)

#include "tst_surfacecardtest.moc"
