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

#include <algorithm>
#include <chrono>
#include "core/math/equationsolver.hpp"
#include "core/utils/message.hpp"



class EquationsolverTest : public QObject
{
	Q_OBJECT

public:
	EquationsolverTest();

private Q_SLOTS:
	void test4th2();
	void test2nd();
	void test3rd();
	void testCompoundQuadratic();
	void test4th();
	void testBug3();
};

EquationsolverTest::EquationsolverTest() {}

void EquationsolverTest::test4th2()
{
	double c4=81.0000000000000711, c3=-971.967600000000971, c2=27060.1884043286518, c1=-144861.519769971666, c0=210336.31869343578;
//	double c4=81.0000000000001,    c3=-971.967600000001,    c2=27060.1884043287,    c1=-144861.519769972,    c0=210336.318693436;
	auto results = math::solve4thR<double>(c4, c3, c2, c1, c0, true);
	mDebug() << "results before=" << results;
	for(auto &res: results) {
		res = math::mod4thRbyNewton<double>(c4, c3, c2, c1, c0, res, 1e-12);
	}
	std::sort(results.begin(), results.end());
	std::vector<double> expects {2.869014932042753059023, 3.130787352739158529199};  // 20桁まで正しい解
	mDebug() << "results after=" << results;
}

void EquationsolverTest::test2nd()
{
	using namespace math;
	// 2次方程式の解法テスト
	double a, b, c;
	// 二次の係数が0の場合
	a = 0; b = 5; c=100;
	QCOMPARE(math::solve2ndR(a, b, c, false), std::vector<double>{-c/b});

	// 重解
	a = 1; b = 2; c = 1;
	QCOMPARE(math::solve2ndR(a, b, c, true), std::vector<double>{-b/(2*a)});
	QCOMPARE(math::solve2ndR(a, b, c, false), std::vector<double>());

	// 解なし
	a = 1; b = 2; c = 4;
	QCOMPARE(math::solve2ndR(a, b, c, true), std::vector<double>());

	// 実2解
	a = 5;b = 10; c = 1;
	std::vector<double> ex1{(-b + std::sqrt(b*b - 4*a*c))/(2*a), (-b - std::sqrt(b*b - 4*a*c))/(2*a)};
	std::sort(ex1.begin(), ex1.end());
	auto sol2 = math::solve2ndR(a, b, c, false);
	std::sort(sol2.begin(), sol2.end());
	for(size_t i = 0; i < 2; ++i) {
		QVERIFY(std::abs(sol2[i] - ex1[i]) < math::EPS);
	}

	// 虚数重解
	typedef std::complex<double> comp_type;
	comp_type c2, c1, c0;
	double A=1, B=2;
	comp_type tmp(A, B);
	c2 = comp_type(1, 0);
	c1 = -2.0*tmp;
	c0 = std::complex<double>(A*A-B*B, 2*A*B);
	std::vector<comp_type> ex2{comp_type(A, B)};
	auto sols = math::solve2ndC(c2, c1, c0, true);
	QCOMPARE(sols.size(), (size_t)1);
	QVERIFY(std::abs(sols[0] - ex2[0]) < math::EPS);
	// 2虚数解
	double C=2, D=3;
	comp_type tmp2(C, D);
	c2 = comp_type(1, 0);
	c1 = -1.0*(tmp + tmp2);
	c0 = tmp*tmp2;
	std::vector<comp_type> ex3{comp_type(A, B), comp_type(C, D)};
	auto compFunc = [](const comp_type& comp1, const comp_type& comp2) {
					return comp1.real() > comp2.real();
				};
	std::sort(ex3.begin(), ex3.end(), compFunc);
	sols = math::solve2ndC(c2, c1, c0, true);
	std::sort(sols.begin(), sols.end(), compFunc);
	//mDebug() << sols;
	QCOMPARE(sols.size(), (size_t)2);
	for(size_t i = 0; i < sols.size(); ++i) {
		QVERIFY(std::abs(sols[i] - ex3[i]) < math::EPS);
	}
}

