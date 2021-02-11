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

#include<list>



#include "core/io/input/dataline.hpp"
#include "core/utils/utils.hpp"
#include "core/source/abstractdistribution.hpp"
#include "core/source/multigroupdistribution.hpp"
#include "core/source/proportionaldistribution.hpp"
#include "core/source/discretedistribution.hpp"
#include "core/source/phits/phitsenergydistribution.hpp"


using namespace src;
using namespace inp;


class EnergydistributionTest : public QObject
{
	Q_OBJECT

public:
	EnergydistributionTest();

private:
//	std::list<DataLine> sourceInput1_;
//	std::list<DataLine> sourceInput8_;
//	std::list<DataLine> badSourceInput1_;
//	std::list<DataLine> badSourceInput8_;

private Q_SLOTS:
//	void testEtype1();
//	void testEtype8();
//	void testBadEtype1();
//	void testBadEtype8();
};

EnergydistributionTest::EnergydistributionTest()
{
//	// 正常なtype1/4データ type 21/24としてもOK(21/24では微分値扱い)
//	sourceInput1_.push_back(DataLine("file1", 17,  "ne = 10" ));
//	sourceInput1_.push_back(DataLine("file1", 18,  "0.0E+00  6.62E-04" ));
//	sourceInput1_.push_back(DataLine("file1", 19,  "1.0E-04  6.62E-05" ));
//	sourceInput1_.push_back(DataLine("file1", 20,  "1.1E-04  6.62E-05" ));
//	sourceInput1_.push_back(DataLine("file1", 21,  "1.2E-04  6.62E-05" ));
//	sourceInput1_.push_back(DataLine("file1", 22,  "1.3E-04  6.62E-05" ));
//	sourceInput1_.push_back(DataLine("file1", 23,  "1.4E-04  6.62E-05" ));
//	sourceInput1_.push_back(DataLine("file1", 24,  "1.5E-04  6.62E-05" ));
//	sourceInput1_.push_back(DataLine("file1", 25,  "1.6E-04  1.32E-04" ));
//	sourceInput1_.push_back(DataLine("file1", 26,  "1.8E-04  1.32E-04" ));
//	sourceInput1_.push_back(DataLine("file1", 27,  "2.0E-04  1.32E-04" ));
//	sourceInput1_.push_back(DataLine("file1", 28,  "15" ));
//	// 正常なtype8/9データ
//	sourceInput8_.push_back(DataLine("file2", 17,  "ne = 10" ));
//	sourceInput8_.push_back(DataLine("file2", 18,  "0.0E+00  6.62E-04" ));
//	sourceInput8_.push_back(DataLine("file2", 19,  "1.0E-04  6.62E-05" ));
//	sourceInput8_.push_back(DataLine("file2", 20,  "1.1E-04  6.62E-05" ));
//	sourceInput8_.push_back(DataLine("file2", 24,  "1.5E-04  6.62E-05" )); // type8は必ずしも昇順でなくても良い
//	sourceInput8_.push_back(DataLine("file2", 21,  "1.2E-04  6.62E-05" ));
//	sourceInput8_.push_back(DataLine("file2", 22,  "1.2E-04  6.62E-05" )); // Eが同じデータが重複しても良い。警告はしたい。
//	sourceInput8_.push_back(DataLine("file2", 23,  "1.4E-04  6.62E-05" ));
//	sourceInput8_.push_back(DataLine("file2", 25,  "1.6E-04  1.32E-04" ));
//	sourceInput8_.push_back(DataLine("file2", 26,  "1.8E-04  1.32E-04" ));
//	sourceInput8_.push_back(DataLine("file2", 27,  "2.0E-04  1.32E-04" ));
//	// 異常なtype1/4データ
//	badSourceInput1_.push_back(DataLine("file3", 17,  "ne = 10" ));
//	badSourceInput1_.push_back(DataLine("file3", 18,  "0.0E+00  6.62E-04" ));
//	badSourceInput1_.push_back(DataLine("file3", 19,  "1.0E-04  6.62E-05" ));
//	badSourceInput1_.push_back(DataLine("file3", 20,  "1.1E-04  6.62E-05" ));
//	badSourceInput1_.push_back(DataLine("file3", 21,  "1.1E-04  6.62E-05" ));  // type1は群積分値なので群幅0で有限なpを持つデータは異常値
//	badSourceInput1_.push_back(DataLine("file3", 22,  "1.3E-04  6.62E-05" ));  //
//	badSourceInput1_.push_back(DataLine("file3", 23,  "1.4E-04  6.62E-05" ));
//	badSourceInput1_.push_back(DataLine("file3", 24,  "1.5E-04  6.62E-05" ));
//	badSourceInput1_.push_back(DataLine("file3", 25,  "1.6E-04  1.32E-04" ));
//	badSourceInput1_.push_back(DataLine("file3", 26,  "1.8E-04  1.32E-04" ));
//	badSourceInput1_.push_back(DataLine("file3", 27,  "2.0E-04  1.32E-04" ));
//	badSourceInput1_.push_back(DataLine("file3", 28,  "15" ));
//	// 異常なtype8/9データ
//	badSourceInput8_.push_back(DataLine("file4", 17,  "ne = 10" ));
//	badSourceInput8_.push_back(DataLine("file4", 18,  "0.0E+00  6.62E-04" ));
//	badSourceInput8_.push_back(DataLine("file4", 19,  "1.0E-04  6.62E-05" ));
//	badSourceInput8_.push_back(DataLine("file4", 20,  "-1.1E-04  6.62E-05" ));
//	badSourceInput8_.push_back(DataLine("file4", 21,  "1.2E-04  6.62E-05" ));
//	badSourceInput8_.push_back(DataLine("file4", 22,  "1.3E-04  6.62E-05" ));
//	badSourceInput8_.push_back(DataLine("file4", 23,  "1.4E-04  6.62E-05" ));
//	badSourceInput8_.push_back(DataLine("file4", 24,  "1.5E-04  6.62E-05" ));
//	badSourceInput8_.push_back(DataLine("file4", 25,  "1.6E-04  1.32E-04" ));
//	badSourceInput8_.push_back(DataLine("file4", 26,  "1.8E-04  1.32E-04" ));
//	badSourceInput8_.push_back(DataLine("file4", 27,  "2.0E-04  1.32E-04" ));
}


