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

#include <cmath>
#include <string>
#include <vector>

#include "core/material/nmtc.hpp"
#include "core/utils/message.hpp"

using namespace std;

class nmtc : public QObject
{
    Q_OBJECT

public:
    nmtc();
    ~nmtc();

private slots:
    void test_case1();

};
nmtc::nmtc() {}
nmtc::~nmtc() {}

void nmtc::test_case1()
{
    vector<string> params{"h", "2.0", "o", "1.0"};
    mat::convertNmtcStyle(&params);
    vector<string> expects{"1001", "1.99977", "1002", "0.00023", "8016", "0.99757", "8017", "0.00038", "8018", "0.00205"};
    QCOMPARE(params.size(), expects.size());
    for(size_t i = 0; i < params.size(); ++i) {
        double exval = std::stod(expects.at(i));
        double resval = std::stod(params.at(i));
        mDebug() << "i=" << i << "result =" << resval << "expects=" << exval;
        QVERIFY(abs(exval-resval) < 1e-7);
    }
}

QTEST_APPLESS_MAIN(nmtc)

#include "tst_nmtc.moc"