void EquationsolverTest::test3rd()
{
	double c3, c2, c1, c0;
	std::vector<double> sols;

	// 3重解 x=-2,  f(x)=(x+2)^3 =  x^3 + 6x^2 12x +8  (x=-2)
	c3 = 1; c2 = 6; c1 = 12; c0 = 8;
	sols = math::solve3rdR(c3, c2, c1, c0, true);
	QCOMPARE(sols.size(), (size_t)1);
	QVERIFY(std::abs(sols[0] + 2) < math::EPS);
	QCOMPARE(math::solve3rdR(c3, c2, c1, c0, false), std::vector<double>());

	// 実解1個＋実2重解1個
	// x = 2, -2, -2,  f(x)=(x-2)*(x+2)^2 =  x^3  + 2x^2  -4x^2 - 8
	c3 = 1; c2 = 2; c1 = -4; c0 = -8;
	sols = math::solve3rdR(c3, c2, c1, c0, false);
	//mDebug() << "solution======" << sols;
	QCOMPARE(sols.size(), (size_t)1);
	QVERIFY(std::abs(sols[0] -2) < math::EPS);
	sols = math::solve3rdR(c3, c2, c1, c0, true);
	QCOMPARE(sols.size(), (size_t)2);
	std::vector<double> ex2{2, -2};
	for(size_t i = 0; i < 2; ++i) {
		QVERIFY(std::abs(sols[i] - ex2[i]) < math::EPS);
	}

	// 実解1個＋実2重解1個(最高次の係数が非ゼロ)
	// x = 3, -2, -2,  f(x)=4*(x-3)*(x+2)^2 =  4x^3  + 4x^2  -32x^2 - 48
	c3 = 4; c2 = 4; c1 = -32; c0 = -48;
	sols = math::solve3rdR(c3, c2, c1, c0, false);
	QCOMPARE(sols.size(), (size_t)1);
	QVERIFY(std::abs(sols[0] - 3) < math::EPS);
	sols = math::solve3rdR(c3, c2, c1, c0, true);
	QCOMPARE(sols.size(), (size_t)2);
	std::vector<double> ex3{3, -2};
	for(size_t i = 0; i < 2; ++i) {
		QVERIFY(std::abs(sols[i] - ex3[i]) < math::EPS);
	}

	// 実解1個＋虚解2個
	// x=3, 1+2i, 1-2i, f(x)=(x-3)*(x-1-2i)*(x-1+2i) = (x^2-2x+2)(x+3) = x^3 -5x^2 +8x -6
	c3 = 1; c2 = -5; c1 = 8; c0 = -6;
	sols = math::solve3rdR(c3, c2, c1, c0, false);
	QCOMPARE(sols.size(), (size_t)1);
	//mDebug() << "solution=" << sols[0];
	QVERIFY(std::abs(sols[0] - 3) < math::EPS);
	sols = math::solve3rdR(c3, c2, c1, c0, true);
	QCOMPARE(sols.size(), (size_t)1);
	QVERIFY(std::abs(sols[0] - 3) < math::EPS);

	// 実解1個＋虚解2個 (c3≠0)
	// x=3, 1+2i, 1-2i, f(x)=2*(x-3)*(x-1-2i)*(x-1+2i) = 2*(x^2-2x+2)(x+3) = 2x^3 -10x^2 +16x -12
	c3 = 2; c2 = -10; c1 = 16; c0 = -12;
	sols = math::solve3rdR(c3, c2, c1, c0, false);
	QCOMPARE(sols.size(), (size_t)1);
	QVERIFY(std::abs(sols[0] - 3) < math::EPS);
	sols = math::solve3rdR(c3, c2, c1, c0, true);
	QCOMPARE(sols.size(), (size_t)1);
	QVERIFY(std::abs(sols[0] - 3) < math::EPS);

	// 実解3個
	// x=4, 2, -3,  f(x)=(x-4)*(x-2)*(x+3)   = x^3 -3x^2 +10x +24
	// x=4, 2, -3,  f(x)=3*(x-4)*(x-2)*(x+3) = 3x^3 -9x^2 +30x +72
	//c3 = 1; c2 = -3; c1 = -10; c0 = 24;
	c3 = 3; c2 = -9; c1 = -30; c0 = 72;
	// 解法の選択のため時間を測定する。版
//	auto start = std::chrono::system_clock::now();
//	for(size_t i = 1; i < 100000; ++i) {
//		//mDebug() << "i=" << i;
//		sols = math::solve3rdR(c3*i, c2*i, c1*i, c0*i, false);
//	}
//	auto end = std::chrono::system_clock::now();
//	mDebug() << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << "muSec";
	sols = math::solve3rdR(c3, c2, c1, c0, false);
	std::vector<double> ex4{4, 2, -3};
	std::sort(sols.begin(), sols.end());
	std::sort(ex4.begin(), ex4.end());
	QCOMPARE(sols.size(), (size_t)3);
	//mDebug() << "solutions=" << sols;
	for(size_t i = 0; i < 3; ++i) {
		//mDebug() << "result=" << sols[i] << ", expect=" << ex4[i];
		QVERIFY(std::abs(sols[i]-ex4[i]) < math::EPS);
	}
}


