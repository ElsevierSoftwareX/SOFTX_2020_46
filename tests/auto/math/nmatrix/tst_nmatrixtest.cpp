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

#include <cmath>
#include <random>

#include "core/math/nmatrix.hpp"
#include "core/math/nvector.hpp"
#include "core/utils/matrix_utils.hpp"
#include "core/utils/message.hpp"
#include "core/utils/numeric_utils.hpp"


using Vec3 = math::Vector<3>;

#define COMPAREVECPARA(vv1, vv2) \
	QCOMPARE(vv1.size(), vv2.size());\
	for(size_t i = 0; i < vv1.size(); ++i) {\
		QCOMPARE(vv1.at(i).size(), vv2.at(i).size());\
		QVERIFY(math::isDependent(vv1.at(i), vv2.at(i)));\
	}

#define COMPAREVECTOR(v1, v2) \
	QCOMPARE((v1).size(), (v2).size()); \
	for(size_t i = 0; i < (v1).size(); ++i) {\
		QVERIFY(utils::isSameDouble((v1).at(i), (v2).at(i))); \
	}



using namespace math;

Q_DECLARE_METATYPE(Point);

class NmatrixTest : public QObject
{
	Q_OBJECT

public:
	NmatrixTest();
	// 適当な値
	const double m11 =   1, m12=   2, m13 =   3, m14 =  4;
	const double m21 = -11, m22=  12, m23 =  13, m24 = 14;
	const double m31 =  21, m32= -22, m33 =  23, m34 = 24;
	const double m41 =  31, m42=  32, m43 = -33, m44 = 34;
	// 更に適当でコンストラクタで乱数を入れるための値
	double n11, n12, n13, n14;
	double n21, n22, n23, n24;
	double n31, n32, n33, n34;
	double n41, n42, n43, n44;
	double v1, v2, v3, v4;
	double u1, u2, u3, u4;
	const double a = -11.2;

private Q_SLOTS:
    void testConstruct();
    void testRank();
    void testDiagonalize();
    void testEigenDep();
    void testEigenDiagonal();
    void testEigen4();
    void testEigen();
    void testMatrix3();
    void testMatrix4();
    void testGenRotMat1_data();
    void testGenRotMat1();
    void testGenRotMat12();
    void testGenRotMat2();
};

NmatrixTest::NmatrixTest()
{
	std::mt19937 engine(0);
	std::uniform_real_distribution<> dist(-5.0, 5.0);
	n11 = dist(engine);	n12 = dist(engine);	n13 = dist(engine);	n14 = dist(engine);
	n21 = dist(engine);	n22 = dist(engine);	n23 = dist(engine);	n24 = dist(engine);
	n31 = dist(engine);	n32 = dist(engine);	n33 = dist(engine);	n34 = dist(engine);
	n41 = dist(engine);	n42 = dist(engine);	n43 = dist(engine);	n44 = dist(engine);
	v1 = dist(engine); v2 = dist(engine); v3 = dist(engine);v4 = dist(engine);
    u1 = dist(engine); u2 = dist(engine); u3 = dist(engine);u4 = dist(engine);
}

void NmatrixTest::testConstruct()
{
    std::string trstr = "1 2 3  1 1 0  -1 1 0  0 0 1";
    auto mat = utils::generateSingleTransformMatrix(trstr, false);
    mDebug() << "mat==="<<mat;
    mDebug() << "invMAt" << mat.inverse();
    std::string trstr2 = "1 2 3  1 1 0  -1 1 0  0 0 1  -1";
    auto mat2 = utils::generateSingleTransformMatrix(trstr2, false);
    mDebug() << mat2;
}

