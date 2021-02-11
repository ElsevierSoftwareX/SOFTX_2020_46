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

#include <list>


#include "core/io/input/dataline.hpp"
#include "core/io/input/phits/phitsinputsection.hpp"
#include "core/source/abstractdistribution.hpp"
#include "core/source/phits/phitscylindersource.hpp"
#include "core/utils/utils.hpp"

using namespace inp;
using namespace src;

class PhitscylindersourceTest : public QObject
{
	Q_OBJECT

public:
	PhitscylindersourceTest();

private:
	std::list<DataLine> sourceInput1_;
	std::list<DataLine> sourceInput2_;

private Q_SLOTS:
	void testCase1();
	void testCase2();
    void testMultiSource();
};

PhitscylindersourceTest::PhitscylindersourceTest()
{
    sourceInput1_.push_back(DataLine("file",  0,  "s-type = 4" ));
	sourceInput1_.push_back(DataLine("file",  1,  "proj = photon " ));
	sourceInput1_.push_back(DataLine("file",  2,  "	sx = 1" ));
	sourceInput1_.push_back(DataLine("file",  3,  " sy = 0" ));
	sourceInput1_.push_back(DataLine("file",  4,  " sz = 0" ));
	sourceInput1_.push_back(DataLine("file",  5,  "reg = all" ));
	sourceInput1_.push_back(DataLine("file",  6,  "ntmax = 900" ));
	sourceInput1_.push_back(DataLine("file",  7,  "wgt = 2" ));
	sourceInput1_.push_back(DataLine("file",  8,  "factor = 2" ));
	sourceInput1_.push_back(DataLine("file",  9,  "s-type = 4" ));
	sourceInput1_.push_back(DataLine("file", 10,  "x0 = 0" ));
	sourceInput1_.push_back(DataLine("file", 11,  "y0 = 1" ));
	sourceInput1_.push_back(DataLine("file", 12,  "z0 = 0" ));
	sourceInput1_.push_back(DataLine("file", 13,  "z1 = 0.5" ));
	sourceInput1_.push_back(DataLine("file", 14,  "r0 = 5" ));
	sourceInput1_.push_back(DataLine("file", 15,  "r1 = 0" ));
	sourceInput1_.push_back(DataLine("file", 16,  "dir = data"));
	sourceInput1_.push_back(DataLine("file", 17,  "a-type = 1"));
	sourceInput1_.push_back(DataLine("file", 18,  "na = 3"));
	sourceInput1_.push_back(DataLine("file", 19,  "-1.0  0.3"));
	sourceInput1_.push_back(DataLine("file", 20,  "-0.4  0.8"));
	sourceInput1_.push_back(DataLine("file", 21,  " 0.8  0.1"));
	sourceInput1_.push_back(DataLine("file", 22,  " 1.0  "));
	sourceInput1_.push_back(DataLine("file", 23,  "e-type = 1" ));
	sourceInput1_.push_back(DataLine("file", 24,  "ne = 10" ));
	sourceInput1_.push_back(DataLine("file", 25,  "0.0E+00  6.62E-04" ));
	sourceInput1_.push_back(DataLine("file", 26,  "1.0E-04  6.62E-05" ));
	sourceInput1_.push_back(DataLine("file", 27,  "1.1E-04  6.62E-05" ));
	sourceInput1_.push_back(DataLine("file", 28,  "1.2E-04  6.62E-05" ));
	sourceInput1_.push_back(DataLine("file", 29,  "1.3E-04  6.62E-05" ));
	sourceInput1_.push_back(DataLine("file", 30,  "1.4E-04  6.62E-05" ));
	sourceInput1_.push_back(DataLine("file", 31,  "1.5E-04  6.62E-05" ));
	sourceInput1_.push_back(DataLine("file", 32,  "1.6E-04  1.32E-04" ));
	sourceInput1_.push_back(DataLine("file", 33,  "1.8E-04  1.32E-04" ));
	sourceInput1_.push_back(DataLine("file", 34,  "2.0E-04  1.32E-04" ));
	sourceInput1_.push_back(DataLine("file", 35,  "15" ));
//	sourceInput1_.push_back(DataLine("file", 36, "trcl = 0 0 5  1 1 0  -1 1 0  0 0 1  j   "));
//	sourceInput1_.push_back(DataLine("file", 37, "c * r-type = 1"));
//	sourceInput1_.push_back(DataLine("file", 38, "c * nr = 5"));
//	sourceInput1_.push_back(DataLine("file", 39, "c * 0 1 2 3 4 5"));
//	sourceInput1_.push_back(DataLine("file", 40, "c * z-type = 2"));
//	sourceInput1_.push_back(DataLine("file", 41, "c * nz = 3"));
//	sourceInput1_.push_back(DataLine("file", 42, "c * zmin = 0.1"));
//	sourceInput1_.push_back(DataLine("file", 43, "c * zmax = 0.4"));
//	sourceInput1_.push_back(DataLine("file", 44, "c * e-type = 3"));
//	sourceInput1_.push_back(DataLine("file", 45, "c * ne = 2"));
//	sourceInput1_.push_back(DataLine("file", 46, "c * emin=1e-8 ;emax=1e-4"));
//	sourceInput1_.push_back(DataLine("file", 47, "c * a-type = -2"));
//	sourceInput1_.push_back(DataLine("file", 48, "c * na = 10"));
//	sourceInput1_.push_back(DataLine("file", 49, "c * amin = 0"));
//	sourceInput1_.push_back(DataLine("file", 50, "c * amax = 360"));



	sourceInput2_ = sourceInput1_;
	auto itr = sourceInput2_.begin();
	std::advance(itr, 16);
	sourceInput2_.erase(itr, sourceInput2_.end());
	sourceInput2_.push_back(DataLine("file2", 15,  "dir = all"));
	sourceInput2_.push_back(DataLine("file2", 16,  "e-type = 8" ));
	sourceInput2_.push_back(DataLine("file2", 17,  "ne = 10" ));
	sourceInput2_.push_back(DataLine("file2", 18,  "0.1E+00  6.62E-04" ));
	sourceInput2_.push_back(DataLine("file2", 19,  "1.0E-04  6.62E-05" ));
	sourceInput2_.push_back(DataLine("file2", 20,  "1.1E-04  6.62E-05" ));
	sourceInput2_.push_back(DataLine("file2", 21,  "1.2E-04  6.62E-05" ));
	sourceInput2_.push_back(DataLine("file2", 22,  "1.3E-04  6.62E-05" ));
	sourceInput2_.push_back(DataLine("file2", 23,  "1.4E-04  6.62E-05" ));
	sourceInput2_.push_back(DataLine("file2", 24,  "1.5E-04  6.62E-05" ));
	sourceInput2_.push_back(DataLine("file2", 25,  "1.6E-04  1.32E-04" ));
	sourceInput2_.push_back(DataLine("file2", 26,  "1.8E-04  1.32E-04" ));
	sourceInput2_.push_back(DataLine("file2", 27,  "2.0E-04  1.32E-04" ));
	sourceInput2_.push_back(DataLine("file2", 28, "trcl = 0 0 5  1 1 0  -1 1 0  0 0 1  j   "));
	sourceInput2_.push_back(DataLine("file2", 29, "c * r-type = 1"));
	sourceInput2_.push_back(DataLine("file2", 30, "c * nr = 5"));
	sourceInput2_.push_back(DataLine("file2", 31, "c * 0 1 2 3 4 5"));
	sourceInput2_.push_back(DataLine("file2", 32, "c * z-type = 2"));
	sourceInput2_.push_back(DataLine("file2", 33, "c * nz = 1"));
	sourceInput2_.push_back(DataLine("file2", 34, "c * zmin = 0.2"));
	sourceInput2_.push_back(DataLine("file2", 35, "c * zmax = 0.2"));
	sourceInput2_.push_back(DataLine("file2", 37, "c * e-type = 3"));
	sourceInput2_.push_back(DataLine("file2", 38, "c * ne = 2"));
	sourceInput2_.push_back(DataLine("file2", 39, "c * emin=1e-8 ;emax=1e-4"));
	sourceInput2_.push_back(DataLine("file2", 40, "c * a-type = -2"));
	sourceInput2_.push_back(DataLine("file2", 41, "c * na=36"));
	sourceInput2_.push_back(DataLine("file2", 42, "c * amin=0"));
	sourceInput2_.push_back(DataLine("file2", 43, "c * amax=360"));

}

