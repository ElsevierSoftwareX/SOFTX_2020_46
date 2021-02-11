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

// add necessary includes here
#include "core/math/nmatrix.hpp"
#include "core/io/input/filling_utils.hpp"
#include "core/geometry/cell/boundingbox.hpp"

using Vec = math::Vector<3>;
using BB = geom::BoundingBox;

class Filling_utilsTest : public QObject
{
	Q_OBJECT

public:
    Filling_utilsTest();
    ~Filling_utilsTest();

private slots:
	void test_calcFillRangeRectangle2Dinf();  // 2D 格子で一方向が無限大BBに対するdimdecl計算
	void test_calcFillRangeRectangle2D();
	void test_calcFillRangeHexagon2D();
	void test_calcFillRangeHexagon2Dinf();
	void test_calcFillRangeHexagon3D();
};

Filling_utilsTest::Filling_utilsTest() {}
Filling_utilsTest::~Filling_utilsTest() {}

void Filling_utilsTest::test_calcFillRangeRectangle2Dinf()
{
	math::Point center{3.75, 2.5, 0};
	const std::vector<Vec> indexVectors1{Vec{5, 0, 0}, Vec{2.5, 5, 0}};
	const std::array<std::pair<int, int>, 3> expects1 {
		std::make_pair(-1, 4), std::make_pair(-5, -2), std::make_pair(0, 0),
	};

	// 1方向のみ無限大BBの場合(無限大の方向がstインデックスと直行していれば問題なく求まるはず)
	BB outerBB(-6, 13,  -23, -8, -BB::MAX_EXTENT, BB::MAX_EXTENT);
	const math::Point center1 = center;
	std::array<std::pair<int, int>, 3> results1 = calcDimensionDeclarator(2, center1, indexVectors1, outerBB);
	//for(size_t i = 0; i < expects1.size(); ++i) mDebug() << "i===" << i << "result, expect =" << results1.at(i) << expects1.at(i);
	for(size_t i = 0; i < expects1.size(); ++i) QCOMPARE(results1.at(i), expects1.at(i));

	// 1方向のみ無限大BBの場合(無限大の方向がstと直行していないケースは要素数無限になるので例外を投げる。)
	BB outerBB2(-6, 13,  -23, -8, -BB::MAX_EXTENT, BB::MAX_EXTENT);
	const math::Point center2 = center;
	const std::vector<Vec> indexVectors2{Vec{5, 0, 0.1}, Vec{2.5, 5, 0.1}};
	QVERIFY_EXCEPTION_THROWN(calcDimensionDeclarator(2, center2, indexVectors2, outerBB2), std::runtime_error);
}

void Filling_utilsTest::test_calcFillRangeRectangle2D()
{
	math::Point center{3.75, 2.5, 0};
	const std::vector<Vec> indexVectors{Vec{5, 0, 0}, Vec{2.5, 5, 0}};
	BB outerCellBB0(-6, 13,  -23, -8, -100, 100);
	const std::array<std::pair<int, int>, 3> expects {
		std::make_pair(-1, 4), std::make_pair(-5, -2), std::make_pair(0, 0),
	};


	std::array<std::pair<int, int>, 3> results = calcDimensionDeclarator(2, center, indexVectors, outerCellBB0);
//	for(size_t i = 0; i < expects.size(); ++i) mDebug() << "i===" << i << "result, expect =" << results.at(i) << expects.at(i);
	for(size_t i = 0; i < expects.size(); ++i) QCOMPARE(results.at(i), expects.at(i));





	// BB, center共に同じだけ平行移動させても結果は変わらないはず。
	auto mat = math::Matrix<4>::IDENTITY();
	mat.setTranslationVector(Vec{10, 20, 5});
	math::affineTransform(&center, mat);
	outerCellBB0.transform(mat);
	results = calcDimensionDeclarator(2, center, indexVectors, outerCellBB0);
//	for(size_t i = 0; i < expects.size(); ++i) mDebug() << "i===" << i << "result, expect=" << results.at(i) << expects.at(i);
	for(size_t i = 0; i < expects.size(); ++i) QCOMPARE(results.at(i), expects.at(i));
}