void NmatrixTest::testRank()
{
    QCOMPARE(Matrix<3>().rank(), 0);      // ゼロ行列のランクはゼロ
    QCOMPARE(Matrix<3>::IDENTITY().rank(), 3);  // 単位行列のランクは行数

    // rank==Dimのフルランクの場合。
    Matrix<3> mat31{
        4, 2, 1,
        5, 4, 1,
        1, 2, 5
    };
    QCOMPARE(mat31.rank(), 3);


    // rank!=Dimのランク落ちの場合。2行目=1行目+3行目なのでrankは2
    Matrix<3> mat32{
        4, 2, 1,
        5, 4, 1,
        1, 2, 0
    };
    QCOMPARE(mat32.rank(), 2);


    // rank!=Dimの場合。zero vectorを含むのでrankは2
    Matrix<3> mat33{
        4, 2, 1,
        0, 0, 0,
        1, 2, 0
    };
    QCOMPARE(mat33.rank(), 2);

    // rank!=Dimの場合。従属なベクトルをを含むのでrankは2
    Matrix<3> mat34{
        4, 2, 1,
        4, 2, 1,
        1, 2, 0
    };
    QCOMPARE(mat34.rank(), 2);

    // rank!=Dimの場合。独立なベクトルは1個しかないのでrankは1個しかないので
    Matrix<3> mat35{
        4, 2, 1,
        4, 2, 1,
        4, 2, 1
    };
    QCOMPARE(mat35.rank(), 1);

    // 4次の行列のテスト。この行列のランクは3。detは0だし。
    Matrix<4>mat36 {
         0.283024, 0,        -0.00532,  -2.42219,
         0,        0.219961,  0.036113, -0.31423,
        -0.00532,  0.036113,  0.006029, -0.00606,
        -2.42219, -0.31423,  -0.00606,  20.1787
    };
    //mDebug() << "detm36===" << mat36.determinant();
    QCOMPARE(mat36.rank(), 3);
}



void NmatrixTest::testDiagonalize()
{
    Matrix<3> mat31{
        0, 1, -1,
        1, 0, 1,
        -1, 1, 0
    };
    std::vector<double> evals;
    std::vector<Vector<3>> evecs;
    mat31.symEigenVV(&evals, &evecs, true, true);
    Matrix<3> pmat {std::array<Vector<3>, 3>{evecs.at(0), evecs.at(1), evecs.at(2)}};
    auto result31 = pmat*mat31*pmat.inverse();
//	mDebug() << "EigenValues ===" << evals;
//	mDebug() << "EigenVectors ===" << evecs;
//	mDebug() << "result===" << result31;

    for(size_t i = 0; i < result31.size(); ++i) {
        for(size_t j = 0; j < result31.size(); ++j) {
            if(i == j) {
                QVERIFY(utils::isSameDouble(result31(i, j), evals.at(i)));
            } else {
                QVERIFY(utils::isSameDouble(result31(i, j), 0));
            }
        }
    }

}

// ランク落ちした4次の行列
void NmatrixTest::testEigen4()
{
    Matrix<4> mat41{
         0.5,   0.0,   0.5,   -62.5,
         0.0,   1.0,   0.0,   234.0,
         0.5,   0.0,   0.5,   -62.5,
       -62.5, 234.0, -62.5, 62559.5
    };



    std::vector<double> eigenVals, eigenValsEx{62560.500143861, 1, -1.4386074247011E-4, 3.6780699E-16};
    std::vector<Vector<4>> eigenVecs{
        Vector<4>{9.9904091730199E-4, -0.0037404091943787, 9.9904091730199E-4, -0.99999200655483},
        Vector<4>{0.66148955018669, 0.35335980245016, 0.66148955018669, -1.6890919504767E-19},
        Vector<4>{0.24986111523407, -0.93548001547411, 0.24986111525425, 0.0039983529671953},
        Vector<4>{0.70710678118655, -1.9019962754526E-20, -0.70710678118655, 8.1330964424014E-23},
    };
    auto sig = mat41.symEigenVV(&eigenVals, &eigenVecs, true, true);



//	mDebug() << "signature===" << sig;
//	mDebug() << "eigen values===" << eigenVals;
//	mDebug() << "eigen vectors ===" << eigenVecs;
//	for(size_t i = 0; i < eigenVecs.size(); ++i) {
//		QVERIFY(utils::isSameDouble(eigenVals.at(i), eigenValsEx.at(i)));
//	}

//	// 対角化
//	Matrix<4> pmat {std::array<Vector<4>, 4>{
//			eigenVecs.at(0), eigenVecs.at(1), eigenVecs.at(2), eigenVecs.at(3)
//		}};
//	auto resultMat = pmat.inverse()*mat41*pmat;
//	mDebug() << resultMat;
}



