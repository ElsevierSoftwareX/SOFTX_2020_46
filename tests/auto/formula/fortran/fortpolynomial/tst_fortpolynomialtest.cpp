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

#include <cmath>

#include "../../../../../core/utils/message.hpp"
#include "../../../../../core/utils/numeric_utils.hpp"
#include "../../../../../core/formula/fortran/fortnode.hpp"
#include "../../../../../core/formula/formula_utils.hpp"

using namespace fort;
using namespace std;

/*
 * FOTRANは演算
 * べき乗 **
 * 乗算 *
 * 除算 /
 * 加算 +
 * 減算 -
 * 符号反転 -
 * 組み込み関数
 *
 * を考慮する必要がある。
 */

class FortpolynomialTest : public QObject
{
	Q_OBJECT

public:
	FortpolynomialTest();

private Q_SLOTS:
    void testParenthesis();
    void testSignedFunction();
    void testCalc1();
    void testCase1();
    void testNode1();
};

FortpolynomialTest::FortpolynomialTest() {}

void FortpolynomialTest::testParenthesis()
{
    string eqStr = "(0.6)+(0.4)";
    utils::isSameDouble(1.0, fort::eq(eqStr));
}

void FortpolynomialTest::testSignedFunction()
{
    std::string eqStr="-sin(45.0/180.0*pi)";
    fort::Node node(eqStr);
    double result = node.calculate();
    double expect = -1*std::sin(45.0/180.0 * 3.14159265359);
    QVERIFY(utils::isSameDouble(result, expect));

}

void FortpolynomialTest::testCalc1()
{
    std::string eqStr = "x**0.5*exp(-1*x/1.42)";
    auto func = [](double x) {return std::sqrt(x)*std::exp(-x/1.42);};
    fort::Node equation(eqStr);

    size_t np = 100;
    double emin=1e-12, emax=20;
    double dE = (emax-emin)/(np-1);
    std::vector<double> epoints{emin};

    for(size_t i = 1; i < np; ++i) {
        epoints.emplace_back(epoints.at(i-1)+dE);
    }
    for(size_t i =0; i < epoints.size(); ++i) {
        double result = equation.calculate("x", epoints.at(i));
        double expected = func(epoints.at(i));
        QVERIFY(utils::isSameDouble(result, expected));
    }
}

void FortpolynomialTest::testCase1()
{
    std::string source = "012(4(67)(0))";
    auto poss = formula::findOutmostParenthesis(source);
    QCOMPARE(poss.first, size_t(3));
    QCOMPARE(poss.second, size_t(12));
    QVERIFY_EXCEPTION_THROWN(formula::findOutmostParenthesis("123((sdfd)"), std::invalid_argument);
    QVERIFY_EXCEPTION_THROWN(formula::findOutmostParenthesis("123sdfd)"), std::invalid_argument);
}

