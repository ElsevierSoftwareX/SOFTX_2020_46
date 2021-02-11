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

#include <iostream>
#include <regex>
#include <string>

#include "../../../../../core/input/mcmode.hpp"
#include "../../../../../core/input/phits/phits_metacards.hpp"
#include "../../../../../core/utils/message.hpp"
#include "../../../../../core/utils/string_utils.hpp"
#include "../../../../../core/input/common/commoncards.hpp"

using namespace std;
using namespace inp;

class PhitsMetaCardsTest : public QObject
{
	Q_OBJECT

public:
    PhitsMetaCardsTest();

private Q_SLOTS:
	void includePatternTest();
	void lineRangeTest();
	void sectionPatternTest();
	void sectionNameTest();
	void commentPattern();
};

PhitsMetaCardsTest::PhitsMetaCardsTest() {}

void PhitsMetaCardsTest::includePatternTest()
{
	const string str1 = "infl:{filename}[1-2]";
	const string str2 = "infl: {  file_name1  }[  1  -  2  ]";
	const string str3 = "infl:{surfinc2.mcn}[1-2]";
	const string str4 = "infl: {  file_name1  } [  - 2   ]";
	const string str5 = "infl: {  file_name1  } [1-   ]";
	const string str6 = "infl: {  file_name1  } [-]";
	// 行レンジは別のカードには含めないから中括弧まで合っていればinflカードと認める。

	std::smatch sm;
	QVERIFY(std::regex_search(str1, sm, phits::getIncludeCardPattern()));
	QVERIFY(std::regex_search(str2, sm, phits::getIncludeCardPattern()));
	QVERIFY(std::regex_search(str3, sm, phits::getIncludeCardPattern()));
	QVERIFY(std::regex_search(str4, sm, phits::getIncludeCardPattern()));
	QVERIFY(std::regex_search(str5, sm, phits::getIncludeCardPattern()));
	QVERIFY(std::regex_search(str6, sm, phits::getIncludeCardPattern()));

//	mDebug() << "matchlen=" << sm.size();
//	mDebug() << "match=" << sm.str(1);
//	mDebug() << "matchsuff=" << sm.suffix();
}

void PhitsMetaCardsTest::lineRangeTest()
{
	QCOMPARE(phits::getFileRange("[1-100]"), std::make_pair(size_t(1), size_t(100)));
	QCOMPARE(phits::getFileRange("  [  1  -   100 ]  "), std::make_pair(size_t(1), size_t(100)));
	QCOMPARE(phits::getFileRange("  [-100]  "), std::make_pair(size_t(0), size_t(100)));
	QCOMPARE(phits::getFileRange("  [100-]  "), std::make_pair(size_t(100), inp::phits::MAX_LINE_NUMBER));

	// 不適切な入力なので例外発生
	QVERIFY_EXCEPTION_THROWN(phits::getFileRange("  [0 100]  "), std::invalid_argument);
	QVERIFY_EXCEPTION_THROWN(phits::getFileRange("  []  "), std::invalid_argument);
}

void PhitsMetaCardsTest::sectionPatternTest()
{
	/*
	 * ※注意 smatchはマッチ部分の文字列を複製するわけではなく、検索元へのポインタを持っているだけだから
	 *        検索元文字列の寿命が尽きたらstr()の結果は不定になる！
	 */
	std::smatch sm1;
	string str1 = "[  c e l L]";
	QVERIFY(std::regex_search(str1, sm1, phits::getSectionPattern()));
	QVERIFY(sm1.size() == 2u);
	QCOMPARE(sm1.str(1), string("c e l L"));

	std::smatch sm0;
	std::string str = "[cell]";
	QVERIFY(std::regex_search(str, sm0, phits::getSectionPattern()));
	// section表現としてダメな例
	std::string badstr1 = "      [cell]", badstr2 = "[cel?l]";
	QVERIFY(!std::regex_search(badstr1, sm0, phits::getSectionPattern()));
	QVERIFY(!std::regex_search(badstr2, sm0, phits::getSectionPattern()));
}

void PhitsMetaCardsTest::sectionNameTest()
{
	string str1 = "    c e l L  ";
	phits::canonicalizeSectionName(&str1);
	QCOMPARE(str1, string("cell"));
}

void PhitsMetaCardsTest::commentPattern()
{
	std::cmatch cm;
	QVERIFY(std::regex_search("c this is commentline", cm, inp::comm::getPreCommentPattern()));
	QVERIFY(std::regex_search("c これはコメント", cm, inp::comm::getPreCommentPattern()));
	QVERIFY(std::regex_search("c ", cm, inp::comm::getPreCommentPattern()));
	QVERIFY(std::regex_search("c", cm, inp::comm::getPreCommentPattern()));
	QVERIFY(!std::regex_search("c101 1 -1.0 -1", cm, inp::comm::getPreCommentPattern()));
	QVERIFY(!std::regex_search("cell1 1 -1.0 -1", cm, inp::comm::getPreCommentPattern()));
}

QTEST_APPLESS_MAIN(PhitsMetaCardsTest)

#include "tst_phits_metacardstest.moc"