void NmatrixTest::testEigenDep()
{
    //rank落ちしている場合
    Matrix<3> mat31{
        0.5, 0.0, 0.5,
        0.0, 1.0, 0.0,
        0.5, 0.0, 0.5
    };
    std::vector<double> eigenVals;
    std::vector<Vector<3>> eigenVecs;
    auto sig = mat31.symEigenVV(&eigenVals, &eigenVecs, false);
//	mDebug() << "eigen values ===" << eigenVals;
//	mDebug() << "eigen vecs ===" << eigenVecs;
}

void NmatrixTest::testEigenDiagonal()
{
    using V3 = math::Vector<3>;
    Matrix<3> mat31{
        281.25, 0, 0,
        0, 506.25, 0,
        0, 0, 506.25,
    };
//	Matrix<3> mat31{
//		1, 0, 0,
//		0, 2, 0,
//		0, 0, 2,
//	};
    std::vector<double> eigenVals, exVals{281.25, 506.25, 506.25};
    std::vector<Vector<3>> eigenVecs, exVecs{V3{1, 0, 0}, V3{0, 1, 0}, V3{0, 0, 1}};

    auto sig = mat31.symEigenVV(&eigenVals, &eigenVecs, false);
//	mDebug() << "eigen values  ===" << eigenVals;
//	mDebug() << "eigen vectors ===" << eigenVecs;
//	mDebug() << "signature ===" << sig;

    COMPAREVECTOR(eigenVals, exVals);

//	mDebug() << "evecsize===" << eigenVecs.size() << "refsize===" << exVecs.size();
    QCOMPARE(eigenVecs.size(), exVecs.size());
    for(size_t i = 0; i < eigenVecs.size(); ++i) {
//		mDebug() << "resultVec ===" << eigenVecs.at(i);
//		mDebug() << "expectVec ===" << exVecs.at(i);
        QVERIFY(math::isSamePoint(eigenVecs.at(i), exVecs.at(i)));
    }
    QCOMPARE(std::get<0>(sig), 3);
    QCOMPARE(std::get<1>(sig), 0);
}




