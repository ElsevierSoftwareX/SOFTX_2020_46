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
#include <QMetaObject>

#include "core/io/input/cellparameter.hpp"
#include "core/io/input/ijmr.hpp"
#include "core/utils/message.hpp"

using namespace std;

class CellparameterTest : public QObject
{
	Q_OBJECT

public:
	CellparameterTest();

private Q_SLOTS:
	void testIsCellParam_data();
	void testIsCellParam();
	void testCellTrString_data();
	void testCellTrString();
};

//
Q_DECLARE_METATYPE(string);
Q_DECLARE_METATYPE(bool);


CellparameterTest::CellparameterTest() {}


/*
 *  問題は 番号とparticle designatorがあるのとないのと。 xxxN:MのNとM
 *  番号を持つものは番号を省略可能なので要注意
 * ・両方ないもの  : u, trcl, fill, lat, nonu pwt vol rho mat
 * ・両方あるもの  : dxc wwn
 * ・designatorのみ: ext fcl imp
 * ・番号のみ      : pd tmp
 *
 * パラメータ引数は
 * u:  u= 1個 (u=1)
 * trcl: = 可変長 (trcl=2  trcl=(cos(0) 2r 9j))
 * fill: 可変長
 * tmp: 可変 (tmp1=100, tmp 100 2m)
 * wwn: 可変 (wwn1:N=0.5, wwn:N 0.5 4r)
 * lat 1個(123のどれか)
 * dxc: 1個 (dxc1:n=1) みたいな感じ。
 * ext: 1個 (ext:n=1)
 * fcl: 1個 (fcl:n=1)
 * imp: 1個 (imp:n=1)
 * nonu 0個
 * pd:  1個 (pd1=1)
 * pwt: 1個 (pwt=1)
 * vol: 1個 (vol=10)
 * rho: 1個 (row=0.5)
 * mat: 1個 (mat=hydrogen)
 *
 * ※ trcl,fillは ijmr展開の対象となる。
 */

void CellparameterTest::testIsCellParam_data()
{
#define SETNEWROW(arg) QTest::newRow(arg) << string(arg)
	QTest::addColumn<string>("srcStr");
	QTest::addColumn<bool>("expected");
	SETNEWROW("u")        << true;
	SETNEWROW("u1")       << false;
	SETNEWROW("  trcl ")  << true;
	SETNEWROW(" *trcl ")  << true;
	SETNEWROW("trcl1")    << false;
	SETNEWROW("fill")     << true;
	SETNEWROW(" *fill ")  << true;
	SETNEWROW("*fill2")   << false;
	SETNEWROW("lat")      << true;
	SETNEWROW("-lat")     << false;
	SETNEWROW("lat:n")    << false;
	SETNEWROW("dxc1:n")   << true;
	SETNEWROW("dxc222:n") << true;
	SETNEWROW("dxc:p")    << true;
	SETNEWROW("dxc1")     << false;
	SETNEWROW("wwn5:p")   << true;
	SETNEWROW("wwn222:n") << true;
	SETNEWROW("wwn:e")    << true;
	SETNEWROW("wwn1")     << false;
	SETNEWROW("ext:p")    << true;
	SETNEWROW("ext")      << false;
	SETNEWROW("fcl:p")    << true;
	SETNEWROW("fcl")      << false;
	SETNEWROW("imp:n")    << true;
	SETNEWROW("imp")      << false;
	SETNEWROW(" nonu")    << true;
	SETNEWROW("-nonu")    << false;
	SETNEWROW("pd1")      << true;
	SETNEWROW("pd")       << true;
	SETNEWROW("pd1:n")    << false;
	SETNEWROW("pwt")      << true;
	SETNEWROW("pwt1")     << false;
	SETNEWROW("tmp")      << true;
	SETNEWROW("tmp1")     << true;
	SETNEWROW("tmp1:n")   << false;
	SETNEWROW(" vol ")    << true;
	SETNEWROW("vol1")     << false;
	SETNEWROW(" rho")     << true;
	SETNEWROW("-rho")     << false;
	SETNEWROW("mat ")     << true;
	SETNEWROW("mat1")     << false;
#undef SETNEWROW
}
void CellparameterTest::testIsCellParam()
{
	QFETCH(string, srcStr);
	QFETCH(bool, expected);
	QCOMPARE(inp::isCellParam(srcStr), expected);
}

void CellparameterTest::testCellTrString_data()
{
#define SETNEWROW(arg1, arg2) QTest::newRow(arg2) << arg1 << string(arg2)
	QTest::addColumn<bool>("asterisk");
	QTest::addColumn<string>("srcStr");
	QTest::addColumn<string>("expected");
	//        引数1    引数2                               期待される結果
	SETNEWROW(false, " 2 ")                     << string("2");                    // ケース1
	SETNEWROW(false, " (11 2 -3 )")             << string("11 2 -3");              // ケース2
	SETNEWROW(false, " 123(-45 +6 -7  )")       << string("123,-45 +6 -7");        // ケース3
	SETNEWROW(false, " 123   (-45 +6 -7  )")    << string("123,-45 +6 -7");        // ケース4
	SETNEWROW(true,  " 2 ")                     << string("*2");                   // ケース1
	SETNEWROW(true,  " (11 2   -3 ) ")          << string("*11 2   -3");           // ケース2
	SETNEWROW(true,  " 123(-45 +6 -7  )")       << string("*123,*-45 +6 -7");     // ケース3
	SETNEWROW(true,  " 123   (-45 +6 -7  )")    << string("*123,*-45 +6 -7");     // ケース4
	SETNEWROW(false, " (cos(1) {exp(-2)} +3) ") << string("cos(1) {exp(-2)} +3");
	SETNEWROW(true,  " (cos(1) {exp(-2)} +3) ") << string("*cos(1) {exp(-2)} +3");
	SETNEWROW(true,  " (0 0 0  0 -90 -90  90 0 -90  90 {45 + 45} 0, 2 2 2) ") << string("*0 0 0  0 -90 -90  90 0 -90  90 {45 + 45} 0,*2 2 2");


#undef SETNEWROW
}
void CellparameterTest::testCellTrString()
{
	QFETCH(bool, asterisk);
	QFETCH(string, srcStr);
	QFETCH(string, expected);
	std::string result;
	inp::getCellTrStr(asterisk, &srcStr, &result);
	QCOMPARE(result, expected);
}

QTEST_APPLESS_MAIN(CellparameterTest)

#include "tst_cellparametertest.moc"
