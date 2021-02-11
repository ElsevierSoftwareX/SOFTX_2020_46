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

#include "core/io/input/filldata.hpp"

class filldata : public QObject
{
    Q_OBJECT

public:
    filldata();
    ~filldata();

private slots:
    void test_case1();

};

filldata::filldata()
{

}

filldata::~filldata()
{

}

void filldata::test_case1()
{
    const std::string str = " fill 13 (-8.039 8.989 0) imp:n 1";
    inp::FillingData fillData = inp::FillingData::fromString(str);
    QCOMPARE(fillData.toInputString(), "13(-8.039 8.989 0)");
}

QTEST_APPLESS_MAIN(filldata)

#include "tst_filldata.moc"
