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
#include <cstring>
#include <sstream>
#include <unordered_map>
#include "core/math/constants.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/system_utils.hpp"
#include "core/utils/numeric_utils.hpp"
#include "core/utils/message.hpp"


class String_testTest : public QObject
{
	Q_OBJECT

public:
	String_testTest();

private Q_SLOTS:
	void testCanonicalName();
	void testDequote();
	void testSplitSpaces();
	void testSplitNestedQuote();
	void testCase0();
	void testUnivsesalSplit();
	void testEncoding();
	void testSplitInputParams();
	void testSeparatePath();
	void testStringVectorTo();
	void testConversion();
	void testIsNumberVector();
	void testIsNumber();
	void testTrim();
	void testSplit();

};

String_testTest::String_testTest() {;}

void String_testTest::testCanonicalName()
{
	std::string str1 = "6.1";
	auto result1 = utils::canonicalName(str1);
	std::string expect1 = "6.1";
//	mDebug() << "result1 ===" << result1;
//	mDebug() << "expect1 ===" << expect1;
	QCOMPARE(result1, expect1);

	std::string str2 = "sp6.5";
	auto result2 = utils::canonicalName(str2);
	std::string expect2 = str2;
//	mDebug() << "result ===" << result2;
//	mDebug() << "expect ===" << expect2;
	QCOMPARE(result2, expect2);

	std::string str3 = "010120.5";
	auto result3 = utils::canonicalName(str3);
	std::string expect3 = "10120.5";
	QCOMPARE(result3, expect3);

	std::string str4 = "010aaa120.5";
	auto result4 = utils::canonicalName(str4);
	std::string expect4 = str4;
	QCOMPARE(result4, expect4);

	std::string str5 = "1500";
	auto result5 = utils::canonicalName(str5);
	std::string expect5 = "1500";
//	mDebug() << "result ===" << result5;
//	mDebug() << "expect ===" << expect5;
	QCOMPARE(result5, expect5);

}

void String_testTest::testDequote()
{
	// std::string uctest = u8"あいうえ";
	// uctest.size()は文字数ではなくバイト数を返す。
	// uctest.at()はその位置の文字1バイト分を返すのでひらがなは表現できない
	// uctest.find(u8"う")はnposではなくヒットしたバイト位置=6を返す。


	// (a+b) → a+b
	const std::string expected1 = "a + b";
	auto result1 = utils::dequote(std::unordered_map<char, char>{{'(', ')'}}, "( " + expected1 + ")  ", true);
	QCOMPARE(result1, expected1);

	// ((a+b)) → a+b
	const std::string expected12 = "a + b";
	auto result12 = utils::dequote(std::unordered_map<char, char>{{'(', ')'}}, "( (" + expected12 + "))  ", true);
	QCOMPARE(result12, expected12);


	// (a)+(b) → (a)+(b) そのまま
	const std::string expected2 = "(a)+(b)";
	auto result2 = utils::dequote(std::unordered_map<char, char>{{'(', ')'}},  expected2, true);
//	mDebug() << "expected2 ===" << expected2;
//	mDebug() << "results2 ====" << result2;
	QCOMPARE(result2, expected2);

	// #########  左右の区別がない引用符の場合
	// "a"+"b" → "a"+"b" そのまま
	const std::string expected3 = "\"a\"+\"b\"";
	auto result3 = utils::dequote('\"',  expected3, true);
//	mDebug() << "expected3 ===" << expected3;
//	mDebug() << "results3 ====" << result3;
	QCOMPARE(result3, expected3);

	// "a+b" → a+b
	const std::string expected4 = "a+b";
	auto result4 = utils::dequote('\"',  "\"" + expected4 + "\"", true);
//	mDebug() << "expected4 ===" << expected4;
//	mDebug() << "results4 ====" << result4;
	QCOMPARE(result4, expected4);

}

void String_testTest::testSplitSpaces()
{
	std::string spaces = "    ,   ";
	std::vector<std::string> expect{"    ", "   "};
	auto result = utils::splitString(",", spaces, false);
//	mDebug() << "result=" << result;
//	mDebug() << "expect=" << expect;
	QCOMPARE(result.size(), expect.size());
	for(size_t i = 0; i < result.size(); ++i) {
		QCOMPARE(result.at(i), expect.at(i));
	}
}

