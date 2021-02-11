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


#include "core/geometry/surface/quadric.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/utils/message.hpp"

using namespace geom;
using namespace math;
typedef math::Vector<3> Vec;
typedef math::Matrix<4> Mat;

class QuadricTest : public QObject
{
	Q_OBJECT

public:
	QuadricTest();

private Q_SLOTS:
    void testTransAndInvTrans();
    void testEllipticCylinder();
    void testSqSphere();
    void testSphere();
    void testTrSphere();
    void testEllipticParaboloid();
};

QuadricTest::QuadricTest() {;}

// vector要素の最大値が1になるように規格化
void normalize(std::vector<double> *vec) {
    double mval = 0;
    for(const auto& v: *vec) {
        mval = std::max(v, mval);
    }
    if(std::abs(mval) > 1e-5) return;
    for(auto& v: *vec) v /= mval;
}

// 変換*逆変換で元に戻るかのチェック
void QuadricTest::testTransAndInvTrans()
{
    std::vector<double> params{1, 2, 3,   4, 5, -6,   -7, -8, 9, 10};
    auto pl = Quadric::createQuadric("pl", params, Mat::IDENTITY(), Quadric::TYPE::GQ, false);
//    mDebug() << pl->toString();
    double sq2 = std::sqrt(2.0);
    Mat mat{ 0.5*sq2, 0.5*sq2, 0,   0,
              -0.5*sq2, 0.5*sq2, 0,   0,
               0,       0,       1,   0,
              10,      20,      30,   1};
//    Mat mat{ 0,   1,   0,   0,
//            -1,   0,   0,   0,
//             0,   0,   1,   0,
//            0,  0,  0,   1};
//    Mat mat{ 0,   -1,   0,   0,
//        1,   0,   0,   0,
//        0,   0,   1,   0,
//        0,  0,  0,   1};
//    mDebug() << "mat45 ===" << mat;
//    mDebug() << "inv   ===" << mat.inverse();
    auto params1 = pl->parameters();
    pl->transform(mat);
    auto params2 = pl->parameters();
    pl->transform(mat.inverse());
    auto params3 = pl->parameters();
//    mDebug() << "params1 ===" << params1;
//    mDebug() << "params2 ===" << params2;
//    mDebug() << "params3 ===" << params3;
    normalize(&params1);
    normalize(&params3);
    for(size_t i = 0; i < params1.size(); ++i) {
        QVERIFY(std::abs(params1.at(i) - params3.at(i)) < 1e-4);
    }
}

void QuadricTest::testEllipticCylinder()
{
    // 45度回転行列
    double sq2 = std::sqrt(2.0);
    Mat mat45{ 0.5*sq2, 0.5*sq2, 0,   0,
              -0.5*sq2, 0.5*sq2, 0,   0,
               0,       0,       1,   0,
               0,       0,       0,   1};

    // これは楕円柱
    std::vector<double> params{0.5, 1.0, 0.5, 0.0, 0.0, 1.0,
        -1.25E+02, 1.7219298E+03, -1.25E+02, 7.4901830E+05};
    auto quad = Quadric::createQuadric("qd1", params, Mat::IDENTITY(), Quadric::TYPE::GQ, false);
//    mDebug() << "quad===" << quad->toString();
//    mDebug() << "planeVectors:";
//    for(const auto& plvec: quad->boundingPlanes()) {
//        mDebug() << "planes ===";
//        for(const auto& pl:plvec) {
//            mDebug() << pl.toString();
//        }
//    }
    quad->transform(mat45);
    quad->boundingPlanes();
//    mDebug() << "quad===" << quad->toString();
//    mDebug() << "planeVectors:";
//    for(const auto& plvec: quad->boundingPlanes()) {
//        mDebug() << "planes ===";
//        for(const auto& pl:plvec) {
//            mDebug() << pl.toString();
//        }
//    }

}