void PhitscylindersourceTest::testCase1()
{
	std::unordered_map<size_t, math::Matrix<4>> trMap;
	inp::phits::PhitsInputSection inputSection("source", sourceInput1_, true, true);
	auto cylinderSource
			= src::PhitsCylinderSource::create(trMap,
												std::unordered_map<std::string, std::shared_ptr<const geom::Cell>>(),
												inputSection, true);
//	PhitsCylinderSource cylindeSource(trMap, std::unordered_map<std::string, std::shared_ptr<const geom::Cell>>(), inputSection);

	//mDebug() << cylindeSource.toString();
	math::Point expectedCenter{0, 1, 0};
	QVERIFY(math::isSamePoint(cylinderSource->baseCenter(), expectedCenter));

	std::vector<double> expectedEnergies{0,   1e-4,   1.1e-4,   1.2e-4,  1.3e-4,  1.4e-4,   1.5e-4, 1.6e-4, 1.8e-4, 2.0e-4,    15};
	std::vector<double> expectedValues{  6.62e-4, 6.62e-5, 6.62e-5, 6.62e-5, 6.62e-5, 6.62e-5, 6.62e-5, 1.32e-4, 1.32e-4, 1.32e-4};
	expectedValues = utils::normalize(expectedValues);
	for(size_t i = 0; i < expectedValues.size(); ++i) {
		expectedValues[i] = expectedValues.at(i)/(expectedEnergies.at(i+1) - expectedEnergies.at(i));
	}

	// Energy
	QCOMPARE(cylinderSource->energyDistribution()->getProbability(0, 10, 15), 1.0);
	//Angle
	QCOMPARE(cylinderSource->angularDistribution()->getProbability(-1, 1.5, 2), 1.0);
}

