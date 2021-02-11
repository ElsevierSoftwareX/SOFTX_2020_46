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
#include <QCryptographicHash>
#include <QFile>
#include <QDebug>

#include <cstdlib>
#include <string>

#include "core/io/input/inputdata.hpp"
#include "core/io/input/common/commoncards.hpp"
#include "core/utils/message.hpp"



using namespace std;

namespace {

//std::string GetHash(const std::string& fileName) {
//	QCryptographicHash hash(QCryptographicHash::Sha1);
//	QFile file(QString::fromStdString(fileName));
//	if(!file.open(QFile::ReadOnly)) {
//		qFatal(QString::fromStdString(std::string("No such a file=") + fileName).toLatin1());
//	}
//	hash.addData(file.readAll());

//	return hash.result().toStdString();
//}

const char TESTDATADIR[] = "tests/auto/input/inputdata";
}


class Inputdata_testTest : public QObject
{
	Q_OBJECT

public:
	Inputdata_testTest();

private Q_SLOTS:
	void testComment();
	void testConstants();
//	void testMcnpInp();
	void testPhitsInp();
};

Inputdata_testTest::Inputdata_testTest() {;}

void Inputdata_testTest::testComment()
{
	std::string str1("READ file=sdf $ aaaaabd $ ");
	inp::comm::removeMcnpPostComment(&str1);
	//std::cout << "result=\"" << str1 << "\"" << std::endl;
	QCOMPARE(str1, std::string("READ file=sdf "));

	std::string str2("infl:{aa} ! aaaaabd $ ");
	inp::comm::removePhitsPostCommentNotSharp(&str2);
	//std::cout << "result=\"" << str2 << "\"" << std::endl;
	QCOMPARE(str2, std::string("infl:{aa} "));

	std::string str3("%infl:{aa} ! aaaaabd $ ");
	inp::comm::removePhitsPostCommentNotSharp(&str3);
	//std::cout << "result=\"" << str3 << "\"" << std::endl;
	QCOMPARE(str3, std::string(""));

	std::smatch sm;
	std::string str4("       c infl:{aa ! a $");  // コメントではない
	std::string str5("c infl:{aa} ! aaaaabd $");  // コメント
//	std::string str6("c * extended input $");     // コメントではない
	std::string str7("c");                        // コメント

	QVERIFY(!std::regex_search(str4 , sm, inp::comm::getPreCommentPattern()));
	QVERIFY(std::regex_search(str5 , sm, inp::comm::getPreCommentPattern()));
//	QVERIFY(!std::regex_search(str6 , sm, inp::comm::getPreCommentPattern()));
	QVERIFY(std::regex_search(str7 , sm, inp::comm::getPreCommentPattern()));
}

void Inputdata_testTest::testConstants()
{
	std::smatch sm;
	std::string str1 = "c1";
	std::string str2 = "c2";
	std::string str3 = "c11";
	std::string str4 = "c14";
	std::string str5 = "c100";
	std::regex re(R"([cC]\d{1,2}(\D|$))");
	QVERIFY(std::regex_search(str1, sm, re));
	QVERIFY(std::regex_search(str2, sm, re));
	QVERIFY(std::regex_search(str3, sm, re));
	QVERIFY(std::regex_search(str4, sm, re));
	QVERIFY(!std::regex_search(str5, sm, re));
}


//void Inputdata_testTest::testMcnpInp()
//{
//	const string DATADIR = string(TESTDATADIR) + "/mcnpinp";
//	// include処理、comment削除, 連結、ijmr展開
//	const string INPUT_FILE    = DATADIR + "/test.mcn";
//	const string EXPECTED_FILE = DATADIR + "/expected.mcn";
//	const string RESULT_FILE   = DATADIR + "/result.mcn";
//	inp::InputData input(INPUT_FILE);
//    std::ofstream ofs(RESULT_FILE);
//    input.dumpData(ofs);
//	QCOMPARE(GetHash(RESULT_FILE), GetHash(EXPECTED_FILE));

//	// cell, surf, data切り分け
//	const string RESULT_CELL = DATADIR + "/result.cell";
//	const string RESULT_SURF = DATADIR + "/result.surf";
//	const string RESULT_DATA = DATADIR + "/result.data";
//	const string EXPECT_CELL = DATADIR + "/expected.cell";
//	const string EXPECT_SURF = DATADIR + "/expected.surf";
//	const string EXPECT_DATA = DATADIR + "/expected.data";
//    std::ofstream cof(RESULT_CELL), sof(RESULT_SURF), dof(RESULT_DATA);
//    inp::InputData::dumpData(cof, input.cellCards());
//    inp::InputData::dumpData(sof, input.surfaceCards());
//    inp::InputData::dumpData(dof, input.dataCards());


//	QCOMPARE(GetHash(RESULT_CELL), GetHash(EXPECT_CELL));
//	QCOMPARE(GetHash(RESULT_SURF), GetHash(EXPECT_SURF));
//	QCOMPARE(GetHash(RESULT_DATA), GetHash(EXPECT_DATA));
//}

void Inputdata_testTest::testPhitsInp()
{
////	const string DATADIR = string(TESTDATADIR) + "/.";
//	const string DATADIR =  ".";
//	// include処理、comment削除, 連結、ijmr展開
//	const string INPUT_FILE   = DATADIR + "/test.phi";
//	const string EXPECTED_FILE = DATADIR + "/expected.phi";
//	const string RESULT_FILE   = DATADIR + "/result.phi";
//	inp::InputData input(INPUT_FILE);
//	std::ofstream of(RESULT_FILE);
//	input.dumpData(of);
//	QCOMPARE(GetHash(RESULT_FILE), GetHash(EXPECTED_FILE));
}

QTEST_APPLESS_MAIN(Inputdata_testTest)

#include "tst_inputdatatest.moc"
