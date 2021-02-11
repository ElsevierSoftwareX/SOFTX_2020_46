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


#include "core/io/input/phits/phitsinputsection.hpp"
#include "core/io/input/dataline.hpp"
#include "core/utils/utils.hpp"


using namespace inp;
using namespace phits;

class PhitsinputsectionTest : public QObject
{
	Q_OBJECT

public:
	PhitsinputsectionTest();

private Q_SLOTS:
	void testExpandBrace();
};

PhitsinputsectionTest::PhitsinputsectionTest() {}

void PhitsinputsectionTest::testExpandBrace()
{

	auto result1 = PhitsInputSection::expandBrace("{ 100  - 105   }");
	std::string expected1 = "100 101 102 103 104 105";
//	mDebug() << "result=" << result;
//	mDebug() << "expect=" << expected;
	QCOMPARE(result1, expected1);
	QVERIFY_EXCEPTION_THROWN(PhitsInputSection::expandBrace("{101-101}"), std::invalid_argument);
	QVERIFY_EXCEPTION_THROWN(PhitsInputSection::expandBrace("{105-101}"), std::invalid_argument);
}

QTEST_APPLESS_MAIN(PhitsinputsectionTest)

#include "tst_phitsinputsectiontest.moc"