void NmatrixTest::testEigen()
{
    // ###### 二次行列
    // 二次行列で固有値が2つ[6,1]で固有ベクトルが2つ[(1,-2)(2,1)]の場合
    Matrix<2> mat21{
        5, 2,
        2, 2
    };
    std::vector<double> eigenVals, exVals{6, 1};
    std::vector<Vector<2>> eigenVecs, exVecs{Vector<2>{2, 1}.normalized(), Vector<2>{1, -2}.normalized()};
    mat21.symEigenVV(&eigenVals, &eigenVecs, true, true);
//	for(size_t i = 0; i < eigenVals.size(); ++i) {
//		mDebug() << "i===" << i << "eigen val===" << eigenVals.at(i) << "vec===" << eigenVecs.at(i);
//		mDebug() << "i===" << i << "expec val===" << exVals.at(i) << "vec===" << exVecs.at(i);
//	}


    COMPAREVECTOR(eigenVals, exVals);
    COMPAREVECPARA(eigenVecs, exVecs);

    // 対角化
    Matrix<2> p21{std::array<Vector<2>, 2>{eigenVecs.at(0), eigenVecs.at(1)}};
    auto resultMat21 = p21*mat21*p21.inverse();
    QVERIFY(resultMat21.isSymmetric());
//	mDebug() << resultMat21;
    for(size_t i = 0; i < eigenVals.size(); ++i) {
        //mDebug() << i << "番目の固有値 ===" << eigenVals.at(0) << ", 対角化行列の成分===" << resultMat21(i, i);
        QVERIFY(utils::isSameDouble(resultMat21(i, i), eigenVals.at(i)));
    }


    // ####### 三次
    // 3次行列で固有値が3つ[8, 5, 3]で固有ベクトルが3つ[(-1,-1,1) (-1,2,1) (1,0,1)]の場合
    Matrix<3> mat31{
         5,  1, -2,
         1,  6, -1,
        -2, -1,  5
    };
    std::vector<double> eigenVals31, exVals31{8, 5, 3};
    std::vector<Vec3> eigenVecs31;
    std::vector<Vec3> exVecs31{Vec3{-1, -1, 1}.normalized(), Vec3{-1, 2, 1}.normalized(),Vec3{1, 0, 1}.normalized()};
    mat31.symEigenVV(&eigenVals31, &eigenVecs31, true, true);
    COMPAREVECTOR(eigenVals31, exVals31);
    COMPAREVECPARA(eigenVecs31, exVecs31);
    for(size_t i = 0; i < eigenVals31.size(); ++i) {
        math::isSamePoint(eigenVecs31.at(i)*mat31, eigenVals31.at(i)*eigenVecs31.at(i));
    }
    COMPAREVECPARA(eigenVecs31, exVecs31);

    // 3次行列で固有値が2つ[4,1,1]で固有ベクトルが3つ[(1,1,1)(-1,0,1)(0,-1,1)]の場合
    // 重解の固有値に属する固有ベクトルは絶対値だけでは一意にならないのでAx=λxで計算結果を検証する。
    Matrix<3> mat32{
        2, 1, 1,
        1, 2, 1,
        1, 1, 2
    };
    std::vector<double> eigenVals32, exVals32{4, 1, 1};
    std::vector<Vector<3>> eigenVecs32;
    mat32.symEigenVV(&eigenVals32, &eigenVecs32, true, true);
    COMPAREVECTOR(eigenVals32, exVals32);
    for(size_t i = 0; i < eigenVals32.size(); ++i) {
        math::isSamePoint(eigenVecs32.at(i)*mat32, eigenVals32.at(i)*eigenVecs32.at(i));
    }

    // 3次でランク落ちして固有値がゼロになる場合.固有値は[6,1,0]、固有ベクトルは[(1,-2,0)(2,1,0)(0,0,1)]
    Matrix<3> mat33{
        5, 2, 0,
        2, 2, 0,
        0, 0, 0
    };
    std::vector<double> eigenVals33, exVals33{6,1,0};
    std::vector<Vec3> eigenVecs33;
    std::vector<Vec3> exVecs33{Vec3{2,1,0}.normalized(), Vec3{1,-2,0}.normalized(),Vec3{0,0,1}.normalized()};
    mat33.symEigenVV(&eigenVals33, &eigenVecs33, true);
    //mDebug() << "vals===" << eigenVals33 << "vecs===" << eigenVecs33;
    COMPAREVECTOR(eigenVals33, exVals33);
    for(size_t i = 0; i < eigenVals33.size(); ++i) {
        math::isSamePoint(eigenVecs33.at(i)*mat33, eigenVals33.at(i)*eigenVecs33.at(i));
    }
    COMPAREVECPARA(eigenVecs33, exVecs33);

}