void PhitscylindersourceTest::testCase2()
{
	inp::phits::PhitsInputSection inputSection2("source", sourceInput2_, true, true);
	auto cylindeSource = src::PhitsCylinderSource::create(std::unordered_map<size_t, math::Matrix<4>>(),
									  std::unordered_map<std::string, std::shared_ptr<const geom::Cell>>(),
									  inputSection2, true);
	//mDebug() << cylindeSource.toString();
	math::Point expectedCenter{-std::sqrt(2.0)*0.5, std::sqrt(2.0)*0.5, 5};
	mDebug() << "result=" << cylindeSource->baseCenter() << "expected=" << expectedCenter;
	QVERIFY(math::isSamePoint(cylindeSource->baseCenter(), expectedCenter));

	std::vector<double> expectedEnergies{0.1,     1e-4,    1.1e-4,  1.2e-4,  1.3e-4,  1.4e-4,  1.5e-4,  1.6e-4, 1.8e-4,  2.0e-4};
	std::vector<double> expectedValues{  6.62e-4, 6.62e-5, 6.62e-5, 6.62e-5, 6.62e-5, 6.62e-5, 6.62e-5, 1.32e-4, 1.32e-4, 1.32e-4};
	double total = std::accumulate(expectedValues.begin(), expectedValues.end(), 0.0);
	for(auto &elem: expectedValues) {
		elem /= total;
	}
	// Energy
	auto resultEnergies = cylindeSource->energyDistribution()->getDiscretePoints();
	QCOMPARE(expectedEnergies.size(), resultEnergies.size());
	for(size_t i = 0; i < expectedEnergies.size(); ++i) {
		QCOMPARE(resultEnergies.at(i), expectedEnergies.at(i));
	}
	QCOMPARE(cylindeSource->energyDistribution()->getProbability(0.5, 0.5, 0.5), 1.0);
	QCOMPARE(cylindeSource->energyDistribution()->getProbability(0.1, 1e-6, 1e-6), expectedValues.front());
	//Angle
	QCOMPARE(cylindeSource->angularDistribution()->getProbability(0, 1, 1), 1.0);
}