void Filling_utilsTest::test_calcFillRangeHexagon2D()
{
	math::Point center{0, 0, 0};
	const std::vector<Vec> indexVectors{Vec{10, 4, 0}, Vec{6, 14, 0}, Vec{-4, 10, 0}};
	BB outerCellBB(10, 40, -10, 18, -18.5, -6.5);
	const std::array<std::pair<int, int>, 3> expects {
		std::make_pair(0,5),
		std::make_pair(-2,1),
		std::make_pair(0, 0),
	};

	std::array<std::pair<int, int>, 3> results = calcDimensionDeclarator(2, center, indexVectors, outerCellBB);

//	for(size_t i = 0; i < expects.size(); ++i) {
//		mDebug() << "i===" << i << "result===" << results.at(i) << " expect===" << expects.at(i);
//	}
	for(size_t i = 0; i < expects.size(); ++i) QCOMPARE(results.at(i), expects.at(i));

	// BB, center共に同じだけ平行移動させても結果は変わらないはず。
	auto mat = math::Matrix<4>::IDENTITY();
	mat.setTranslationVector(Vec{10, 20, 5});
	math::affineTransform(&center, mat);
	outerCellBB.transform(mat);
	results = calcDimensionDeclarator(2, center, indexVectors, outerCellBB);
//	for(size_t i = 0; i < expects.size(); ++i) {
//		mDebug() << "i===" << i << "result===" << results.at(i) << " expect===" << expects.at(i);
//	}
	for(size_t i = 0; i < expects.size(); ++i) QCOMPARE(results.at(i), expects.at(i));
}

void Filling_utilsTest::test_calcFillRangeHexagon2Dinf()
{
	// stuインデックスベクトルがBBの無限大方向と直交する場合。
	math::Point center{0, 0, 0};
	const std::vector<Vec> indexVectors{Vec{10, 4, 0}, Vec{6, 14, 0}, Vec{-4, 10, 0}};
	BB outerBB(10, 40, -10, 18, -BB::MAX_EXTENT, BB::MAX_EXTENT);
	const std::array<std::pair<int, int>, 3> expects {
		std::make_pair(0,5),
		std::make_pair(-2,1),
		std::make_pair(0, 0),
	};

	std::array<std::pair<int, int>, 3> results = calcDimensionDeclarator(2, center, indexVectors, outerBB);
	for(size_t i = 0; i < expects.size(); ++i) QCOMPARE(results.at(i), expects.at(i));

	auto mat = math::Matrix<4>::IDENTITY();
	mat.setTranslationVector(Vec{10, 20, 5});
	math::affineTransform(&center, mat);
	outerBB.transform(mat);
	results = calcDimensionDeclarator(2, center, indexVectors, outerBB);
	for(size_t i = 0; i < expects.size(); ++i) QCOMPARE(results.at(i), expects.at(i));

	// stuインデックスベクトルがBBの無限大方向と直交しない場合は例外発生
	const std::vector<Vec> indexVectors2{Vec{10, 4, 0.1}, Vec{6, 14, 0.1}, Vec{-4, 10, 0.1}};
	QVERIFY_EXCEPTION_THROWN(calcDimensionDeclarator(2, center, indexVectors2, outerBB), std::runtime_error);
}



void Filling_utilsTest::test_calcFillRangeHexagon3D()
{
	math::Point center{0, 0, 0};
	const std::vector<Vec> indexVectors{Vec{10, 4, 0}, Vec{6, 14, 0}, Vec{-4, 10, 0}, Vec{0, 0, -5}};
	BB outerCellBB(10, 40, -10, 18, -18.5, -6.5);
	const std::array<std::pair<int, int>, 3> expects {
		std::make_pair(0,5),
		std::make_pair(-2,1),
		std::make_pair(1,4),
	};

	std::array<std::pair<int, int>, 3> results = calcDimensionDeclarator(3, center, indexVectors, outerCellBB);

//	for(size_t i = 0; i < expects.size(); ++i) {
//		mDebug() << "i===" << i << "result===" << results.at(i) << " expect===" << expects.at(i);
//	}
	for(size_t i = 0; i < expects.size(); ++i) QCOMPARE(results.at(i), expects.at(i));

	// BB, center共に同じだけ平行移動させても結果は変わらないはず。
	auto mat = math::Matrix<4>::IDENTITY();
	mat.setTranslationVector(Vec{10, 20, 5});
	math::affineTransform(&center, mat);
	outerCellBB.transform(mat);
	results = calcDimensionDeclarator(3, center, indexVectors, outerCellBB);
//	for(size_t i = 0; i < expects.size(); ++i) {
//		mDebug() << "i===" << i << "result===" << results.at(i) << " expect===" << expects.at(i);
//	}
	for(size_t i = 0; i < expects.size(); ++i) QCOMPARE(results.at(i), expects.at(i));
}

QTEST_APPLESS_MAIN(Filling_utilsTest)

#include "tst_filling_utils.moc"