// 複二次式
void EquationsolverTest::testCompoundQuadratic()
{
	double c4, c3, c2, c1, c0;
	std::vector<double> sols;

	// x^4 = 1, の場合
	std::vector<double> ex1{1, -1}; // 実解は±1 複素解は±i
	c4 = 1; c3=0; c2=0; c1=0; c0=-1;
	sols = math::solve4thR(c4, c3, c2, c1, c0, true);
	QCOMPARE(sols.size(), (size_t)2);
	for(size_t i = 0; i < 2; ++i) {
		QVERIFY(std::abs(sols[i]-ex1[i]) < math::EPS);
	}

	// x=±2の重解2個
	// x^4 -8x^2 +16
	c4 = 1; c3=0; c2=-8; c1=0; c0=16;
	std::vector<double> ex2{2, -2}; // 解は実解は±2の重解2個
	sols = math::solve4thR(c4, c3, c2, c1, c0, true);
	QCOMPARE(sols.size(), (size_t)2);
	for(size_t i = 0; i < 2; ++i) {
		QVERIFY(std::abs(sols[i]-ex2[i]) < math::EPS);
	}
	QCOMPARE(math::solve4thR(c4, c3, c2, c1, c0, false).size(), (size_t)0);

	// x^4 =0,  x=2の4重解
	c4 = 1; c3=0; c2=0; c1=0; c0=0;
	std::vector<double> ex3{0};
	sols = math::solve4thR(c4, c3, c2, c1, c0, true);
	QCOMPARE(sols.size(), (size_t)1);
	for(size_t i = 0; i < sols.size(); ++i) {
		QVERIFY(std::abs(sols[i]-ex3[i]) < math::EPS);
	}
	QCOMPARE(math::solve4thR(c4, c3, c2, c1, c0, false).size(), (size_t)0);
}