void NmatrixTest::testMatrix3()
{
    constexpr unsigned int N = 3;
    typedef Matrix<N> mat_type;
    mat_type zero, identity = Matrix<N>::IDENTITY();
    mat_type mat1{m11, m12, m13, m21, m22, m23, m31, m32, m33};
    mat_type trMat1{m11, m21, m31, m12, m22, m32, m13, m23, m33};
    mat_type mat2{n11, n12, n13, n21, n22, n23, n31, n32, n33};
    // 行列の比較ができていないとテストのしようが無いので初期化と比較を最初にテスト
    QVERIFY(math::isSameMatrix(zero, mat_type::ZERO()));
    QVERIFY(math::isSameMatrix(identity, mat_type::IDENTITY()));
    // 転値
    QVERIFY(math::isSameMatrix(mat1.transposed(), trMat1));
    // 和、差、交換法則
    Matrix<N> added{m11+n11, m12+n12, m13+n13, m21+n21, m22+n22, m23+n23, m31+n31, m32+n32, m33+n33};
    QVERIFY(math::isSameMatrix(mat1+mat2, added));
    QVERIFY(math::isSameMatrix(mat1+mat2, mat2+mat1));
    Matrix<N> subtracted{m11-n11, m12-n12, m13-n13, m21-n21, m22-n22, m23-n23, m31-n31, m32-n32, m33-n33};
    QVERIFY(math::isSameMatrix(mat1-mat2, subtracted));
    QVERIFY(math::isSameMatrix(mat2-mat1, -subtracted));
    // スカラとの積と分配法則
    mat_type amat{a*m11, a*m12, a*m13, a*m21, a*m22, a*m23, a*m31, a*m32, a*m33};
    QVERIFY(math::isSameMatrix(a*mat1, amat));
    QVERIFY(math::isSameMatrix(a*(mat1 + mat2), a*mat1 + a*mat2));
    QVERIFY(math::isSameMatrix(a*(mat1 - mat2), a*mat1 - a*mat2));
    // vectorとの積と分配法則
    //mDebug() << "mat2=" << mat2;
    Vector<N> vec1{v1, v2, v3}, vec2{u1, u2, u3};
//	mDebug() << "mat1=" << (vec1 + vec2)*(mat1 + mat2);
//	mDebug() << "mat2=" << vec1*mat1 + vec1*mat2 + vec2*mat1 + vec2*mat2;
    QVERIFY(math::isSamePoint((vec1 + vec2)*(mat1 + mat2), vec1*mat1 + vec1*mat2 + vec2*mat1 + vec2*mat2));

    // 行列行列積
    Matrix<N>::array_type parr;
    for(size_t i = 0; i < N; ++i) {
        for(size_t j = 0; j < N; ++j) {
            parr[i][j] = math::dotProd(mat1.rowVector(i), mat2.colVector(j));
        }
    }
    QVERIFY(math::isSameMatrix(mat1*mat2, Matrix<N>(parr)));
    // 部分小行列
    QVERIFY(math::isSameMatrix(Matrix<N-1>::IDENTITY(), identity.minorMatrix(0, 0)));
    QVERIFY(math::isSameMatrix(Matrix<N-1>::IDENTITY(), identity.minorMatrix(N-1, N-1)));
    // 逆行列     逆行列が合っていれば、行列式、小行列、余因子、全てあっているはず。
//	mDebug() << "test matrix=" << mat1;
//	mDebug() << "inverse mat=" << mat1.inverse();
//	mDebug() << "mat*inv=" << mat1*mat1.inverse();
    QVERIFY(math::isSameMatrix(identity*identity.inverse(), mat_type::IDENTITY()));
    QVERIFY(math::isSameMatrix(mat1*mat1.inverse(),  mat_type::IDENTITY()));
    // 正規直交化
    //mDebug() << "before=" << mat2;
    Matrix<N>::orthonormalize(&mat2);
    //mDebug() << "after=" << mat2;

    QVERIFY(mat2.isOrthogonal());
    for(size_t i = 0; i < N; ++i) {
        QVERIFY(utils::isSameDouble(mat2.rowVector(i).norm(2), 1.0));
    }
}

