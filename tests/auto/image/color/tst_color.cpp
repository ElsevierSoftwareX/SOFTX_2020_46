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
#include <QtTest>

// add necessary includes here

#include "core/utils/message.hpp"
#include "core/image/color.hpp"


using namespace img;

class TestColor : public QObject
{
	Q_OBJECT

public:
	TestColor();
	~TestColor();

private slots:
    void testColorFromString();
	void testPalette1();

};

TestColor::TestColor()
{

}

TestColor::~TestColor()
{

}

void TestColor::testColorFromString()
{
    QCOMPARE(std::string("#ffffff"), Color::fromPhitsString("white").toRgbString());
    QCOMPARE(std::string("#000000"), Color::fromPhitsString("black").toRgbString());
    QCOMPARE(std::string("#ff0000"), Color::fromPhitsString("red").toRgbString());
	QCOMPARE(std::string("#0000ff"), Color::fromPhitsString("blue").toRgbString());
	QCOMPARE(std::string("#00ffff"), Color::fromPhitsString("cyan").toRgbString());
	QCOMPARE(std::string("#ffff00"), Color::fromPhitsString("yellow").toRgbString());
}

void TestColor::testPalette1()
{
	std::vector<std::string> strings1 {
		"mat      name      size   color",
		"0         void      1     lightgray",
		"1         air       0.5   yellowgreen",
		"2         {mat 2}   2     orangeyellow",
		"3         {mat 3}   1     {0.067 0.067 1.0}",
		"{4 - 7}   Fe        3     mossgreen"
	};

}

QTEST_APPLESS_MAIN(TestColor)

#include "tst_color.moc"