void EquationsolverTest::test4th()
{
	double c4, c3, c2, c1, c0;
	std::vector<double> sols;

	// 4重解
	// (x-2)^4 = x^4  - 8 x^3  + 24 x^2  - 32 x + 16
	c4 = 1; c3=-8; c2=24; c1=-32; c0=16;
	sols = math::solve4thR(c4, c3, c2, c1, c0, true);
	std::vector<double> ex1{2};  // 1個の重解が返る。
	QCOMPARE(sols.size(), (size_t)1);
	for(size_t i = 0; i < sols.size(); ++i) {
		QVERIFY(std::abs(sols[i]-ex1[i]) < math::EPS);
	}
	QCOMPARE(math::solve4thR(c4, c3, c2, c1, c0, false), std::vector<double>());

	// 4実解
	// x=5, 3, 1, -2     x^4 + -7x^3 +5x^2 +31x + -30
	// x=5, 3, 1, -2     2x^4 + -14x^3 +10x^2 +62x + -60
	c4 = 2; c3=-14; c2=10; c1=62; c0=-60;
	sols = math::solve4thR(c4, c3, c2, c1, c0, true);
	std::vector<double> ex2{5, 3, 1, -2};
	std::sort(sols.begin(), sols.end(), std::greater<double>());
	//mDebug() << "sols=" << sols;
	for(size_t i = 0; i < sols.size(); ++i) {
		QVERIFY(std::abs(sols[i]-ex2[i]) < math::EPS);
	}

	// 2実重解
	// x=2, -3,   f(x)=(x-2)^2*(x+3)^2, = x^4  + 2 x^3  - 11 x^2  - 12 x + 36
	// x=2, -3,   f(x)=3*(x-2)^2*(x+3)^2, = 3x^4  + 6 x^3  - 33 x^2  - 36 x + 108
	c4 = 3; c3=6; c2=-33; c1=-36; c0=108;
	sols = math::solve4thR(c4, c3, c2, c1, c0, true);
	std::vector<double> ex3{2, -3};
	//mDebug() << "sols=" << sols;
	QCOMPARE(sols.size(), ex3.size());
	for(size_t i = 0; i < sols.size(); ++i) {
		QVERIFY(std::abs(sols[i]-ex3[i]) < math::EPS);
	}
	QCOMPARE(math::solve4thR(c4, c3, c2, c1, c0, false), std::vector<double>());

	// 2実解+1実重解
	// x=3, 2, -3,   f(x)=(x-2)*(x-3)*(x+3)^2, = x  + x  - 15 x  - 9 x + 54
	// x=3, 2, -3,   f(x)=3*(x-2)*(x-3)*(x+3)^2, = 3x  + 3x  - 45 x  - 27 x + 162
	c4 = 3; c3=3; c2=-45; c1=-27; c0=162;
	sols = math::solve4thR(c4, c3, c2, c1, c0, true);
	std::sort(sols.begin(), sols.end(), std::greater<double>());
	std::vector<double> ex4{3, 2, -3};
	QCOMPARE(sols.size(), ex4.size());
	for(size_t i = 0; i < sols.size(); ++i) {
		QVERIFY(std::abs(sols[i]-ex4[i]) < math::EPS);
	}
	std::vector<double> ex4m{3, 2};
	sols = math::solve4thR(c4, c3, c2, c1, c0, false);
	QCOMPARE(sols.size(), ex4m.size());
	for(size_t i = 0; i < sols.size(); ++i) {
		QVERIFY(std::abs(sols[i]-ex4m[i]) < math::EPS);
	}

	// 2実解+2複素解
	// x= 3, 4,  2+2i, 2-2i,
	// f(x)=(x-3)*(x-4)(x^2+4) = x^4  - 7 x^3  + 16 x^2  - 28 x + 48
	// f(x)=2(x-3)*(x-4)(x^2+4) = 2x^4  - 14 x^3  + 32 x^2  - 56 x + 96
	c4 = 2; c3=-14; c2=32; c1=-56; c0=96;
	sols = math::solve4thR(c4, c3, c2, c1, c0, true);
	std::sort(sols.begin(), sols.end(), std::greater<double>());
	//mDebug() << "sols=" << sols;
	std::vector<double> ex5{4, 3};
	QCOMPARE(sols.size(), ex5.size());
	for(size_t i = 0; i < sols.size(); ++i) {
		QVERIFY(std::abs(sols[i]-ex5[i]) < math::EPS);
	}

	// 1実重解 + 2複素解
	// x= 2, 3+2i, 3-2i
	// f(x)=(x-2)^2*(x^2+13) = x^4  - 4 x^3  + 17 x^2  - 52 x + 52
	// f(x)=-2*(x-2)^2*(x^2+13) = -2x^4  +8 x^3  -34 x^2  + 104 x -104
	c4 = -2; c3=8; c2=-34; c1=104; c0=-104;
	sols = math::solve4thR(c4, c3, c2, c1, c0, true);
	std::sort(sols.begin(), sols.end(), std::greater<double>());
	//mDebug() << "sols=" << sols;
	std::vector<double> ex6{2};
	QCOMPARE(sols.size(), ex6.size());
	for(size_t i = 0; i < sols.size(); ++i) {
		QVERIFY(std::abs(sols[i]-ex6[i]) < math::EPS);
	}
	QCOMPARE(math::solve4thR(c4, c3, c2, c1, c0, false), std::vector<double>());

	// 複素解のみ
	// x=2+i, 2-i, 3+2i, 3-2i
	// f(x) =(x^2+5)*(x^2+13) = x^4 + 18x^2 + 65
	// f(x) = -2 (x^2+5)*(x^2+13) = -2x^4 - 36x^2  -130
	c4 = -2; c3=0; c2=-36; c1=0; c0=-130;
	sols = math::solve4thR(c4, c3, c2, c1, c0, true);
	QCOMPARE(sols, std::vector<double>());
}

void EquationsolverTest::testBug3()
{
	double c3= 1, c2= -5, c1= 8, c0= -6;
	std::vector<double> sols;
	sols = math::solve3rdR(c3, c2, c1, c0, false);
	QVERIFY(std::abs(sols[0] - 3) < math::EPS);

	// この時実解1個複素解2個になる。が解の値がおかしい
	double a =1, b=84.04, c= 3300, d = -100;
	sols = math::solve3rdR(a, b, c, d, false);
	double ex = 0.03027967255866315;
//	mDebug() << "result = " << sols.front();
//	mDebug() << "expect = " << ex;
	QCOMPARE(sols.front(), ex);
}

QTEST_APPLESS_MAIN(EquationsolverTest)

#include "tst_equationsolvertest.moc"
