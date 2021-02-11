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

#include <random>

#include "core/utils/system_utils.hpp"
#include "core/utils/message.hpp"

class system_util : public QObject
{
	Q_OBJECT

public:
	system_util();
	~system_util();

private slots:
	void testReserved();
	void testValidName1();

};

system_util::system_util() {;}
system_util::~system_util() {;}




void system_util::testReserved()
{
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
	std::random_device seed_gen;
	std::mt19937 engine(seed_gen());

	// 予約語そのもの
	std::vector<std::string> reserved1 {
		"aux", "com", "con", "lpt", "nul", "prn"
	};
	for(const auto& name: reserved1) {
		auto res = utils::toValidEachFileName(name, true);
		mDebug() << "name===" << name;
		mDebug() << "res ===" << res;
		QVERIFY(name != res);
	}

	// 予約語+数字
	auto reserved2 = reserved1;
	for(auto& elem: reserved2) elem += std::to_string(engine() % 10);
	for(const auto& name: reserved2) {
		auto res = utils::toValidEachFileName(name, true);
		QVERIFY(name != res);
	}

	// 予約語+数字+拡張子
	auto reserved3 = reserved2;
	for(auto& elem: reserved3) elem += ".stl";
	for(const auto& name: reserved3) {
		auto res = utils::toValidEachFileName(name, true);
		QVERIFY(name != res);
	}

	// 予約語+拡張子
	auto reserved4 = reserved1;
	for(auto& elem: reserved4) elem += ".stl";
	for(const auto& name: reserved4) {
		auto res = utils::toValidEachFileName(name, true);
		QVERIFY(name != res);
	}
#endif

}

// ファイルシステム的に受け付けない,あるいはそういう名前にしないほうが良いファイル名を解明する。
void system_util::testValidName1()
{
	std::vector<std::string> names{"1<2<3", "cell<aaa[1,2,-1]", "a@bcd___asd>2"};
	for(const auto& name: names) {
		auto res = utils::toValidEachFileName(name);
//		mDebug() << "src   ===" << name;
		mDebug() << "result ==" << res;
		QVERIFY(name != res);
		QVERIFY(res.find_first_of("<>,") == std::string::npos);
	}

}

QTEST_APPLESS_MAIN(system_util)

#include "tst_system_util.moc"