void QuadricTest::testSqSphere()
{
    // 中心(X,0,0)半径radiusの球
    double radius = 4, X=10;
    std::vector<double> params{1, 1, 1, 0, 0, 0, -radius*radius, X, 0, 0};
    auto sqSph = Quadric::createQuadric("sphere1", params, Mat::IDENTITY(), Quadric::TYPE::SQ, false);
    //	mDebug() << quad2->toString();
    QVERIFY(!sqSph->isForward(Point{X, 0, 0}));
    QVERIFY(sqSph->isForward(Point{-10, 0, 0}));
    QVERIFY(!sqSph->isForward(Point{(radius+X)*0.99, 0, 0}));

    // 外側から交点を求める
    auto p_ex = Point{X-radius, 0, 0};
    QVERIFY(math::isSamePoint(sqSph->getIntersection(Point{-100, 0, 0}, Vec{1, 0, 0}), p_ex));
    QVERIFY(math::isSamePoint(sqSph->getIntersection(Point{-100, 0, 0}, Vec{-1, 0, 0}), Vec::INVALID_VECTOR()));
    QVERIFY(math::isSamePoint(sqSph->getIntersection(Point{-100, 5, 0}, Vec{1, 0, 0}), Vec::INVALID_VECTOR()));
    // 内側から交点を求める
    auto p_ex2 = Point{X+radius, 0, 0};
    auto p_ex3 = Point{X, radius, 0};
    QVERIFY(math::isSamePoint(sqSph->getIntersection(Point{X, 0, 0}, Vec{1, 0, 0}), p_ex2));
    QVERIFY(math::isSamePoint(sqSph->getIntersection(Point{X, 0, 0}, Vec{-1, 0, 0}), p_ex));
    QVERIFY(math::isSamePoint(sqSph->getIntersection(Point{X, 0, 0}, Vec{0, 1, 0}), p_ex3));

    auto orgParams = sqSph->parameters();
    // 45度回転行列
    double sq2 = std::sqrt(2.0);
    Mat mat{ 0.5*sq2, 0.5*sq2, 0,   0,
            -0.5*sq2, 0.5*sq2, 0,   0,
             0,       0,       1,   0,
             0,       0,       0,   1};
// 単位行列 OK
//    Mat mat{ 1,   0,   0,   0,
//        0,   1,   0,   0,
//        0,   0,   1,   0,
//        0,   0,   0,   1};
// 平行移動 OK
//    Mat mat{ 1,   0,   0,   0,
//        0,   1,   0,   0,
//        0,   0,   1,   0,
//        10,   20,   30,   1};
    sqSph->transform(mat);
    auto newParams = sqSph->parameters();
    mDebug() << sqSph->toString();
    //基本的な内外判定 (45度回転版)
    QVERIFY(!sqSph->isForward(Point{5*sq2, 5*sq2, 0})); // 球の中心は当然backward
    QVERIFY(sqSph->isForward(Point{7*sq2+0.0001, 7*sq2+0.0001, 0})); // 球をはみ出る。
    QVERIFY(!sqSph->isForward(Point{3*sq2+0.0001, 3*sq2+0.0001, 0})); // 球の内側ぎりぎり。
    QVERIFY(sqSph->isForward(Point{3*sq2-0.0001, 3*sq2-0.0001, 0})); // 球をはみ出る。

    sqSph->transform(mat.inverse());
    auto reTredParams = sqSph->parameters();
    for(auto& val: orgParams) val /= orgParams.front();
    for(auto& val: newParams) val /= newParams.front();
    for(auto& val: reTredParams) val /= reTredParams.front();
    mDebug() << "oldparams ===" << orgParams;
    mDebug() << "newparams ===" << newParams;
    mDebug() << "reTreds   ===" << reTredParams;

    for(size_t i = 0; i < orgParams.size(); ++i) {
        QVERIFY(std::abs(orgParams.at(i) - reTredParams.at(i)) < 1e-4);
    }
}