void String_testTest::testSplitNestedQuote()
{
	std::string source0 = "(a*b*(c*d)*e)*f*g*h";
	std::vector<std::string> expect0{"(a*b*(c*d)*e)", "f", "g", "h"};
	auto result0 = utils::splitString(std::make_pair('(', ')'), "*", source0, true);
//	mDebug() << "source0 = " << source0;
//	mDebug() << "expect0 = " << expect0;
//	mDebug() << "result0 = " << result0;
	QCOMPARE(result0, expect0);
}


void String_testTest::testCase0()
{
	std::string str1 = "-999 ((1.1:1.2:1.3:1.4:1.5:1.6))";
	std::unordered_map<char, char> quotes{{'"', '"'}, {'{', '}'}, {'(', ')'}};
	auto res = utils::splitString(quotes, " ", str1, true);
	std::vector<std::string> expect{"-999", "((1.1:1.2:1.3:1.4:1.5:1.6))"};
	//mDebug() << "res=" << res;
	QCOMPARE(res.size(), expect.size());
	for(size_t i = 0; i < res.size(); ++i) {
		QCOMPARE(res.at(i), expect.at(i));
	}

	// 文字列「" ab) (cd " {a+b c} ((1+2 +{3)) (4} 5 :6)」
//	std::string str2 = "\" ab) (cd \" {a+b c} ((1+2 +{3)) (4} 5 :6)";
	std::string str2 = R"(" ab) (cd " {a+b c} ((1+2 +{3)) (4} 5 :6))";

	std::unordered_map<char, char> qmarkPairs{{'"', '"'}, {'(', ')'}, {'{', '}'}};
	auto result2 = utils::splitString(qmarkPairs, " ", str2, true);
	std::vector<std::string> expect2{"\" ab) (cd \"", "{a+b c}", "((1+2 +{3))", "(4} 5 :6)"};
//	mDebug() << "source2=" << str2;
//	mDebug() << "result2=" << result2;
//	mDebug() << "expect2=" << expect2;
	QCOMPARE(result2.size(), expect2.size());
	for(size_t i = 0; i < expect2.size(); ++i) {
		QCOMPARE(result2.at(i), expect2.at(i));
	}
}


void String_testTest::testUnivsesalSplit()
{
	std::unordered_map<char, char> qmarkMap {{'"', '"'}, {'(', ')'}};

	std::string source0 = "a*b**c*";
	auto result0 = utils::splitString(qmarkMap, "*", source0, false);
	std::vector<std::string> expected0{"a", "b", "", "c", ""};
//	mDebug() << "source0=" << source0;
//	mDebug() << "result0=" << result0;
//	mDebug() << "expect0=" << expected0;
	QCOMPARE(result0, expected0);

	std::string source1 = "ab=cd*e=*f*";
	auto result1 = utils::splitString(qmarkMap, "=*", source1, false);
	std::vector<std::string> expected1{"ab", "cd", "e", "", "f", ""};
//	mDebug() << "\nsource1=" << source1;
//	mDebug() << "result1=" << result1;
//	mDebug() << "expect1=" << expected1;
	QCOMPARE(result1, expected1);

	std::string source2 = "ab=cd*e=*=f*";
	auto result2 = utils::splitString(qmarkMap, "=*", source2, true);
	std::vector<std::string> expected2{"ab", "cd", "e", "f"};
//	mDebug() << "\nsource2=" << source2;
//	mDebug() << "result2=" << result2;
//	mDebug() << "expect2=" << expected2;
	QCOMPARE(result2, expected2);

	std::string source3 = "ab=\"cd*e\"=*=f*";
	auto result3 = utils::splitString(qmarkMap, "=*", source3, true);
	std::vector<std::string> expected3{"ab", "\"cd*e\"", "f"};
//	mDebug() << "\nsource3=" << source3;
//	mDebug() << "result3=" << result3;
//	mDebug() << "expect3=" << expected3;
	QCOMPARE(result3, expected3);

	std::string source4 = R"(ab="cd*e   =a"  ="f = x")";
	QCOMPARE(utils::splitString(qmarkMap, " =", source4, true), utils::splitString(qmarkMap, "= ", source4, true));
	auto result4 = utils::splitString(qmarkMap, " =", source4, true);
	std::vector<std::string> expected4{"ab", "\"cd*e   =a\"", "\"f = x\""};
//	mDebug() << "\nsource4=" << source4;
//	mDebug() << "result4=" << result4;
//	mDebug() << "expect4=" << expected4;
	QCOMPARE(result4, expected4);
}



