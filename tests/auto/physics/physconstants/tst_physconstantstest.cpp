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

#include "../../../../core/physics/physconstants.hpp"
#include "../../../../core/utils/message.hpp"

class PhysconstantsTest : public QObject
{
	Q_OBJECT

public:
	PhysconstantsTest();

private Q_SLOTS:
	void testCase1();
};

PhysconstantsTest::PhysconstantsTest()
{
}

void PhysconstantsTest::testCase1()
{
	// 熱中性子(0.025MeV)のドブロイ波長は1.8E-8 cm = 1.8A
	double len_ang2 = 1.8;
	double result2 = phys::nAngToMeV(len_ang2);
	double expected2 = 0.025*1e-6;
	QVERIFY(std::abs(result2 - expected2)/(0.5*result2+0.5*expected2) < 1e-2);

	double len_ang1 = 2.9E-4;
	double result1 = phys::nAngToMeV(len_ang1);
	double expected1 = 1.0;
    //mDebug() << "result1=" << result1 << "expected1=" << expected1;
	QVERIFY(std::abs(result1 - expected1)/(0.1*result2+0.5*expected1) < 1e-1);
}

QTEST_APPLESS_MAIN(PhysconstantsTest)

#include "tst_physconstantstest.moc"
