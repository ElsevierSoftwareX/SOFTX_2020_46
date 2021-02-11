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
#include "nvector.hpp"
#include "core/utils/message.hpp"

template <>
double math::distance(const math::Vector<3> &v1, const math::Vector<3> &v2) {
	return std::sqrt((v1.data_[0]-v2.data_[0])*(v1.data_[0]-v2.data_[0])
			 + (v1.data_[1]-v2.data_[1])*(v1.data_[1]-v2.data_[1])
			+ (v1.data_[2]-v2.data_[2])*(v1.data_[2]-v2.data_[2])
			);
}


template <>
double math::distance(const std::shared_ptr<math::Vector<3>> &v1, const std::shared_ptr<math::Vector<3>> &v2) {
	return distance(*v1.get(), *v2.get());
}



math::Vector<3> math::getAxisVector(math::AXIS axis)
{
	switch(axis) {
	case math::AXIS::X:
		return math::Vector<3>{1, 0, 0};
	case math::AXIS::Y:
		return math::Vector<3>{0, 1, 0};
    case math::AXIS::Z:
        return math::Vector<3>{0, 0, 1};
//	default:
//		break;
//		std::cerr << "Invalid axis enum" << std::endl;
//		std::exit(EXIT_FAILURE);
	}
	return math::Vector<3>{0, 0, 0};
}


/*
 * あるベクトルvorgに対して垂直な直交する2ベクトルを作成する。
 *
 * axis_に垂直な2ベクトルはVector<3>::orthogonalize(vparray, 0)で作成できる。
 * この2方向の初期値は vx{1, 0, 0} vy{0, 1, 0} vz{0, 0, 1}から2個選ぶ
 * 選んだ2個のどちらかがaxisと同方向ならinvalid_argumentが投げられるのでキャッチして別の2個を選ぶ。
 * 3C2なので3通りの選択がある。
 *
 */
std::pair<math::Vector<3>, math::Vector<3>> math::get2OrthogonalUnitVectors(const math::Vector<3> &vorg)
{
	math::Vector<3> v0 = vorg, v1{1, 0, 0};
	if(math::isDependent(v0, v1)) {// vorgがx軸方向ベクトルの場合
		v1 = math::Vector<3>{0, 1, 0};
	}

	math::Vector<3> v2 = math::crossProd(v0, v1);
//	for(auto &val: v2) if(std::abs(val) < 1e-10) val = 0;
//	v2 = v2.normalized();

	std::vector<math::Vector<3>*> vecs{&v0, &v1, &v2};

	// orthogonalizeは失敗する可能性がある
	if(math::orthogonalize(vecs, 0) != vecs.size()) throw std::invalid_argument("get2OrthogonalUnitVectors failed.");
	assert(math::isSamePoint(v0, vorg));
	return std::make_pair(v1.normalized(), v2.normalized());
}



math::Vector<3> math::getOrthogonalUnitVector(const math::Vector<3> &refVec)
{
    using Vec = math::Vector<3>;
    // x,y,z方向との内積を計算して最も直交に近い方向ベクトルからグラムシュミットで直交化ベクトルを作成する。
    // normalに直交するベクトルv1は{1,0,0}, {0,1,0}, {0,0,1}の内もっとも内積が最も小さいものを元にして
    // グラムシュミット直交化法で作成する。
    Vec v1 = Vec{1,0,0};
    double prod = math::dotProd(v1, refVec);
    Vec tmpVec = Vec{0,1,0};
    double tmpProd = math::dotProd(tmpVec, refVec);
    if(prod*prod > tmpProd*tmpProd) {
        v1 = tmpVec;
        prod = tmpProd;
    }
    tmpVec = Vec{0, 0, 1};
    tmpProd = math::dotProd(tmpVec, refVec);
    if(prod*prod > tmpProd*tmpProd) {
        v1 = tmpVec;
        prod = tmpProd;
    }

    // ここから直交化
//    v1 = (v1 - prod/math::dotProd(refVec,refVec)*refVec).normalized();
//    return v1;
    return (v1 - prod/math::dotProd(refVec,refVec)*refVec).normalized();
}