void String_testTest::testEncoding()
{
	if(auto dp = std::getenv("DATAPATH")) {
		(void)dp;
	   std::string datapath = std::getenv("DATAPATH");
//	   qDebug() << "datapath=" << QString::fromStdString(datapath);
//	   qDebug() << "utf8=" << QString::fromStdString(utils::utf8ToSystemEncoding(datapath));
//	   qDebug() << "utf8tosjis=" << QString::fromStdString(utils::utf8ToSystemEncoding(utils::systemEncodingToUtf8(datapath)));
	   QCOMPARE(datapath, utils::utf8ToSystemEncoding(utils::systemEncodingToUtf8(datapath)));

	   std::string u8filename = utils::systemEncodingToUtf8(datapath) + "\\" + "mcplib84";
//	   std::cout << "acefile(u8)=======" << u8filename << std::endl;
//	   std::cout << "acefile(sjis)=====" << utils::utf8ToSystemEncoding(u8filename)<< std::endl;
	   QCOMPARE(u8filename, utils::systemEncodingToUtf8(utils::utf8ToSystemEncoding(u8filename)));
	}
}


void String_testTest::testSplitInputParams()
{
	std::string str1 = "1 0    (-1.1 -1.2 -1.3 -1.4 -1.5 -1.6)";
	std::unordered_map<char, char> quotes{{'"', '"'}, {'{', '}'}, {'(', ')'}};
	auto res = utils::splitString(quotes, " ", str1, true);
	std::vector<std::string> expect{"1", "0", "(-1.1 -1.2 -1.3 -1.4 -1.5 -1.6)"};
	QCOMPARE(res.size(), expect.size());
	for(size_t i = 0; i < res.size(); ++i) {
		QCOMPARE(res.at(i), expect.at(i));
	}
}



void String_testTest::testSeparatePath()
{
	using namespace std;
	using namespace utils;
	string str1 = "/usr/bin/gcc";
	string str2 = "root/.emacs.d/.emacs.el";
	string str3 = "/etc/";
	string str4 = "/";
	string str5 = "file";
	string str6 = "/c/cygdrive/program files/MSOFFICE/office.exe";
	QCOMPARE(separatePath(str1), (std::pair<string, string>("/usr/bin/", "gcc")));
	QCOMPARE(separatePath(str2), (std::pair<string, string>("root/.emacs.d/", ".emacs.el")));
	QCOMPARE(separatePath(str3), (std::pair<string, string>("/etc/", "")));
	QCOMPARE(separatePath(str4), (std::pair<string, string>("/", "")));
	QCOMPARE(separatePath(str5), (std::pair<string, string>("", "file")));
	QCOMPARE(separatePath(str6), (std::pair<string, string>("/c/cygdrive/program files/MSOFFICE/", "office.exe")));
}

void String_testTest::testStringVectorTo()
{
	std::vector<std::string> elements = {"10", "S", "0", "0", "5", "5"};
	std::vector<double> result = utils::stringVectorTo<double>(elements, 2, 4);
//	mDebug() << "elements=" << elements;
//	mDebug() << "result=" << result;
	std::vector<double> expect{0, 0, 5, 5};
	QCOMPARE(result.size(), expect.size());
	for(size_t i = 0; i < result.size(); ++i) {
		QVERIFY(utils::isSameDouble(result.at(i), expect.at(i)));
	}
}