void NmatrixTest::testMatrix4()
{
    constexpr unsigned int N = 4;
    typedef Matrix<N> mat_type;
    mat_type zero, identity = Matrix<N>::IDENTITY();
    mat_type   mat1{m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44};
    mat_type trMat1{m11, m21, m31, m41, m12, m22, m32, m42, m13, m23, m33, m43, m14, m24, m34, m44};
    mat_type   mat2{n11, n12, n13, n14, n21, n22, n23, n24, n31, n32, n33, n34, n41, n42, n43, n44};
    // 初期化と比較を最初にテスト
    QVERIFY(math::isSameMatrix(zero, mat_type::ZERO()));
    QVERIFY(math::isSameMatrix(identity, mat_type::IDENTITY()));
    // 転値
    QVERIFY(math::isSameMatrix(mat1.transposed(), trMat1));
    // 和、差、交換法則
    Matrix<N> added{m11+n11, m12+n12, m13+n13, m14+n14,
                    m21+n21, m22+n22, m23+n23, m24+n24,
                    m31+n31, m32+n32, m33+n33, m34+n34,
                    m41+n41, m42+n42, m43+n43, m44+n44};
    QVERIFY(math::isSameMatrix(mat1+mat2, added));
    QVERIFY(math::isSameMatrix(mat1+mat2, mat2+mat1));
    Matrix<N> subtracted{	m11-n11, m12-n12, m13-n13, m14-n14,
                            m21-n21, m22-n22, m23-n23, m24-n24,
                            m31-n31, m32-n32, m33-n33, m34-n34,
                            m41-n41, m42-n42, m43-n43, m44-n44};
    QVERIFY(math::isSameMatrix(mat1-mat2, subtracted));
    QVERIFY(math::isSameMatrix(mat2-mat1, -subtracted));
    // スカラとの積と分配法則
    mat_type amat{a*m11, a*m12, a*m13, a*m14, a*m21, a*m22, a*m23, a*m24,
                  a*m31, a*m32, a*m33, a*m34, a*m41, a*m42, a*m43, a*m44};
    QVERIFY(math::isSameMatrix(a*mat1, amat));
    QVERIFY(math::isSameMatrix(a*(mat1 + mat2), a*mat1 + a*mat2));
    QVERIFY(math::isSameMatrix(a*(mat1 - mat2), a*mat1 - a*mat2));
    // vectorとの積と分配法則
    //mDebug() << "mat2=" << mat2;
    Vector<N> vec1{v1, v2, v3, v4}, vec2{u1, u2, u3, u4};
    QVERIFY(math::isSamePoint((vec1 + vec2)*(mat1 + mat2), vec1*mat1 + vec1*mat2 + vec2*mat1 + vec2*mat2));
    // 行列行列積
    Matrix<N>::array_type parr;
    for(size_t i = 0; i < N; ++i) {
        for(size_t j = 0; j < N; ++j) {
            parr[i][j] = math::dotProd(mat1.rowVector(i), mat2.colVector(j));
        }
    }
    QVERIFY(math::isSameMatrix(mat1*mat2, Matrix<N>(parr)));
    // 部分小行列
    QVERIFY(math::isSameMatrix(Matrix<N-1>::IDENTITY(), identity.minorMatrix(0, 0)));
    QVERIFY(math::isSameMatrix(Matrix<N-1>::IDENTITY(), identity.minorMatrix(N-1, N-1)));
    // 逆行列     逆行列が合っていれば、行列式、小行列、余因子、全てあっているはず。
//	mDebug() << "test matrix=" << mat1;
//	mDebug() << "inverse mat=" << mat1.inverse();
//	mDebug() << "mat*inv=" << mat1*mat1.inverse();
    QVERIFY(math::isSameMatrix(identity*identity.inverse(), mat_type::IDENTITY()));
    QVERIFY(math::isSameMatrix(mat1*mat1.inverse(), mat_type::IDENTITY()));
    // 直交化、正規直交化
    Matrix<N>::orthonormalize(&mat1);
    QVERIFY(mat1.isOrthogonal());
    Matrix<N>::orthonormalize(&mat2);
    QVERIFY(mat2.isOrthogonal());
    for(size_t i = 0; i < N; ++i) {
        QVERIFY(utils::isSameDouble(mat2.rowVector(i).norm(2), 1.0));
    }
}


/*
 * generateRotationMatrix(vec1, vec2)のテスト。
 *
 * 生成した回転行列をvec2に適用して、vec1になるかテストしている。
 *
 */
void NmatrixTest::testGenRotMat1_data()
{
    using Vec3 = math::Vector<3>;
    QTest::addColumn<Vec3>("ref");
    QTest::addColumn<Vec3>("target");
    std::mt19937 engine(0);
    std::uniform_real_distribution<> dist(-10.0, 10.0);
    QTest::newRow("positive rotation ")  <<Vec3{0, 1, 0} << Vec3{1, 0, 0};
    QTest::newRow("netative rotation ")  <<Vec3{1, 0, 0} << Vec3{0, 1, 0};
    QTest::newRow("rotation test2 ")  <<Vec3{dist(engine), dist(engine), dist(engine)} << Vec3{dist(engine),dist(engine),dist(engine)};
    QTest::newRow("rotation test3 ")  <<Vec3{dist(engine), dist(engine), dist(engine)} << Vec3{dist(engine),dist(engine),dist(engine)};
    QTest::newRow("rotation test4 ")  <<Vec3{dist(engine), dist(engine), dist(engine)} << Vec3{dist(engine),dist(engine),dist(engine)};
    QTest::newRow("rotation test5 ")  <<Vec3{dist(engine), dist(engine), dist(engine)} << Vec3{dist(engine),dist(engine),dist(engine)};
    QTest::newRow("rotation test6 ")  <<Vec3{dist(engine), dist(engine), dist(engine)} << Vec3{dist(engine),dist(engine),dist(engine)};
    QTest::newRow("rotation test7 ")  <<Vec3{dist(engine), dist(engine), dist(engine)} << Vec3{dist(engine),dist(engine),dist(engine)};
    QTest::newRow("rotation test8 ")  <<Vec3{dist(engine), dist(engine), dist(engine)} << Vec3{dist(engine),dist(engine),dist(engine)};
    QTest::newRow("rotation test9 ")  <<Vec3{dist(engine), dist(engine), dist(engine)} << Vec3{dist(engine),dist(engine),dist(engine)};
    QTest::newRow("rotation test10")  <<Vec3{dist(engine), dist(engine), dist(engine)} << Vec3{dist(engine),dist(engine),dist(engine)};
    QTest::newRow("rotation test11")  <<Vec3{dist(engine), dist(engine), dist(engine)} << Vec3{dist(engine),dist(engine),dist(engine)};
}

