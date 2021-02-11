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


#include <iostream>
#include "core/io/input/ijmr.hpp"
#include "core/utils/message.hpp"

class IjmrTest : public QObject
{
	Q_OBJECT

public:
	IjmrTest();

private Q_SLOTS:
	void testCutFirstElement();
	void testIjmrExpand();
};

IjmrTest::IjmrTest() {;}

void IjmrTest::testCutFirstElement()
{
	std::string str1(" aa bbb   c dd");
	std::string elem;
	elem = inp::cutFirstElement(" =", &str1);
	QCOMPARE(elem, "aa");
	QCOMPARE(str1, "bbb   c dd");
	elem = inp::cutFirstElement(" =", &str1);
	QCOMPARE(elem, "bbb");
	QCOMPARE(str1, "c dd");
	elem = inp::cutFirstElement(" =", &str1);
	QCOMPARE(elem, "c");
	QCOMPARE(str1, "dd");
	elem = inp::cutFirstElement(" =", &str1);
	QCOMPARE(elem, "dd");
	QCOMPARE(str1, "");
}

void IjmrTest::testIjmrExpand()
{
	std::string str1("fill 1                  ");
	inp::ijmr::expandIjmrExpression(" =", &str1);
	//mDebug() << "result===" << str1;
	std::string expected1("fill 1");
	QCOMPARE(str1, expected1);
}

QTEST_APPLESS_MAIN(IjmrTest)

#include "tst_ijmrtest.moc"
