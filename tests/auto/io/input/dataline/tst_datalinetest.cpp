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

#include  "core/io/input/dataline.hpp"


using namespace inp;

class Dataline_testTest : public QObject
{
	Q_OBJECT

public:
	Dataline_testTest();

private Q_SLOTS:
	void testExpandI();
	void testExpandM();
	void testExpandJ();
	void testExpandR();
	void testExpandIJMR();
};

Dataline_testTest::Dataline_testTest() {}

void Dataline_testTest::testExpandI()
{
	std::string str = "0 1 2  2i 5";
	std::string expected ="0 1 2 3 4 5";
	auto result = inp::DataLine::expandIJMR(DataLine("file", 0, str), " =", false);
	//mDebug() << "result=" << result;
	QVERIFY(result == expected);

	std::string str2 = "0 1 2 i 5";
	std::string expected2  = "0 1 2 3.5 5";
	auto result2 = inp::DataLine::expandIJMR(DataLine("file", 0, str2), " =", false);
	QVERIFY(result2 == expected2);
}

void Dataline_testTest::testExpandM()
{
	std::string str = "0 1 2  5m 5";
	std::string expected  = "0 1 2 10 5";
	auto result = inp::DataLine::expandIJMR(DataLine("file", 0, str), " =", false);
	//mDebug() << "exnamdM result=" << result;
	QCOMPARE(result, expected);

	std::string str2 = "0 1 2  5.0e+2m 0.5m";
	std::string expected2 = "0 1 2 1000 500";
	auto result2 = inp::DataLine::expandIJMR(DataLine("file", 0, str2), " =", false);
	//mDebug() << "exnamdM result=" << result2;
	QCOMPARE(result2, expected2);
}

void Dataline_testTest::testExpandJ()
{
	std::string str = " 1 2j j 5 j";
	std::string expected ="1 j j j 5 j";
	auto result = inp::DataLine::expandIJMR(DataLine("file", 0, str), " =", false);
	//mDebug() << "result=" << result;
	QCOMPARE(result, expected);
}

void Dataline_testTest::testExpandR()
{
	std::string str = "  0 1 2 3r r   ";
	std::string expected {"0 1 2 2 2 2 2"};
	auto result = inp::DataLine::expandIJMR(DataLine("file", 0, str), " =", false);
	//mDebug() << "exnamdR result=" << result;
	QVERIFY(true);
	QCOMPARE(result, expected);
}

void Dataline_testTest::testExpandIJMR()
{
	std::string str = "2 r 2r 5m 2i 40 2j j      ";
	std::string expected = "2 2 2 2 10 20 30 40 j j j";
	auto result = inp::DataLine::expandIJMR(DataLine("file", 0, str), " =", false);
	//mDebug() << "IMR result=" << result;
	QCOMPARE(result, expected);
}

QTEST_APPLESS_MAIN(Dataline_testTest)

#include "tst_datalinetest.moc"
