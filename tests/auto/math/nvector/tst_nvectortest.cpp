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
#include "core/math/nvector.hpp"
#include "core/utils/message.hpp"

using namespace math;

using Vec3 = math::Vector<3>;

class NvectorTest : public QObject
{
	Q_OBJECT

public:
	NvectorTest();
	const double v1 = 1, v2 = 1, v3 = 0, v4 = 1;
	const double u1 = 0, u2 = 2, u3 = 1, u4 = 0;
	const double a = -11.0;

private Q_SLOTS:
    void testGetOrthoVec();
	void testOrtho();
	void testVec3();
	void testVec4();
};

NvectorTest::NvectorTest() {;}

void NvectorTest::testGetOrthoVec()
{
    // 1つのベクトルから、それに直交するもう一つのベクトルを取得する。
    Vector<3>vec1{1,1,-2.5};
    auto vec12 = math::getOrthogonalUnitVector(vec1);
    QVERIFY(std::abs(vec12.abs()-1) < Vector<3>::EPS );
    QVERIFY(std::abs(math::dotProd(vec12, vec1)) < Vector<3>::EPS);

//    auto vec13 = math::getOrthogonalUnitVector(vec1, vec12);
}

void NvectorTest::testOrtho()
{
#define DUMP3VEC(v1, v2, v3)  mDebug() << "v1===" << v1 << "\n" << "v2===" << v2 << "\n" << "v3===" << v3;

	// num loop == 0なら1つ目のベクトルはそのまま採用され、
	// 2個目以降が直交するように変更される。
	Vector<3> v11{1, 0, 0}, v12{0, 1, 0}, v13{1, 1, 1};
	size_t nv1 = orthogonalize({&v11, &v12, &v13}, 0);
	QVERIFY(isSamePoint(v11, Vec3{1, 0, 0}));
	QVERIFY(isSamePoint(v12, Vec3{0, 1, 0}));
	QVERIFY(isSamePoint(v13, Vec3{0, 0, 1}));
	QCOMPARE(nv1, size_t(3));

	// 失敗する例 (v1, v2が直交で v3＝v1＋v2)
	Vector<3> v21{1, 0, 0}, v22{0, 1, 0}, v23{1, 1, 0};
	size_t nv2 = orthogonalize({&v21, &v22, &v23}, 0);
	QCOMPARE(nv2, 2);
	QVERIFY(!v22.isZero());
	QVERIFY(v23.isZero());
	// 失敗する例2 (v1, v2が非直交でv3=v1+v2)
	Vector<3> v31{4, 2, 1}, v32{1, 2, 0}, v33{5, 4, 1};
	size_t nv3 = orthogonalize({&v31, &v32, &v33}, 0);
	QCOMPARE(nv3, size_t(2));
	QVERIFY(!v32.isZero());
	QVERIFY(v33.isZero());

	// 失敗する例3 (v2=v3)
	Vector<3> v41{1, 2, 1}, v42{1, 1, 1}, v43{1, 1, 1};
	size_t nv4 = orthogonalize({&v41, &v42, &v43}, 0);
	QCOMPARE(nv4, size_t(2));
	QVERIFY(!v42.isZero());
	QVERIFY(v43.isZero());

	// 失敗する例4 (v1=v3)
	Vector<3> v51{1, 2, 1}, v52{1, 1, 1}, v53{1, 2, 1};
	size_t nv5 = orthogonalize({&v51, &v52, &v53}, 0);
	QCOMPARE(nv5, size_t(2));
	QVERIFY(v53.isZero());

	// 失敗する例5 (v1=v2=v3 で独立なベクトルは1個
	Vector<3> v61{1, 2, 1}, v62{1, 2, 1}, v63{1, 2, 1};
	size_t nv6 = orthogonalize({&v61, &v62, &v63}, 0);
    //DUMP3VEC(v61, v62, v63);
	QCOMPARE(nv6, size_t(1));
	QVERIFY(isSamePoint(v61, Vec3{1, 2, 1}));
	QVERIFY(v62.isZero());
	QVERIFY(v63.isZero());

	// 失敗する例6 (v1=0)
	Vector<3> v71{0, 0, 0}, v72{1, 2, 1}, v73{3, 2, 1};
	size_t nv7 = orthogonalize({&v71, &v72, &v73}, 0);
	QCOMPARE(nv7, 2);
	QVERIFY(!v72.isZero());
	QVERIFY(v73.isZero());


}

