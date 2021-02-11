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
#include <QDebug>

#include <list>

#include "core/io/input/phits/phitsinputsubsection.hpp"
#include "core/io/input/dataline.hpp"
#include "core/utils/utils.hpp"

using namespace inp;
using namespace phits;

class Phits_inputsubsectionTest : public QObject
{
	Q_OBJECT

public:
	Phits_inputsubsectionTest();

private Q_SLOTS:

	void testCase1();
	void testCase2();
	void testErrorCase();
};

Phits_inputsubsectionTest::Phits_inputsubsectionTest() {;}


void Phits_inputsubsectionTest::testCase1()
{
	std::list<inp::DataLine> singleParamSubsection
	{
		{"file",  2, "  point = 1                        "},
		{"file",  3, " x   y    z  r0                    "},
		{"file",  4, " 0   1    2   0.0                  "}
	};

	auto it = singleParamSubsection.cbegin();
	++it;
	inp::phits::PhitsInputSubsection subsection(it, singleParamSubsection.cend(), 1);
	mDebug() << subsection.toString();
	std::vector<double> xex{0.0}, yex{1}, zex{2}, rex{0.0};
	auto xresult = subsection.getValueVector("x");
	auto yresult = subsection.getValueVector("y");
	auto zresult = subsection.getValueVector("z");
	auto rresult = subsection.getValueVector("r0");
	for(size_t i = 0; i < xex.size(); ++i) {
		QCOMPARE(utils::stringTo<double>(xresult.at(i)), xex.at(i));
		QCOMPARE(utils::stringTo<double>(yresult.at(i)), yex.at(i));
		QCOMPARE(utils::stringTo<double>(zresult.at(i)), zex.at(i));
		QCOMPARE(utils::stringTo<double>(rresult.at(i)), rex.at(i));
	}
}

void Phits_inputsubsectionTest::testCase2()
{
	std::list<inp::DataLine> multiParamSubsection
	{
		{"file",  3, " non x   y    z  r0                    "},
		{"file",  4, " 1   0   1    2   0.0                  "},
		{"file",  5, " 2   0   1    2   0.0                  "},
		{"file",  6, " 3   1   2    5   1.0                  "},
		{"file",  7, " 4  10  20   50   10.0                  "}
	};

	auto it = multiParamSubsection.cbegin();
	inp::phits::PhitsInputSubsection subsection(it, multiParamSubsection.cend(), 4);
	mDebug() << subsection.toString();
	std::vector<double> xex{0.0, 0, 1}, yex{1, 1, 2}, zex{2, 2, 5}, rex{0.0, 0, 1};
	auto xresult = subsection.getValueVector("x");
	auto yresult = subsection.getValueVector("y");
	auto zresult = subsection.getValueVector("z");
	auto rresult = subsection.getValueVector("r0");
	for(size_t i = 0; i < xex.size(); ++i) {
		QCOMPARE(utils::stringTo<double>(xresult.at(i)), xex.at(i));
		QCOMPARE(utils::stringTo<double>(yresult.at(i)), yex.at(i));
		QCOMPARE(utils::stringTo<double>(zresult.at(i)), zex.at(i));
		QCOMPARE(utils::stringTo<double>(rresult.at(i)), rex.at(i));
	}
}

void Phits_inputsubsectionTest::testErrorCase()
{
	using namespace inp;
	using namespace phits;
	using namespace std;
	std::list<inp::DataLine> emptySubsection
	{
		{"file",  3, " x   y    z  r0                    "}
	};
	auto it = emptySubsection.cbegin();
	// 入力行が少なすぎる場合
	QVERIFY_EXCEPTION_THROWN(inp::phits::PhitsInputSubsection(it, emptySubsection.cend(), 1), std::invalid_argument);

	std::list<inp::DataLine> titleErrorSubsection
	{
		{"file",  3, " x   y    z                        "},
		{"file",  4, " 0   1    2   0.0                  "},
	};
	it = titleErrorSubsection.cbegin();
	// タイトル行が欠損している場合
	QVERIFY_EXCEPTION_THROWN(inp::phits::PhitsInputSubsection(it, titleErrorSubsection.cend(), 1), std::invalid_argument);

	std::list<inp::DataLine> tooFewParamsSubsection
	{
		{"file",  3, " x   y    z   r0                   "},
		{"file",  4, " 0   1    2    1.0                  "},
		{"file",  5, " 1   2        1.0                  "}
	};
	it = tooFewParamsSubsection.cbegin();
	// データが欠損している場合
	QVERIFY_EXCEPTION_THROWN(PhitsInputSubsection(it, tooFewParamsSubsection.cend(), 2), invalid_argument);
	// イテレータendが渡された場合
	QVERIFY_EXCEPTION_THROWN(PhitsInputSubsection(it, tooFewParamsSubsection.end(), 2), invalid_argument);
}

QTEST_APPLESS_MAIN(Phits_inputsubsectionTest)

#include "tst_phits_inputsubsectiontest.moc"