//void EnergydistributionTest::testEtype1()
//{
//	// type 1
//	auto itr = sourceInput1_.cbegin();
//	auto dist1 = src::phits::createPhitsEnergyDistribution("1", sourceInput1_, itr);
//	std::vector<double> expectedEnergies{0, 1e-4, 1.1e-4, 1.2e-4, 1.3e-4, 1.4e-4, 1.5e-4, 1.6e-4, 1.8e-4, 2.0e-4, 15};
//	std::vector<double> expectedValues{6.62e-4, 6.62e-5, 6.62e-5,6.62e-5,6.62e-5,6.62e-5,6.62e-5, 1.32e-4, 1.32e-4, 1.32e-4};
//	expectedValues = utils::normalize(expectedValues);
//	for(size_t i = 0; i < expectedValues.size(); ++i) {
//		expectedValues[i] = expectedValues.at(i)/(expectedEnergies.at(i+1) - expectedEnergies.at(i));
//	}

////	auto energies = dist1->getValues();
////	auto probabilities = dist1->getProbabilities();
////	QCOMPARE(energies.size(), expectedEnergies.size());
////	QCOMPARE(probabilities.size(), expectedValues.size());
////	for(size_t i = 0; i < energies.size(); ++i) {
////		QCOMPARE(expectedEnergies.at(i), energies.at(i));
////	}
////	for(size_t i = 0; i < probabilities.size(); ++i) {
////		QCOMPARE(expectedValues.at(i), probabilities.at(i));
////	}
////	mDebug() << "e-type=1\n" << dist1->toString();
//}



//void EnergydistributionTest::testEtype8()
//{
//	// type 8
//	auto itr = sourceInput8_.cbegin();
//	auto dist8 = src::phits::createPhitsEnergyDistribution("8", sourceInput8_, itr);
//	std::vector<double> expectedEnergies{0.00e-0, 1.0e-4,  1.1e-4, 1.5e-4,  1.2e-4, 1.2e-4, 1.4e-4,  1.6e-4,  1.8e-4,   2.0e-4};
//	std::vector<double> expectedValues  {6.62e-4, 6.62e-5, 6.62e-5, 6.62e-5, 6.62e-5, 6.62e-5, 6.62e-5, 1.32e-4, 1.32e-4, 1.32e-4};
//	double total = std::accumulate(expectedValues.begin(), expectedValues.end(), 0.0);
//	for(auto &elem: expectedValues) {
//		elem /= total;
//	}

////	auto energies = dist8->getValues();
////	auto probs = dist8->getProbabilities();
////	QCOMPARE(energies.size(), expectedEnergies.size());
////	QCOMPARE(probs.size(), expectedValues.size());
////	for(size_t i = 0; i < energies.size(); ++i) {
////		QCOMPARE(energies.at(i), expectedEnergies.at(i));
////	}
////	for(size_t i = 0; i < probs.size(); ++i) {
////		mDebug() << "i=" << i;
////		QCOMPARE(probs.at(i), expectedValues.at(i));
////	}
////	mDebug() << "e-type8=\n" << dist8->toString();
//}

//// 不正なデータはexitにしたのでテストは用済み
//void EnergydistributionTest::testBadEtype1()
//{
////	// type1不正なデータ
////	auto itr = badSourceInput1_.cbegin();
////	QVERIFY_EXCEPTION_THROWN(Distribution::makePhitsEnergyDistribution("1", badSourceInput1_, itr),
////							 std::invalid_argument);
////	itr = badSourceInput1_.cbegin();
////	try {
////		auto dist = Distribution::makePhitsEnergyDistribution("1", badSourceInput1_, itr);
////	} catch (std::exception &e) {
////		std::cerr << e.what() << std::endl;
////		QCOMPARE(e.what(), "Distribution values are not ascendant order.");
////	}
//}

//void EnergydistributionTest::testBadEtype8()
//{
////	// type8不正なデータ
////	auto itr = badSourceInput8_.cbegin();
////	QVERIFY_EXCEPTION_THROWN(Distribution::makePhitsEnergyDistribution("8", badSourceInput8_, itr),
////							 std::invalid_argument);
////	itr = badSourceInput8_.cbegin();
////	try {
////		auto dist = Distribution::makePhitsEnergyDistribution("8", badSourceInput8_, itr);
////	} catch (std::exception &e) {
////		std::cerr << e.what() << std::endl;
////		QCOMPARE(e.what(), "Error: file4:19\nEnergy and probability should be positive.");
////	}
//}

QTEST_APPLESS_MAIN(EnergydistributionTest)

#include "tst_phitsenergydistributiontest.moc"