void NvectorTest::testVec3()
{
	constexpr size_t N = 3;
	typedef Vector<N> vec_type;
	typedef vec_type::array_type ArrayType;
	vec_type vec0, vec1{ArrayType{v1, v2, v3}}, vec2(ArrayType{u1, u2, u3}), vec3(ArrayType{v2, v3, v1});
	// EPSの表示
//	std::cout << "EPS=" << vec_type::EPS << std::endl;
	// distanceテスト自分自身との距離は0
	QCOMPARE(math::distance(vec1, vec1), 0.0);
	QCOMPARE(math::distance(vec2, vec2), 0.0);
	QCOMPARE(math::distance(vec1, vec2), std::sqrt((v1-u1)*(v1-u1) + (v2-u2)*(v2-u2) + (v3-u3)*(v3-u3)));
	// 未初期化Vectorはゼロ初期化
	QVERIFY(math::isSamePoint(vec0, vec_type::ORIGIN()));
	// 和・差
	QVERIFY(math::isSamePoint(vec1-vec2, vec_type(ArrayType{v1-u1, v2-u2, v3-u3})));
	QVERIFY(math::isSamePoint(vec1-vec1, vec_type::ORIGIN()));
	QVERIFY(math::isSamePoint(vec1+vec2, vec_type(ArrayType{v1+u1, v2+u2, v3+u3})));
	// 分配法則
	QVERIFY(math::isSamePoint(0.0*vec1, vec_type::ORIGIN()));
	QVERIFY(math::isSamePoint(vec1*0.0, vec_type::ORIGIN()));
	QVERIFY(math::isSamePoint(vec1+vec2, vec2+vec1));
	QVERIFY(math::isSamePoint(a*(vec1+vec2), a*vec1 + a*vec2));
	// 内積外積
	QCOMPARE(math::dotProd<3>(vec1, vec2), v1*u1 + v2*u2 + v3*u3);
	QCOMPARE(math::dotProd(vec1, vec2), v1*u1 + v2*u2 + v3*u3);
	QCOMPARE(math::dotProd(vec1, vec2), math::dotProd(vec2, vec1));
	QVERIFY(math::isSamePoint(math::crossProd(vec1, vec2), -math::crossProd(vec2, vec1)));
	QVERIFY(math::isSamePoint(math::crossProd(vec1, vec1), vec_type::ORIGIN()));
	QVERIFY(!math::isSamePoint(math::crossProd(vec1, vec2), math::crossProd(vec2, vec1)));
	// 正規化
	QCOMPARE(vec1.normalized().norm(2), 1.0);
	QCOMPARE(vec2.normalized().norm(2), 1.0);
	QCOMPARE(vec3.normalized().norm(2), 1.0);
	// 一次従属
	QVERIFY(!math::isDependent(vec1, vec2));
	QVERIFY(math::isDependent(vec1, vec1));
	QVERIFY(math::isDependent(vec1, a*vec1));
	// 直交化
	vec1 = vec_type{1, 2, 3};
	vec2 = vec_type{-11, 12, 13};
	vec3 = vec_type{21, -22, 23};
	std::array<vec_type, N> bases{{vec1, vec2, vec3}};
//	for(size_t i = 0; i < bases.size(); ++i) {
//		mDebug() << "BEFORE orthogonalize i, vec=" << i << ", " << bases[i];
//	}
	math::orthogonalize(&bases, 0);
//	for(size_t i = 0; i < N; ++i) {
//		mDebug() << "AFTER orthogonalize i, vec=" << i << ", " << bases[i];
//	}
	QVERIFY(std::abs(math::dotProd(bases[0], bases[1])) < N*vec_type::EPS);
	QVERIFY(std::abs(math::dotProd(bases[0], bases[2])) < N*vec_type::EPS);
	QVERIFY(std::abs(math::dotProd(bases[1], bases[2])) < N*vec_type::EPS);
	bases = std::array<vec_type, N>{{vec1, 10.2*vec1, vec3}};
	// 2行目が1行目に従属しているので直交化は失敗するはず。
	QCOMPARE(math::orthogonalize(&bases, 20), size_t(2));
	//QVERIFY_EXCEPTION_THROWN(math::orthogonalize(&bases, 20), std::invalid_argument);
	vec1 = vec_type{1, 2, 3};
	vec2 = vec_type{11, -2, 5};
	math::orthogonalize(std::vector<vec_type*>{&vec1, &vec2}, 0);
	math::orthogonalize({&vec1, &vec2}, 0);
	QVERIFY(math::isOrthogonal(vec1, vec2));
}