void FortpolynomialTest::testNode1()
{
    // 指数表記
    QVERIFY(utils::isSameDouble(Node(".1E1").calculate(), 1.0));
    QVERIFY(utils::isSameDouble(Node(".1E-1").calculate(), 0.01));
    QVERIFY(utils::isSameDouble(Node("1.0E-2 + 0.01").calculate(), 0.02));
    QVERIFY(utils::isSameDouble(Node("1.0E+2 + 0.01").calculate(), 100.01));
    QVERIFY(utils::isSameDouble(Node("1.0E-2 * 0.01").calculate(), 0.0001));
    QVERIFY(utils::isSameDouble(Node("1.0E-2 / 0.01").calculate(), 1.0));
    QVERIFY(utils::isSameDouble(Node("11.0E2 * 0.01").calculate(), 11.0));
    QVERIFY(utils::isSameDouble(Node("log10(1e2) + log10(1.e-2)").calculate(), 0.0));

    //	// 四則演算とべき乗
    QVERIFY(utils::isSameDouble(Node("1+2").calculate(), 1.0+2.0));
    QVERIFY(utils::isSameDouble(Node("1-2").calculate(), 1.0-2.0));
    QVERIFY(utils::isSameDouble(Node("1*2").calculate(), 1.0*2.0));
    QVERIFY(utils::isSameDouble(Node("1/2").calculate(), 1.0/2.0));
    QVERIFY(utils::isSameDouble(Node("3**2").calculate(), 3.0*3.0));
    QVERIFY(utils::isSameDouble(Node("1+2**(1+2)+ (5-6)**(7*8)").calculate(), 1+std::pow(2,(1+2))+ std::pow((5-6), (7*8))));

    // 単項演算
    QVERIFY(utils::isSameDouble(Node("-1").calculate(), -1.0));
    QVERIFY(utils::isSameDouble(Node("+1").calculate(), 1.0));
    QVERIFY(utils::isSameDouble(Node("-(1+2)").calculate(), -3.0));
    QVERIFY(utils::isSameDouble(Node("+(1-2)").calculate(), -1.0));

    //組み込み(1変数)
    //exp, log10, log, sqrt, asin, sinh, sin, acos, cosh, cos, atan, tanh, tan, abs, float int, nint
    QVERIFY(utils::isSameDouble(Node("exp(0)+exp(1+2)").calculate(), std::exp(0) + std::exp(1+2)));
    QVERIFY(utils::isSameDouble(Node("log10(1.E-9)").calculate(), -9.0));
    QVERIFY(utils::isSameDouble(Node("log(1.526)").calculate(), std::log(1.526)));
    QVERIFY(utils::isSameDouble(Node("sqrt(3.1)").calculate(), std::sqrt(3.1)));
    QVERIFY(utils::isSameDouble(Node("asin(0.6)").calculate(), std::asin(0.6)));
    QVERIFY(utils::isSameDouble(Node("sinh(3.1)").calculate(), std::sinh(3.1)));
    QVERIFY(utils::isSameDouble(Node("sin(3.1)").calculate(), std::sin(3.1)));
    QVERIFY(utils::isSameDouble(Node("acos(-0.6)").calculate(), std::acos(-0.6)));
    QVERIFY(utils::isSameDouble(Node("cosh(-3.1)").calculate(), std::cosh(-3.1)));
    QVERIFY(utils::isSameDouble(Node("cos(0.6)").calculate(), std::cos(0.6)));
    QVERIFY(utils::isSameDouble(Node("atan(0.6)").calculate(), std::atan(0.6)));
    QVERIFY(utils::isSameDouble(Node("tanh(0.6)").calculate(), std::tanh(0.6)));
    QVERIFY(utils::isSameDouble(Node("tan(0.6)").calculate(), std::tan(0.6)));
    QVERIFY(utils::isSameDouble(Node("abs(-0.6)").calculate(), std::abs(-0.6)));
    QVERIFY(utils::isSameDouble(Node("int(0.6)").calculate(), 0.0));
    QVERIFY(utils::isSameDouble(Node("nint(0.6)").calculate(), 1.0));

    QVERIFY_EXCEPTION_THROWN(Node("log10(-1.526)").calculate(), std::domain_error);
    QVERIFY_EXCEPTION_THROWN(Node("log(-1.526)").calculate(), std::domain_error);
    QVERIFY_EXCEPTION_THROWN(Node("sqrt(-3.1)").calculate(), std::domain_error);
    QVERIFY_EXCEPTION_THROWN(Node("asin(3.1)").calculate(), std::domain_error);
    QVERIFY_EXCEPTION_THROWN(Node("sinh(1e6)").calculate(), std::overflow_error);
    QVERIFY_EXCEPTION_THROWN(Node("acos(1.1)").calculate(), std::domain_error);
    QVERIFY_EXCEPTION_THROWN(Node("cosh(1e6)").calculate(), std::overflow_error);

    // 組み込み 2変数
    // atan2, mod, sign
    QVERIFY(utils::isSameDouble(Node("mod(10, 3)").calculate(), 1.0));
    QVERIFY(utils::isSameDouble(Node("mod(10, 3.5)").calculate(), 3.0));
    QVERIFY(utils::isSameDouble(Node("sign(1, -1)").calculate(), -1.0));
    QVERIFY(utils::isSameDouble(Node("sign(5, 1)").calculate(), 5.0));
    QVERIFY(std::abs(Node("atan2(2, 3.46410161512)").calculate() - math::toRadians(30.0)) < 1e-10); // 入力12桁だからこんなもん
    // 組み込み variadic 可変長関数
    QVERIFY(utils::isSameDouble(Node("min(-1, -10.1, -99)").calculate(), -99.0));
    QVERIFY(utils::isSameDouble(Node("max(-1, -10.1, -99)").calculate(), -1.0));

    // 適当に混ぜる
    QVERIFY(utils::isSameDouble(Node("exp(exp(1))+2**(1+2)+ (5-6)**(7*8) -cos(sin(0.5))").calculate(),
             std::exp(std::exp(1))+std::pow(2,(1+2))+ std::pow((5-6), (7*8)) - std::cos(std::sin(0.5))));
    QVERIFY(utils::isSameDouble(fort::eq("exp(exp(1))+2**(1+2)+ (5-6)**(7*8) -cos(sin(0.5))"),
             std::exp(std::exp(1))+std::pow(2,(1+2))+ std::pow((5-6), (7*8)) - std::cos(std::sin(0.5))));
}

QTEST_APPLESS_MAIN(FortpolynomialTest)

#include "tst_fortpolynomialtest.moc"
