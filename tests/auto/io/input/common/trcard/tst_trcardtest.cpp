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

#include <stdexcept>
#include "core/io/input/common/trcard.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"


using namespace inp;
using namespace comm;



using namespace math;

class Trcard_testTest : public QObject
{
	Q_OBJECT

public:
	Trcard_testTest();

private Q_SLOTS:
    void testTrCard3();
    void testTrMod();
    void testTrCard1();
    void testTrCard2();

};

Trcard_testTest::Trcard_testTest() {;}

void Trcard_testTest::testTrCard3()
{
    std::string str1 = "tr6  (-0.500000)*1 0 0   1 0 0  0 1 0  0 0 1  1";
    TrCard tr1(str1);
}


void Trcard_testTest::testTrMod()
{
    std::string str1 = "*Tr01 0 0 10";
    std::string str2 = "tR1* 0 0 10";
    std::string str3 = "TR5 0 0 cos(0)";
    std::string str4 = "-TR1 0 12 0";
    std::string str5 = "TR1x 0 0 10";
    std::string str6 = "*TR5 0 0  ab";
    std::string str7 = "TR-1* 0 0 10";
    TrCard tr1(str1);
    TrCard tr2(str2);
    TrCard tr3(str3);
    QCOMPARE(tr1.idNumber(), 1);
    QCOMPARE(tr2.idNumber(), 1);
    QCOMPARE(tr3.idNumber(), 5);
    QCOMPARE(tr1.id(), "01");
    QCOMPARE(tr2.id(), "1");
    QCOMPARE(tr3.id(), "5");
    QCOMPARE(tr1.modifier(), "*");
    QCOMPARE(tr2.modifier(), "*");
    QCOMPARE(tr3.modifier().empty(), true);

    QVERIFY_EXCEPTION_THROWN(TrCard{str4}, std::invalid_argument);
    QVERIFY_EXCEPTION_THROWN(TrCard{str5}, std::invalid_argument);
    QVERIFY_EXCEPTION_THROWN(TrCard{str6}, std::invalid_argument);
    QVERIFY_EXCEPTION_THROWN(TrCard{str7}, std::invalid_argument);
}


void Trcard_testTest::testTrCard1()
{
    std::string str1 = "Tr01 0 0 10";
    std::string str2 = "tR1 0 0 10";
    std::string str3 = "TR5 0 0 10";
    std::string str4 = "TR1 0 ab 0 10";
    std::string str5 = "TR1x 0 0 10";
    std::string str6 = "SO 0 0 10";
    std::string str7 = "TR-1 0 0 10";
    TrCard tr1(str1);
    TrCard tr2(str2);
    TrCard tr3(str3);
    QCOMPARE(tr1.idNumber(), 1);
    QCOMPARE(tr2.idNumber(), 1);
    QCOMPARE(tr3.idNumber(), 5);
    QVERIFY_EXCEPTION_THROWN(TrCard{str4}, std::invalid_argument);
    QVERIFY_EXCEPTION_THROWN(TrCard{str5}, std::invalid_argument);
    QVERIFY_EXCEPTION_THROWN(TrCard{str6}, std::invalid_argument);
    QVERIFY_EXCEPTION_THROWN(TrCard{str7}, std::invalid_argument);
}

void Trcard_testTest::testTrCard2()
{
    std::string str0 = "TR01  j j j  j j j  j j j   j  j j     j";  // all jは恒等変換
    std::string str1 = "Tr6   0 0 0  1 0 0  0 1 0   0  0 1     1";  // フル9要素入力
    std::string str2 = "TR22  j j j  1 0 0  0 1 0   0  0 1     1";  // フル9要素入力 with j
    std::string str3 = "TR-10 j j j  1 0 0  0 1 0   j  j j    -1";  // 恒等変換
    std::string str4 = "TR9   j j j  j j j  0 1 0   0  0 1.0  -1";  // 恒等変換
    std::string str5 = "TR08  j j j  1 0 0  j j j   0  0 1.0  -1";  // 恒等変換
    std::string str6 = "TR06  1 2 3  4 5 6  7 8 9  10 11 12   13";  // 適当な入力
    std::string str7 = "TR7   1 2 3  4 5 6  7 8 9  10 11 120  -13";  // 適当な入力
    TrCard tr0(str0);
    TrCard tr1(str1);
    TrCard tr2(str2);
    QVERIFY_EXCEPTION_THROWN(TrCard{str3}, std::invalid_argument);
    TrCard tr4(str4);
    TrCard tr5(str5);
    QVERIFY_EXCEPTION_THROWN(TrCard{str6}, std::invalid_argument);
    TrCard tr7(str7);

    QCOMPARE(tr0.idNumber(), 1);
    QCOMPARE(tr1.idNumber(), 6);
    QCOMPARE(tr2.idNumber(), 22);
    QCOMPARE(tr4.idNumber(), 9);
    QCOMPARE(tr5.idNumber(), 8);
    QCOMPARE(tr7.idNumber(), 7);

//	mDebug() <<" tr0="<< tr0.trMatrix();
    QVERIFY(math::isSameMatrix(tr0.trMatrix(), Matrix<4>::IDENTITY()));
    QVERIFY(math::isSameMatrix(tr1.trMatrix(), Matrix<4>::IDENTITY()));
    QVERIFY(math::isSameMatrix(tr2.trMatrix(), Matrix<4>::IDENTITY()));
    QVERIFY(math::isSameMatrix(tr4.trMatrix(), Matrix<4>::IDENTITY()));
    QVERIFY(math::isSameMatrix(tr5.trMatrix(), Matrix<4>::IDENTITY()));

    auto mat7rot = tr7.trMatrix().rotationMatrix();
    auto mat7unit = mat7rot*mat7rot.transposed();
    QVERIFY(math::isSameMatrix(mat7unit, Matrix<3>::IDENTITY()));
}

QTEST_APPLESS_MAIN(Trcard_testTest)

#include "tst_trcardtest.moc"