void String_testTest::testConversion()
{
	std::string str1 = "1.0", str2 = "1.12E+03", str3 = "1.50+4", str4 = "1.5E+05abcd3", str5 = "sdf1.5";

	QCOMPARE(utils::stringTo<double>(str1), 1.0);
	QCOMPARE(utils::stringTo<double>(str2), 1.12E+3);
	QCOMPARE(utils::stringTo<double>(str3), 15000.0);
	QVERIFY_EXCEPTION_THROWN(utils::stringTo<double>(str4), std::invalid_argument);
	QVERIFY_EXCEPTION_THROWN(utils::stringTo<double>(str5), std::invalid_argument);

	QCOMPARE(utils::stringTo<int>(std::string("01")), 1);
	QCOMPARE(utils::stringTo<int>(str1), 1);
	QCOMPARE(utils::stringTo<int>(str2), 1120);
	QCOMPARE(utils::stringTo<int>(str3), 15000);
	QVERIFY_EXCEPTION_THROWN(utils::stringTo<int>(str4), std::invalid_argument);
	QVERIFY_EXCEPTION_THROWN(utils::stringTo<int>(str5), std::invalid_argument);


	std::vector<std::string> vec1{str1, str2, str3, str4, str5};
	QVERIFY_EXCEPTION_THROWN(utils::stringVectorTo<int>(vec1), std::invalid_argument);

	std::vector<std::string> vec2(vec1.begin(), vec1.begin()+3);
	std::vector<int> expected{1, 1120, 15000};
	QCOMPARE(utils::stringVectorTo<int>(vec2), expected);
	QCOMPARE(utils::stringVectorTo<int>(vec2, 0, vec2.size()), expected);

	expected = {1, 1120};
	QCOMPARE(utils::stringVectorTo<int>(vec2, 0, 2), expected);
	//QCOMPARE(utils::stringVectorTo<int>(vec2.begin(), vec2.begin()+2), expected);
	QVERIFY_EXCEPTION_THROWN(utils::stringVectorTo<int>(vec2, 0, vec2.size()+1), std::out_of_range);


}

void String_testTest::testIsNumberVector()
{
	using namespace std;
	vector<string> strVec {"0", "0", "10"};
	QCOMPARE(utils::isArithmeticVector(strVec), true);
	vector<string> strVec2 {"TR1", "0", "0", "10"};
	QCOMPARE(utils::isArithmeticVector(strVec2.begin()+1, strVec2.end()), true);
}


//#define STODTEST(ARG) "stod"#ARG << std::stod(ARG)
void String_testTest::testIsNumber()
{
	QVERIFY(utils::isArithmetic("1.5E+06#abc$") == false);
	QVERIFY(utils::isArithmetic("1.5E+06")      == true);
	QVERIFY(utils::isArithmetic(".1.5E+06")     == false);
	QVERIFY(utils::isArithmetic("   1.5E+06   ")== true);
	QVERIFY(utils::isArithmetic("abc1.5E+06   ")== false);
	QVERIFY(utils::isArithmetic("+1.5+06")      == true);
	QVERIFY(utils::isArithmetic("+0.5-06")      == true);
	QVERIFY(utils::isArithmetic("-1.50")        == true);
	QVERIFY(utils::isArithmetic("1.50")         == true);
	QVERIFY(utils::isArithmetic("  1.50  ")     == true);
	QVERIFY(utils::isArithmetic("abc")          == false);
	QVERIFY(utils::isArithmetic("1.5sdff")      == false);
	QVERIFY(utils::isArithmetic("1.5.1")        == false);
	QVERIFY(utils::isArithmetic("#aaa1.5")      == false);
	QVERIFY(utils::isArithmetic(".5")           == true);
	QVERIFY(utils::isArithmetic(".5aaa")        == false);
	QVERIFY(utils::isArithmetic("aaa.5aaa")     == false);
	QVERIFY(utils::isArithmetic("aaa5aaa")      == false);
	QVERIFY(utils::isArithmetic("5aaa")         == false);
	QVERIFY(utils::isArithmetic("aaa5")         == false);
	QVERIFY(utils::isArithmetic("5")            == true);
}

void String_testTest::testTrim()
{
	std::string str{"   abc   ddd 123    "};
	utils::trim(&str);
	QVERIFY(str == std::string("abc   ddd 123"));

	std::string str2{"e-type "};
	utils::trim(&str2);
	QVERIFY(str2 == std::string("e-type"));

	std::string str3{"no"};
	utils::trim(&str3);
	QVERIFY(str3 == std::string("no"));

}

void String_testTest::testSplit()
{
	std::string str{"   abc   ddd 123    "};
	std::vector<std::string> answer{"abc", "ddd", "123"};
	auto splitted =  utils::splitString(" ", str, true);
	//mDebug() << "splited vec(not ignore empty)=" << utils::split(' ', str, false);
	//mDebug() << "splited vec=" << splitted;

	QVERIFY(splitted.size() == answer.size());
	for(std::size_t i = 0; i < answer.size(); i++) {
		QVERIFY(splitted.at(i) == answer.at(i));
	}
}



QTEST_APPLESS_MAIN(String_testTest)

#include "tst_stringtest.moc"
