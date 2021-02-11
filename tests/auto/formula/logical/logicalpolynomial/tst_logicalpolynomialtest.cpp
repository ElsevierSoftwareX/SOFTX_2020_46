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

#include "../../../../../core/utils/message.hpp"
#include "../../../../../core/utils/string_utils.hpp"
#include "../../../../../core/formula/formula_utils.hpp"

#include "../../../../../core/formula/logical/lpolynomial.hpp"


using namespace std;
using namespace lg;
using namespace formula;

class LogicalpolynomialTest : public QObject
{
	Q_OBJECT

public:
	LogicalpolynomialTest();
private:
	std::unordered_map<std::string, int> map;
	std::unordered_map<std::string, std::string> smap;

private Q_SLOTS:
    void fromStringTest3();
//	void fromStringTest2();
	void fromStringTest();
	void testCheckNextToParenthesis();
	void testActualExample();
	void testLpolyIntConstruct();
	void testLpolyStrConstruct();
};

LogicalpolynomialTest::LogicalpolynomialTest() {
	map = std::unordered_map<std::string, int> {
			{"1", 1}, {"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6}, {"7", 7}, {"8", 8}, {"9", 9}, {"10", 10},
			{"-1", -1}, {"-2", -2}, {"-3", -3}, {"-4", -4}, {"-5", -5}, {"-6", -6}, {"-7", -7}, {"-8", -8}, {"-9", -9}, {"-10", -10}
	};
	smap = std::unordered_map<std::string, std::string> {
			{"1", "1"}, {"2", "2"}, {"3", "3"}, {"4", "4"}, {"5", "5"}, {"6", "6"}, {"7", "7"}, {"8", "8"}, {"9", "9"}, {"10", "10"},
			{"-1", "-1"}, {"-2", "-2"}, {"-3", "-3"}, {"-4", "-4"}, {"-5", "-5"}, {"-6", "-6"}, {"-7", "-7"}, {"-8", "-8"}, {"-9", "-9"}, {"-10", "-10"}
    };
}

void LogicalpolynomialTest::fromStringTest3()
{
    std::unordered_map<std::string, int> convmap;
    for(int i = -22222; i < 22222; ++i) convmap.emplace(std::to_string(i), i);
    std::string str1 = " ( 2 1036 -1407 -1405 -11110 11113 -11114 11117 (#(2 1036 -1408 -1406 -11111 11112 -11115 11116)) )";
    auto eq1 = LogicalExpression<int>::fromString(str1, convmap);
    mDebug() << "eqstr===" << eq1.toString();
}

//void LogicalpolynomialTest::fromStringTest2()
//{
//	std::unordered_map<std::string, int> convmap;
//	for(int i = -1003; i < 500; ++i) convmap.emplace(std::to_string(i), i);
//	std::string str1 = "203 -204 223 -224 212 -219 #(209 -210 223 -224 216 -219) #(209 -210 223 -224 212 -215)";
//	auto eq1 = LogicalExpression<int>::fromString(str1, convmap);
//	//mDebug() << "eqstr===" << eq1.toString();
//}

// surface complimentが入れ子になっている場合
void LogicalpolynomialTest::fromStringTest()
{
	std::string str1 = "1 #(2)";
	std::string expect1 = "1 -2";
	std::string result1 = LogicalExpression<std::string>::fromString(str1).toString();
//	mDebug() << "result1 ===" << result1;
//	mDebug() << "expect1 ===" << expect1;
	QCOMPARE(expect1, result1);

	std::string str2 = "#(1:#(2))";
	std::string expect2 = "-1 2";
	std::string result2 = LogicalExpression<std::string>::fromString(str2).toString();
//	mDebug() << "result2 ===" << result2;
//	mDebug() << "expect2 ===" << expect2;
	QCOMPARE(expect2, result2);

	std::string str3 = "#(2:(-3))";
	std::string expect3 = "-2 3";
	std::string result3 = LogicalExpression<std::string>::fromString(str3).toString();
//	mDebug() << "result3 ===" << result3;
//	mDebug() << "expect3 ===" << expect3;
	QCOMPARE(expect3, result3);

	// そもそも #(-1:(-2 -3))のコンプリメントは？
	// → #(-1) #(-2 -3) → 1 (2:3) が正解
	std::string str4 = "#(-1:-2 -3)";
	std::string expect4 = "(2:3) 1";
	std::string result4 = LogicalExpression<std::string>::fromString(str4).toString();
//	mDebug() << "result4 ===" << result4;
//	mDebug() << "expect4 ===" << expect4;
	QCOMPARE(expect4, result4);


	//#(1:#(2:(-3))) = #(1:(-2 3)) = #1 #(-2 3) = -1 (2：-3)
	std::string str5 = "#(1:#(2:(-3)))";
	std::string expect5 = "(2:-3) -1";
	std::string result5 = LogicalExpression<std::string>::fromString(str5).toString();
//	mDebug() << "result5 ===" << result5;
//	mDebug() << "expect5 ===" << expect5;
	QCOMPARE(expect5, result5);

	//(#(1:#(2:(-3))) 4):5 = (#(1:(-2 3)) 4):5 = (-1 (2:-3) 4):5
	std::string str6 = "(#(1:#(2:(-3))) 4):5";
	std::string expect6 = "((2:-3) -1 4:5)";
	std::string result6 = LogicalExpression<std::string>::fromString(str6).toString();
//	mDebug() << "result6 ===" << result6;
//	mDebug() << "expect6 ===" << expect6;
	QCOMPARE(expect6, result6);
}

void LogicalpolynomialTest::testCheckNextToParenthesis()
{
	// LoL, RoR Checkは正しく実装されなければならない。
	std::string str0 = "((2))((5:6))";
	std::string str1 = str0;
	// LoLチェック
	QVERIFY(formula::checkNextChar(str1, '(', " :(", true) != str1.cend());
	// LoLエラー修復
	fixOmittedOperator(&str1, ' ', " :(", " :)");
	QCOMPARE(str1, std::string("((2)) ((5:6))"));
	// LoLエラー修復済み文字列でのfromString実行
	auto lp1 = LogicalExpression<string>::fromString(str1);
	// LoLエラー自動修復を実装したので場合例外は出ない
	auto lp0 = LogicalExpression<string>::fromString(str0);
	QCOMPARE(lp0.toString(), lp1.toString());

	std::string str2 = "((2)) ((5:6))7 8";
	std::string str3 = str2;
	// RoRチェック  ここで返り値のiteratorは"7"を指すはず。
	auto it = formula::checkNextChar(str3, ')', " :)", false);
	QVERIFY(it != str3.cend());
	QCOMPARE(*it, '7');

	// RoRエラー修復
	fixOmittedOperator(&str3, ' ', " :(", " :)");
	QCOMPARE(str3, std::string("((2)) ((5:6)) 7 8"));
	// RoRエラー修復済み文字列でのfromString実行
	auto lp2 = LogicalExpression<string>::fromString(str3);
	// RORエラー自動修復を実装したので場合例外は出ない
	auto lp3 = LogicalExpression<string>::fromString(str2);
	QCOMPARE(lp2.toString(), lp3.toString());

	// 実サンプルテスト
	std::string str4 = "(-999 (901))(-11.2(-11.1  11))";
	auto lp4 = LogicalExpression<string>::fromString(str4);
	QCOMPARE(std::string("-999 901 -11.2 -11.1 11"), lp4.toString());
}

void LogicalpolynomialTest::testActualExample()
{
	LogicalExpression<int> lp0(999);  // コンパイルテスト用。
	string src1 = "-1 2";
	string exp1 = "(1:-2)";
	string src2 = "-31 ((901_t101))";
	string exp2 = "(31:-901_t101)";

	auto lp1 = LogicalExpression<string>::fromString(src1);
	QCOMPARE(lp1.toString(), src1);
	QCOMPARE(lp1.complimented().toString(), exp1);
	auto lp2 = LogicalExpression<string>::fromString(src2);
	QCOMPARE(lp2.complimented().toString(), exp2);
}

void LogicalpolynomialTest::testLpolyIntConstruct()
{
	// ##########  整数型
	// コンストラクタ
	LogicalExpression<int> lp1(1);                // 単項式 {1}
	LogicalExpression<int> lp2({1, 2, 3, -1});   // 単項式 {1,2,3,-1}
	LogicalExpression<int> lp3({lp1, lp2});      // 多項式 [{1}, {1,2,3,-1}]
	std::vector<int> uexpect1{1}, uexpect2{1, 2, 3}, uexpect3{1, 2, 3}; // uniqueFactorは符号を無視して正の方のみを返す。
	QCOMPARE(lp1.uniqueFactors(), uexpect1);
	QCOMPARE(lp2.uniqueFactors(), uexpect2);
	QCOMPARE(lp3.uniqueFactors(), uexpect3);
	// 加算
	LogicalExpression<int> lp4({3, 4, 5});
	lp2 += lp4;           // [{1,2,3,-1}, {3,4,5}]
	auto lp5 = lp1 + lp2;  // [{1}], [{1,2,3,-1}, {3,4,5}]
	std::vector<int> uexpect4{1, 2, 3, 4, 5};
	auto uexpect5 = uexpect4;
	QCOMPARE(lp2.uniqueFactors(), uexpect4);
	QCOMPARE(lp5.uniqueFactors(), uexpect5);
	// イコール演算
	QVERIFY(lp1 != lp2);
	QVERIFY(lp2 != lp3);
	QVERIFY(lp3 != lp4);
	QVERIFY(lp4 != lp5);
	QVERIFY(lp1 == lp1);
	QVERIFY(lp2 == lp2);
	QVERIFY(lp3 == lp3);
	QVERIFY(lp4 == lp4);
	QVERIFY(lp5 == lp5);
	// 文字列化
	std::string sexpect1("1"), sexpect2("(1 2 3 -1:3 4 5)"), sexpect3("(1:1 2 3 -1)"),
				sexpect4("3 4 5"), sexpect5("(1:(1 2 3 -1:3 4 5))");
	QCOMPARE(lp1.toString(map), sexpect1);
	QCOMPARE(lp2.toString(map), sexpect2);
	QCOMPARE(lp3.toString(map), sexpect3);
	QCOMPARE(lp4.toString(map), sexpect4);
	QCOMPARE(lp5.toString(map), sexpect5);
	// 文字列からの生成
	auto lp1r = LogicalExpression<int>::fromString(lp1.toString(map), map);
	QCOMPARE(lp1, lp1r);
	QCOMPARE(lp2, LogicalExpression<int>::fromString(lp2.toString(map), map));
	QCOMPARE(lp3, LogicalExpression<int>::fromString(lp3.toString(map), map));
	QCOMPARE(lp4, LogicalExpression<int>::fromString(lp4.toString(map), map));
	QCOMPARE(lp5, LogicalExpression<int>::fromString(lp5.toString(map), map));
	// 多項式はツリーの先からつくっていくので単項の中で因子と入れ子多項式混在する場合、
	// 入れ子多項式の方が先になる。順序変更しても同値なのだが、文字列としては別になる。
	// 入れ子深さが同じなら先に出たほうが先になる。
	std::vector<std::string> sourceStrs {
		"((1:2) 5:6)",      // OK
		"(-6:7) (9:10) 8",  // OK
		"((-6:7):(9:10))",  // OK
		"(1:2) (3:4)",      // OK
	};
	for(const auto &str: sourceStrs) {
		auto lp =LogicalExpression<int>::fromString(str, map);
//		mDebug() << "lpresult ===" << lp.toString(map);
//		mDebug() << "lpexpect ===" << str;
		QCOMPARE(lp.toString(map), str);
	}
	// コンプリメント
	std::vector<std::pair<std::string, std::string>> compSrc {
		{"1 2 -3", "(-1:-2:3)"},  // OK
		{"-1:2:3", "1 -2 -3"},    // OK
		{"((1:2) 5)", "(-1 -2:-5)"},     // OK
		{"(-6:7) (9:10) 8", "(6 -7:-9 -10:-8)"},  //OK
		{"((-6:7):(9:10))", "6 -7 -9 -10"},  //  OK
		{"(1:2) (3:4)", "(-1 -2:-3 -4)"},      //
	};
	for(const auto &strPair: compSrc) {
		auto lp =LogicalExpression<int>::fromString(strPair.first, map);
		auto lpcomp = lp.complimented();
//		mDebug() << "lpcomp ====" << lpcomp.toString(map);
//		mDebug() << "compexp ===" << strPair.second;
		QCOMPARE(lpcomp.toString(map), strPair.second);
	}
	// 括弧のミスマッチは例外発生
	QVERIFY_EXCEPTION_THROWN(LogicalExpression<int>::fromString("1 (2", map), std::invalid_argument);
	QVERIFY_EXCEPTION_THROWN(LogicalExpression<int>::fromString("1 ((2 3) 4))", map), std::invalid_argument);
// 演算子の省略はAND演算子で修復するようにしたので下はobsolete
//	// 「左括弧の左側には演算子か左括弧」、「右括弧の右側には演算子か右括弧」これ以外は例外発生
//	QVERIFY_EXCEPTION_THROWN(LogicalExpression<int>::fromString("1(2)", map), std::invalid_argument);
//	QVERIFY_EXCEPTION_THROWN(LogicalExpression<int>::fromString("(2)1", map), std::invalid_argument);
//	QVERIFY_EXCEPTION_THROWN(LogicalExpression<int>::fromString("(1)(2)", map), std::invalid_argument);
}

void LogicalpolynomialTest::testLpolyStrConstruct()
{
	// ##########  文字列型
	// コンストラクタ
	LogicalExpression<string> lp1("1");                // 単項式 {1}
	LogicalExpression<string> lp2({"1", "2", "3", "-1"});   // 単項式 {1,2,3,-1}
	LogicalExpression<string> lp3({lp1, lp2});      // 多項式 [{1}, {1,2,3,-1}]
	std::vector<string> uexpect1{"1"}, uexpect2{"1", "2", "3"}, uexpect3{"1", "2", "3"};
	QCOMPARE(lp1.uniqueFactors(), uexpect1);
	QCOMPARE(lp2.uniqueFactors(), uexpect2);
	QCOMPARE(lp3.uniqueFactors(), uexpect3);
	// 加算
	LogicalExpression<string> lp4({"3", "4", "5"});
	lp2 += lp4;           // [{1,2,3,-1}, {3,4,5}]
	auto lp5 = lp1 + lp2;  // [{1}], [{1,2,3,-1}, {3,4,5}]
	std::vector<string> uexpect4{"1", "2", "3", "4", "5"};
	auto uexpect5 = uexpect4;
	QCOMPARE(lp2.uniqueFactors(), uexpect4);
	QCOMPARE(lp5.uniqueFactors(), uexpect5);
	// イコール演算
	QVERIFY(lp1 != lp2);
	QVERIFY(lp2 != lp3);
	QVERIFY(lp3 != lp4);
	QVERIFY(lp4 != lp5);
	QVERIFY(lp1 == lp1);
	QVERIFY(lp2 == lp2);
	QVERIFY(lp3 == lp3);
	QVERIFY(lp4 == lp4);
	QVERIFY(lp5 == lp5);
	// 文字列化
	std::string sexpect1("1"), sexpect2("(1 2 3 -1:3 4 5)"), sexpect3("(1:1 2 3 -1)"),
				sexpect4("3 4 5"), sexpect5("(1:(1 2 3 -1:3 4 5))");
	QCOMPARE(lp1.toString(smap), sexpect1);
	QCOMPARE(lp2.toString(smap), sexpect2);
	QCOMPARE(lp3.toString(smap), sexpect3);
	QCOMPARE(lp4.toString(smap), sexpect4);
	QCOMPARE(lp5.toString(smap), sexpect5);
	QCOMPARE(lp1.toString(), sexpect1);
	QCOMPARE(lp2.toString(), sexpect2);
	QCOMPARE(lp3.toString(), sexpect3);
	QCOMPARE(lp4.toString(), sexpect4);
	QCOMPARE(lp5.toString(), sexpect5);

	// 文字列からの生成
	auto lp1r = LogicalExpression<string>::fromString(lp1.toString(smap), smap);
	QCOMPARE(lp1, lp1r);
	QCOMPARE(lp2, LogicalExpression<string>::fromString(lp2.toString(smap), smap));
	QCOMPARE(lp3, LogicalExpression<string>::fromString(lp3.toString(smap), smap));
	QCOMPARE(lp4, LogicalExpression<string>::fromString(lp4.toString(smap), smap));
	QCOMPARE(lp5, LogicalExpression<string>::fromString(lp5.toString(smap), smap));
	// 多項式はツリーの先からつくっていくので単項の中で因子と入れ子多項式混在する場合、
	// 入れ子多項式の方が先になる。順序変更しても同値なのだが、文字列としては別になる。
	// 入れ子深さが同じなら先に出たほうが先になる。
	std::vector<std::string> sourceStrs {
		"((1:2) 5:6)",      // OK
		"(-6:7) (9:10) 8",  // OK
		"((-6:7):(9:10))",  // OK
		"(1:2) (3:4)",      // OK
	};
	for(const auto &str: sourceStrs) {
		auto lp =LogicalExpression<string>::fromString(str, smap);
//		mDebug() << "lpresult ===" << lp.toString(map);
//		mDebug() << "lpexpect ===" << str;
		QCOMPARE(lp.toString(smap), str);
	}
	// コンプリメント
	std::vector<std::pair<std::string, std::string>> compSrc {
		{"1 2 -3", "(-1:-2:3)"},  // OK
		{"-1:2:3", "1 -2 -3"},    // OK
		{"((1:2) 5)", "(-1 -2:-5)"},     // OK
		{"(-6:7) (9:10) 8", "(6 -7:-9 -10:-8)"},  //OK
		{"((-6:7):(9:10))", "6 -7 -9 -10"},  //  OK
		{"(1:2) (3:4)", "(-1 -2:-3 -4)"},      //
	};
	for(const auto &strPair: compSrc) {
		auto lp =LogicalExpression<string>::fromString(strPair.first, smap);
		auto lpcomp = lp.complimented();
//		mDebug() << "lpcomp ====" << lpcomp.toString(map);
//		mDebug() << "compexp ===" << strPair.second;
		QCOMPARE(lpcomp.toString(smap), strPair.second);
	}
	// 括弧のミスマッチは例外発生
	QVERIFY_EXCEPTION_THROWN(LogicalExpression<string>::fromString("1 (2"), std::invalid_argument);
	QVERIFY_EXCEPTION_THROWN(LogicalExpression<string>::fromString("1 ((2 3) 4))"), std::invalid_argument);
	// 演算子の省略はAND演算子で修復するようにしたので下はobsolete
//	// 「左括弧の左側には演算子か左括弧」、「右括弧の右側には演算子か右括弧」これ以外は例外発生
//	QVERIFY_EXCEPTION_THROWN(LogicalExpression<string>::fromString("1(2)"), std::invalid_argument);
//	QVERIFY_EXCEPTION_THROWN(LogicalExpression<string>::fromString("(2)1"), std::invalid_argument);
//	QVERIFY_EXCEPTION_THROWN(LogicalExpression<string>::fromString("(1)(2)"), std::invalid_argument);

}






QTEST_APPLESS_MAIN(LogicalpolynomialTest)

#include "tst_logicalpolynomialtest.moc"