void NmatrixTest::testGenRotMat1()
{
    QFETCH(Point, ref);
    QFETCH(Point, target);
    Point result = target*generateRotationMatrix1(ref, target);
//    mDebug() << "original(arg2)==" << target << ", abs ===" << target.abs();
//    mDebug() << "reference(arg1)=" << ref;
//    mDebug() << "result==========" << result << ", abs===" << result.abs();
//    mDebug() << "expect=========" << ref;
//	mDebug() << "diff===========" << std::abs(dotProd(ref, result) - ref.abs()*result.abs());
//    QVERIFY(math::isSamePoint(ref, result));
    QVERIFY(std::abs(dotProd(ref, result) - ref.abs()*result.abs()) < 1e-11 && math::dotProd(result, ref) > 0);
}

void NmatrixTest::testGenRotMat12()
{
    // 正反対の方向を向いている場合機能しないことがあるのでチェック
    math::Vector<3> ref{-1, 0, 0}, target{1, 0, 0};
    auto rotMat = generateRotationMatrix1(ref, target);
    auto result = target*rotMat;
//    mDebug() << "refvec ===" << ref;
//    mDebug() << "target ===" << target;
//    mDebug() << "result ===" << result;
//    mDebug() << "rotMat ===" << rotMat;
    QVERIFY(std::abs(dotProd(ref, result) - ref.abs()*result.abs()) < 1e-11 && math::dotProd(result, ref) > 0);

    math::Vector<3> ref2{1, 1, -2.5}, target2{-1, -1, 2.5};
    auto rotMat2 = generateRotationMatrix1(ref2, target2);
    auto result2 = target2*rotMat2;
    //    mDebug() << "refvec ===" << ref;
    //    mDebug() << "target ===" << target;
    //    mDebug() << "result ===" << result;
    //    mDebug() << "rotMat ===" << rotMat;
    QVERIFY(std::abs(dotProd(ref2, result2) - ref2.abs()*result2.abs()) < 1e-11 && math::dotProd(result2, ref2) > 0);
}


void NmatrixTest::testGenRotMat2()
{

    using Vec = math::Vector<3>;

    // +45°回転
    auto mat45 = generateRotationMatrix2(Vec{0, 0, 1}, math::toRadians(45));
    auto xunitVec = Vec{1, 0, 0}, expect1 = Vec{0.5*std::sqrt(2), 0.5*std::sqrt(2), 0};
    auto res1 = xunitVec*mat45;
//	mDebug() << "ref===" << ref1;
//	mDebug() << "mat1===" << mat45;
//	mDebug() << "res1===" << res1;
//	mDebug() << "exp2===" << expect1;

    QVERIFY(isSamePoint(res1, expect1));

    // +90°回転
    auto mat90 = generateRotationMatrix2(Vec{0, 0, 1}, math::toRadians(90));
    auto res2 = xunitVec*mat90;
    auto expect2 = Vec{0, 1, 0};
    QVERIFY(isSamePoint(res2, expect2));

    // -90°回転
    auto mat90m = generateRotationMatrix2(Vec{0, 0, 1}, math::toRadians(-90));
    auto res3 = xunitVec*mat90m;
    auto expect3 = Vec{0, -1, 0};
//	mDebug() << "ref===" << ref1;
//	mDebug() << "mat===" << mat90m;
//	mDebug() << "res===" << res3;
//	mDebug() << "exp===" << expect3;
    QVERIFY(isSamePoint(res3, expect3));
}



QTEST_APPLESS_MAIN(NmatrixTest)

#include "tst_nmatrixtest.moc"
