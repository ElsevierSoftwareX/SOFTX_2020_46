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

#include "core/utils/message.hpp"
class Test_debugutilsTest : public QObject
{
	Q_OBJECT

public:
	Test_debugutilsTest();

private Q_SLOTS:
	void testCase1();
	void testVariadicTemp();
};

Test_debugutilsTest::Test_debugutilsTest(){;}

void Test_debugutilsTest::testCase1()
{
	mDebug() << "a" << "b" << "c";
	std::vector<int> vec{1, 20, 5, 3};
	mDebug() << vec;
	mDebug() << true;
}

void Test_debugutilsTest::testVariadicTemp()
{
    typedef MessengerBase<OUTPUT_TYPE::MDEBUG> Wm ;
    MessengerBase<OUTPUT_TYPE::MWARNING>("a", "b", 1, std::vector<double>{1.0, 5, 6.00, -1.1}, "true", true) << "AAAAAAAAAAAA";

    MessengerBase<OUTPUT_TYPE::MDEBUG>("a", "b", 1, std::vector<double>{1.0, 5, 6.00, -1.1}, "true", true) << "AAAAAAAAAAAA";

	Wm("file:line", "something is wrong") << "This is additional message!";

	std::vector<int> vec{1, 20, 5, 3};
	mDebug() << vec;
}

QTEST_APPLESS_MAIN(Test_debugutilsTest)

#include "tst_messagetest.moc"