void QuadricTest::testSphere()
{
    // 半径2の球
    double radius = 2;
    Quadric *quad = new Quadric("sphere1", std::vector<double>{1, 1, 1,  0, 0, 0,  0, 0, 0, -(radius*radius)});
    //mDebug() << quad->toString();

    QVERIFY(!quad->isForward(Point{0, 0, 0}));
    QVERIFY(quad->isForward(Point{5, 0, 0}));
//	for(int i = -20; i < 21; ++i) {
//		mDebug() << "i=" << i << "isForward=" << quad->isForward(Point{static_cast<double>(i), 0, 0});
//	}
    // 球の外側から交点を求める
    auto p_ex = Point{-radius, 0, 0};
    //mDebug() << "intersection=" << quad->getIntersection(Point{-100, 0, 0}, Vec{1, 0, 0});
    QVERIFY(math::isSamePoint(quad->getIntersection(Point{-100, 0, 0}, Vec{1, 0, 0}), p_ex));
    QVERIFY(math::isSamePoint(quad->getIntersection(Point{-100, 0, 0}, Vec{-1, 0, 0}), Vec::INVALID_VECTOR()));
    QVERIFY(math::isSamePoint(quad->getIntersection(Point{-100, 5, 0}, Vec{1, 0, 0}), Vec::INVALID_VECTOR()));

    // 球の内側から交点を求める
    auto p_ex2 = Point{radius, 0, 0};
    //mDebug() << "intersect2=" << quad->getIntersection(Point{0.1, 0, 0}, Vec{1, 0, 0});
    QVERIFY(math::isSamePoint(quad->getIntersection(Point{0.1, 0, 0}, Vec{1, 0, 0}), p_ex2));
    QVERIFY(math::isSamePoint(quad->getIntersection(Point{0, 0, 0}, Vec{-1, 0, 0}), p_ex));
}

void QuadricTest::testTrSphere()
{
    // x方向へXcm平行移動された球
    double radius = 2;
    std::vector<double> params{1, 1, 1, 0, 0, 0, 0, 0, 0, -radius*radius};
    const double X = 10;
    math::Matrix<4> mat{1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  X, 0, 0, 1};
    auto quad2 = Quadric::createQuadric("sphere1", params, mat, Quadric::TYPE::GQ, false);
//	mDebug() << quad2->toString();
    QVERIFY(!quad2->isForward(Point{10, 0, 0}));
    QVERIFY(quad2->isForward(Point{0, 0, 0}));
    auto p_ex2 = Point{8, 0, 0};
    QVERIFY(math::isSamePoint(quad2->getIntersection(Point{-100, 0, 0}, Vec{1, 0, 0}), p_ex2));
    QVERIFY(math::isSamePoint(quad2->getIntersection(Point{-100, 0, 0}, Vec{-1, 0, 0}), Vec::INVALID_VECTOR()));
    QVERIFY(math::isSamePoint(quad2->getIntersection(Point{-100, 5, 0}, Vec{1, 0, 0}), Vec::INVALID_VECTOR()));
}

void QuadricTest::testEllipticParaboloid()
{
    //1 GQ 1  1  0   0 0 0   0  0 -1   0 $ elliptic paraboloid
    // y=0断面ではz=x^2 の放物線
    std::vector<double> params{1, 1, 0, 0, 0, 0, 0, 0, -1, 0};
    auto ep = Quadric::createQuadric("ep", params, Mat::IDENTITY(), Quadric::TYPE::GQ, false);
    //mDebug() << ep->toString();
    QVERIFY(ep->isForward(Point{0, 0, -10}));
    QVERIFY(!ep->isForward(Point{0, 0, 10}));
    // x方向からの交点
    auto p_ex = Point{-std::sqrt(10), 0, 10};
    auto p_re = ep->getIntersection(Point{-100, 0, 10}, Vec{1, 0, 0});
    //mDebug() << "result=" << p_re;
    //mDebug() << "expect=" << p_ex;
    QVERIFY(math::isSamePoint(p_re, p_ex));
    QVERIFY(math::isSamePoint(ep->getIntersection(Point{-100, 0, -10}, Vec{1, 0, 0}), Vec::INVALID_VECTOR()));
    QVERIFY(math::isSamePoint(ep->getIntersection(Point{-100, 0, 10}, Vec{-1, 0, 0}), Vec::INVALID_VECTOR()));

    // z方向からの交点
    auto p_ex2 = Point{0, 0, 0};
    auto p_re2 = ep->getIntersection(Point{0, 0, 10}, Vec{0, 0, -1});
    //mDebug() << "result=" << p_re2;
    //mDebug() << "expect=" << p_ex2;
    QVERIFY(math::isSamePoint(p_re2, p_ex2));
    QVERIFY(math::isSamePoint(ep->getIntersection(Point{-100, 0, -10}, Vec{1, 0, 0}), Vec::INVALID_VECTOR()));
    QVERIFY(math::isSamePoint(ep->getIntersection(Point{-100, 0, 10}, Vec{-1, 0, 0}), Vec::INVALID_VECTOR()));
}

QTEST_APPLESS_MAIN(QuadricTest)

#include "tst_quadrictest.moc"