void PhitscylindersourceTest::testMultiSource()
{
	std::list<DataLine> msInput;
	msInput.push_back(DataLine("mfile",  0,  "totfactor = 4.5" ));
	msInput.push_back(DataLine("mfile",  1,  "<source> = 1.0" ));
	msInput.push_back(DataLine("mfile",  2,  " s-type = 4" ));
	msInput.push_back(DataLine("mfile",  3,  " proj = photon " ));
	msInput.push_back(DataLine("mfile",  4,  " reg = all" ));
	msInput.push_back(DataLine("mfile",  5,  " z0 = 0" ));
	msInput.push_back(DataLine("mfile",  6,  " z1 = 0.5" ));
	msInput.push_back(DataLine("mfile",  7,  " r0 = 5" ));
	msInput.push_back(DataLine("mfile",  8,  " dir = data"));
	msInput.push_back(DataLine("mfile",  9,  " a-type = 1"));
	msInput.push_back(DataLine("mfile", 10,  " na = 3"));
	msInput.push_back(DataLine("mfile", 11,  " -1.0  0.3"));
	msInput.push_back(DataLine("mfile", 12,  " -0.4  0.8"));
	msInput.push_back(DataLine("mfile", 13,  "  0.8  0.1"));
	msInput.push_back(DataLine("mfile", 14,  "  1.0  "));
	msInput.push_back(DataLine("mfile", 15,  " e-type = 1" ));
	msInput.push_back(DataLine("mfile", 16,  " ne = 3" ));
	msInput.push_back(DataLine("mfile", 17,  " 0.0E+00  6.62E-04" ));
	msInput.push_back(DataLine("mfile", 18,  " 1.0E-04  6.62E-05" ));
	msInput.push_back(DataLine("mfile", 19,  " 2.0E-04  1.32E-04" ));
	msInput.push_back(DataLine("mfile", 20,  " 2" ));
	msInput.push_back(DataLine("mfile", 21, " c * r-type = 1"));
	msInput.push_back(DataLine("mfile", 22, " c * nr = 5"));
	msInput.push_back(DataLine("mfile", 23, " c * 0 1 2 3 4 5"));
	msInput.push_back(DataLine("mfile", 24, " c * z-type = 2"));
	msInput.push_back(DataLine("mfile", 25, " c * nz = 3"));
	msInput.push_back(DataLine("mfile", 26, " c * zmin = 0.1"));
	msInput.push_back(DataLine("mfile", 27, " c * zmax = 0.4"));
	msInput.push_back(DataLine("mfile", 28, " c * e-type = 3"));
	msInput.push_back(DataLine("mfile", 29, " c * ne = 2"));
	msInput.push_back(DataLine("mfile", 30, " c * emin=1e-8 ;emax=1e-4"));
	msInput.push_back(DataLine("mfile", 30, " c * a-type = -2"));
	msInput.push_back(DataLine("mfile", 30, " c * na=10"));
	msInput.push_back(DataLine("mfile", 30, " c * amin=0; amax=360"));
	msInput.push_back(DataLine("mfile", 31,  "<source> = 2.0" ));
	msInput.push_back(DataLine("mfile3", 32,  " s-type = 1" ));
	msInput.push_back(DataLine("mfile3", 33,  " proj = photon " ));
	msInput.push_back(DataLine("mfile3", 34,  " reg = all" ));
	msInput.push_back(DataLine("mfile3", 35,  " z0 = 10" ));
	msInput.push_back(DataLine("mfile3", 36,  " z1 = 10" ));
	msInput.push_back(DataLine("mfile3", 37,  " r0 = 15" ));
	msInput.push_back(DataLine("mfile3", 38,  " dir = all"));
	msInput.push_back(DataLine("mfile3", 39,  " e0 = 1.333" ));
	msInput.push_back(DataLine("mfile3", 40, " c * r-type = 1"));
	msInput.push_back(DataLine("mfile3", 41, " c * nr = 5"));
	msInput.push_back(DataLine("mfile3", 42, " c * 0 1 2 3 4 5"));
	msInput.push_back(DataLine("mfile3", 30, " c * a-type = -2"));
	msInput.push_back(DataLine("mfile3", 30, " c * na=10"));
	msInput.push_back(DataLine("mfile3", 30, " c * amin=0; amax=360"));

	auto emptyTrMap = std::unordered_map<size_t, math::Matrix<4>>();
	auto emptyCellMap = std::unordered_map<std::string, std::shared_ptr<const geom::Cell>>();

	auto sourceVec = src::PhitsSource::createMultiSource(emptyTrMap, emptyCellMap, msInput, true, true);
	std::vector<double> expectedFactors{1.5, 3.0};
	for(size_t i = 0; i <  sourceVec.size(); ++i) {
		mDebug() << "######### source i=" << i << "#########";
		//mDebug() << sourceVec.at(i)->toString();
		QVERIFY(std::abs(expectedFactors.at(i) - sourceVec.at(i)->factor()) < 1e-6);
	}
}

QTEST_APPLESS_MAIN(PhitscylindersourceTest)

#include "tst_phitscylindersourcetest.moc"