void NvectorTest::testVec4()
{
	constexpr size_t N = 4;
	typedef Vector<N> vec_type;
	typedef vec_type::array_type ArrayType;
	vec_type vec0, vec1{v1, v2, v3, v4}, vec2{u1, u2, u3, u4}, vec3{v1, v3, v4, v2}, vec4{u2, u4, u3, u1};
	// EPSの表示
//	std::cout << "EPS=" << vec_type::EPS << std::endl;
	// distanceテスト自分自身との距離は0
	QCOMPARE(math::distance(vec1, vec1), 0.0);
	QCOMPARE(math::distance(vec2, vec2), 0.0);
	QCOMPARE(math::distance(vec1, vec2),
			 std::sqrt((v1-u1)*(v1-u1) + (v2-u2)*(v2-u2) + (v3-u3)*(v3-u3) + (v4-u4)*(v4-u4)));
	// 未初期化Vectorはゼロ初期化
	QVERIFY(math::isSamePoint(vec0, vec_type::ORIGIN()));
	// 和・差
	QVERIFY(math::isSamePoint(vec1-vec2, vec_type(ArrayType{v1-u1, v2-u2, v3-u3, v4-u4})));
	QVERIFY(math::isSamePoint(vec1-vec1, vec_type::ORIGIN()));
	QVERIFY(math::isSamePoint(vec1+vec2, vec_type(ArrayType{v1+u1, v2+u2, v3+u3, v4+u4})));
	// 交換・結合法則
	QVERIFY(math::isSamePoint(0.0*vec1, vec_type::ORIGIN()));
	QVERIFY(math::isSamePoint(vec1*0.0, vec_type::ORIGIN()));
	QVERIFY(math::isSamePoint(vec1+vec2, vec2+vec1));
	QVERIFY(math::isSamePoint(a*(vec1+vec2), a*vec1 + a*vec2));
	// 内積
	QCOMPARE(math::dotProd(vec1, vec2), v1*u1 + v2*u2 + v3*u3 + v4*u4);
	QCOMPARE(math::dotProd(vec1, vec2), math::dotProd(vec2, vec1));
	// 正規化
	QCOMPARE(vec1.normalized().norm(2), 1.0);
	QCOMPARE(vec2.normalized().norm(2), 1.0);
	// 一次独立
	QVERIFY(!math::isDependent(vec1, vec2));
	QVERIFY(!math::isDependent(vec1, vec3));
	QVERIFY(!math::isDependent(vec1, vec4));
	QVERIFY(!math::isDependent(vec2, vec3));
	QVERIFY(!math::isDependent(vec2, vec4));
	QVERIFY(!math::isDependent(vec3, vec4));
	QVERIFY(math::isDependent(vec1, vec1));
	QVERIFY(math::isDependent(vec1, a*vec1));
	// 直交化
	std::array<vec_type, N> bases{{vec1, vec2, vec3, vec4}};
//	for(size_t i = 0; i < N; ++i) {
//		mDebug() << "BEFORE orthogonalize i, vec=" << i << ", " << bases[i];
//	}
	math::orthogonalize(&bases, 1000);
//	for(size_t i = 0; i < N; ++i) {
//		mDebug() << "AFTER orthogonalize i, vec=" << i << ", " << bases[i];
//	}
	QVERIFY(std::abs(math::dotProd(bases[0], bases[1])) < vec_type::EPS);
	QVERIFY(std::abs(math::dotProd(bases[0], bases[2])) < vec_type::EPS);
	QVERIFY(std::abs(math::dotProd(bases[0], bases[3])) < vec_type::EPS);
	QVERIFY(std::abs(math::dotProd(bases[1], bases[2])) < vec_type::EPS);
	QVERIFY(std::abs(math::dotProd(bases[1], bases[3])) < vec_type::EPS);
	QVERIFY(std::abs(math::dotProd(bases[2], bases[3])) < vec_type::EPS);
}

QTEST_APPLESS_MAIN(NvectorTest)

#include "tst_nvectortest.moc"
