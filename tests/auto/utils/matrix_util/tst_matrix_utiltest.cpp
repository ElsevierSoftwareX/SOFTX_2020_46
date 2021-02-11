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

#include <string>
#include "core/utils/matrix_utils.hpp"
#include "core/utils/message.hpp"

class Matrix_utilTest : public QObject
{
	Q_OBJECT

public:
	Matrix_utilTest();

private Q_SLOTS:
	void testCase1();
};

Matrix_utilTest::Matrix_utilTest()
{
}

void Matrix_utilTest::testCase1()
{
	std::string src1 = "0 0 0  1 0 0  0 1 0  0 0 1";
	auto matrix1 = utils::generateSingleTransformMatrix(src1, false);
	mDebug() << "matrix1=" << matrix1;
	QVERIFY(math::isSameMatrix(matrix1, math::Matrix<4>::IDENTITY()));

	std::string src2 = "*0 0 0   0 90 90   90 0 90  90 90 0";
	auto matrix2 = utils::generateSingleTransformMatrix(src2, false);
	mDebug() << "matrix2=" << matrix2;;
	QVERIFY(math::isSameMatrix(matrix2, math::Matrix<4>::IDENTITY()));

}

QTEST_APPLESS_MAIN(Matrix_utilTest)

#include "tst_matrix_utiltest.moc"
