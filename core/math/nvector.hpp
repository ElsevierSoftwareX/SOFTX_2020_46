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
#ifndef NVECTOR_HPP
#define NVECTOR_HPP

#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

#include "constants.hpp"

/*
 * math::Vectorは行ベクトル。
 * もっともVector自体の実装は行ベクトルだろうが列ベクトルだろうが変わりはない。
 * math::Matrixとの関係性が変わる。
 */


namespace math {
// 前方宣言
template <unsigned int M> class Matrix;
template <unsigned int M> class Vector;
template <unsigned int M> double dotProd(const Vector<M>&v1, const Vector<M> &v2);
template <unsigned int M> Vector<M> crossProd(const Vector<M>&v1, const Vector<M> &v2);
template <unsigned int M> bool isDependent(const Vector<M> &v1, const Vector<M> &v2);



// NOTE mingwで#C++14が使えるようになったらconstexprにする。
template <unsigned int N>
class Vector {
#include "nvector_inl.hpp" // privateメンバは別ファイル。

// 頻出演算はfriendにする
template <unsigned int M> friend Vector<M> operator - (const Vector<M>& v1, const Vector<M> &v2);
template <unsigned int M> friend Vector<M> operator + (const Vector<M>& v1, const Vector<M> &v2);
template <unsigned int M> friend Vector<M> operator * (const double &a, const Vector<M> &v);
template <unsigned int M> friend void operator *= (Vector<M> &v, const double d);
template <unsigned int M> friend Vector<M> operator * (const Vector<M> &v, const Matrix<M> &m);
template <unsigned int M> friend double dotProd(const Vector<M>&v1, const Vector<M>&v2);
template <unsigned int M> friend Vector<M> crossProd(const Vector<M>&v1, const Vector<M> &v2);
template <unsigned int M> friend double distance(const Vector<M>&v1, const Vector<M> &v2);

public:
	typedef std::array<double, N> array_type;
	// ############ staticメンバ
	// 行列演算誤差を考えると最大桁数から4桁くらいは見ておかないと実用的ではない。
//	static constexpr double EPS = 10000*std::numeric_limits<double>::epsilon();
//	static constexpr double EPS = 1.0e-8; // 実際に演算してみると精度はこの程度
	static constexpr double EPS = math::EPS;
//	math::Vector::EPSは内部リンケージしか持たないら外部から参照できない
	static const double &eps() {
        static const double ep = Vector<N>::EPS;
		return ep;
	}
	// 点を動かすときのオフセット
	static constexpr double DELTA = 100*math::EPS;
	static const double &delta() {
		// 結局トーラスの交差判定(=4次式の解の計算)はたかだか1e-5程度の精度しかない。
		// それよりステップを小さくするとセル内外判定を失敗する
		// → セルのこのサイズより小さい部分はないものとされる
		//   → しかし1μmの放射化箔みたいなのは実際にあり得る。
		//     → だからMCNPで軸非平行トーラスは禁止されているのだろうけど。
		// ステップサイズを小さくしてもwhile内で内外判定をしっかりやれば別にOKでは？
		// 解の選択前にNewton法で修正をすれば精度は上がって内外判定は失敗しない。計算は増えるが。
		//static const double offset = 1000*math::EPS;
		//static const double offset = 10000*math::EPS;  // gccでは2.22E-7になる。
		// プラットフォーム間での再現性を確保するためにdelta()は固定値を使う。
		// デフォルトの単位系はcmなので1e-7の場合 粒子の最小ステップサイズは1nm
		// となりコレより小さい部分は飛び越えられてしまう。
		static const double offset = 1e-6;
		return offset;
	}

	// 原点==ゼロベクトル  TODO このへんC++14でzeroはconstexprにしてしまおう
	static const Vector<N> &ORIGIN() {
		static const Vector<N> zeroVec = initZeroVector();
		return zeroVec;
	}
	static const Vector<N> &ZERO() {
		static const Vector<N> zeroVec = initZeroVector();
		return zeroVec;
	}
	// 不適切なvector
	static const Vector<N> &INVALID_VECTOR(){
		static const Vector<N> npVec = initNotValid();
		return npVec;
	}

	// ############ コンストラクタ
	/*
	 * 生成時に0初期化
	 * std::array<double, N> or initializer_list, std::vector<N>で初期化
	 */
	Vector() {*this = ORIGIN();}
	// initializer_listはC++11ではconstexprではない。(C++14)
	explicit Vector(const std::initializer_list<double> &list) {
		if(N != list.size()) {
			std::cerr << "Vector<N>(initializer_list), list size and N should be the same." << std::endl;
			std::exit(EXIT_FAILURE);
		}
		size_t i = 0;
		for(auto it = list.begin(); it != list.end(); ++it) {
			data_[i] = *it;
			++i;
		}
	}
	explicit Vector(const double values[N] ) {
		for(size_t i = 0; i < N; ++i) data_[i] = values[i];
	}
	explicit Vector(const std::array<double, N> &arr):data_(arr){;}
	explicit Vector(const std::vector<double>& vec) {
		assert(N <= vec.size());
		for(size_t i = 0; i < N; ++i) {
			data_[i] = vec.at(i);
		}
	}


	// ############ アクセサ
	// iterator range-based forが使えるようにbegin/endを定義しておく
	typename std::array<double, N>::iterator begin() {
		return data_.begin();
	}
	typename std::array<double, N>::iterator end() {
		return data_.end();
	}
	inline double x() const {
		static_assert(N >= 1, "Vector size should be > 1 for x() method.");
		return data_[0];
	}
	inline double y() const {
		static_assert(N >= 2, "Vector size should be > 2 for y() method.");
		return data_[1];
	}
	inline double z() const {
		static_assert(N >= 3, "Vector size should be > 3 for z() method.");
		return data_[2];
	}
	inline double o() const {
		static_assert(N >= 4, "Vector size should be > 4 for o() method.");
		return data_[3];
	}
	std::array<double, N> &array() {return data_;}
	const std::array<double, N> &data() const {return data_;}

	// 要素アクセス
	double at(size_t i) const {return data_.at(i);}
	double &operator[](size_t i) {return data_[i];}
	// ############ その他public
	// vector長
	constexpr unsigned int size() const {return N;}
    // 部分vector
    template<unsigned int M>
    Vector<M> slice(size_t start = 0) {
        if(N < M + start) throw std::out_of_range("Vector slicing");
        typename Vector<M>::array_type retData;
        for(size_t i = 0; i < M; ++i) {
            retData.at(i) = data_.at(start + i);
        }
        return Vector<M>(retData);
    }

	// 規格化ベクトルを返す
	inline Vector<N> normalized() const {
		Vector<N> retVec = *this;
		double vAbs = this->abs();
		if(vAbs < N*EPS) {
			std::stringstream ss;
			ss << *this << ", vabs=" << vAbs << ", n*eps=" << N*EPS << std::endl;
            //abort();  // DEBUG
            throw std::invalid_argument("zerodiv in normalizing vector=" + ss.str());
		}
		return retVec/vAbs;
	}
	// Lnノルム norm(2)が絶対値
	inline double norm(int n) const {
		double sum = 0;
		for(size_t i = 0; i < N; ++i) {
			sum += std::pow(data_[i], n);
		}
		return std::pow(sum, 1.0/static_cast<double>(n));
	}
	// マイナス単項演算
	inline Vector<N> operator -() const {
		Vector<N> retVec = *this;
		for(size_t i = 0; i < N; ++i) {
			retVec.data_[i] *= -1.0;
		}
		return retVec;
	}
	// vectorの加減算代入
	Vector<N>& operator += (const Vector<N> &dv) {
		for(size_t i = 0; i < N; ++i) {
			data_[i] += dv.data_[i];
		}
		return *this;
	}
	Vector<N>& operator -= (const Vector<N> &dv) {
		for(size_t i = 0; i < N; ++i) {
			data_[i] -= dv.data_[i];
		}
		return *this;
	}

	// 絶対値
	double abs() const {
		return std::sqrt(math::dotProd(*this, *this));
	}
	// ゼロベクトルならtrueを返す
	bool isZero() const {
		for(size_t i = 0; i < data_.size(); ++i) {
			if(std::abs(data_[i]) > Vector<N>::EPS) return false;
		}
		return true;
	}
	// 妥当な値かチェック
	bool isValid() const {
		// 不適切なvectorは全成分がNOT_VALID値。ここでは最初の1成分だけ調べる。
		return std::abs(data_[0] - INVALID_VALUE) > N*EPS;
	}
	// 文字列へ変換
	std::string toString() const {
		std::stringstream ss;
		for(size_t i = 0; i < N; ++i) {
			ss << data_[i];
			if(i != N-1) ss << ", ";
		}
		return ss.str();
	}

private:
	std::array<double, N> data_;
};

typedef Vector<3> Point;

// 非メンバ関数についてはshared_ptr版も作成する。
// ############################## 非メンバ関数

enum class AXIS :int{X, Y, Z};

Vector<3> getAxisVector(AXIS axis);


// 内積非メンバ版
template<unsigned int M>
double dotProd(const Vector<M> &v1, const Vector<M> &v2) {
	double sum = 0;
	for(size_t i = 0; i < M; ++i) {
		sum += v1.data_[i]*v2.data_[i];
	}
	return sum;
}
template<unsigned int M>
double dotProd(std::shared_ptr<const Vector<M>> &v1, std::shared_ptr<const Vector<M>> &v2) {
	return dotProd(*v1.get(), *v2.get());
}

// 外積非メンバ
template <unsigned int M>
Vector<M> crossProd(const Vector<M> &v1, const Vector<M> &v2)
{
	static_assert(M == 3, "Cross Prod of N!=3 is not implemented yet.");
	Vector<M> retVec;
	retVec.data_[0] = v1.y()*v2.z() - v1.z()*v2.y();
	retVec.data_[1] = v1.z()*v2.x() - v1.x()*v2.z();
	retVec.data_[2] = v1.x()*v2.y() - v1.y()*v2.x();
	return retVec;
}
template<unsigned int M>
std::shared_ptr<Vector<M>> crossProd(std::shared_ptr<Vector<M>> &v1, std::shared_ptr<Vector<M>> &v2) {
	return std::make_shared<Vector<M>>(crossProd(*v1.get(), *v2.get()));
}

// スカラ3重積
template <unsigned int M>
double scalarTriProd(const Vector<M> &v1, const Vector<M> &v2, const Vector<M> &v3)
{
	return math::dotProd(math::crossProd(v1, v2), v3);
}
template<unsigned int M>
double scalarTriProd(std::shared_ptr<const Vector<M>> &v1,
					 std::shared_ptr<const Vector<M>> &v2,
					 std::shared_ptr<const Vector<M>> &v3) {
	return scalarTriProd(*v1.get(), *v2.get(), *v3.get());
}
// 距離は 差ベクトルのL2ノルム
template <unsigned int N>
double distance(const Vector<N> &v1, const Vector<N> &v2) {
	return (v1 - v2).abs();
}
template <unsigned int N>
double distance(const std::shared_ptr<Vector<N>> &v1, const std::shared_ptr<Vector<N>> &v2) {
	return distance(*v1.get(), *v2.get());
}


// 直交性チェック
template <unsigned int N>
bool isOrthogonal(const Vector<N> &v1, const Vector<N> &v2) {
	return std::abs(dotProd(v1, v2)) < Vector<N>::EPS;
}
template <unsigned int N>
bool isOrthogonal(const std::shared_ptr<Vector<N>> &v1, const std::shared_ptr<Vector<N>> &v2) {
	return isOrthogonal(*v1.get(), *v2.get());
}


// 2つのベクトルが従属(dependent)か調べる.内積の絶対値がL2normの積に等しければ一次従属。どちらかがゼロベクトルでも従属
template <unsigned int N>
bool isDependent(const Vector<N> &v1, const Vector<N> &v2) {
	if(v1.isZero() || v2.isZero()) return true;
	double prod = std::abs(math::dotProd(v1, v2));
	return std::abs(prod - v1.abs()*v2.abs()) < N*Vector<N>::EPS;
}
template <unsigned int N>
bool isDependent(const std::shared_ptr<Vector<N>> &v1, const std::shared_ptr<Vector<N>> &v2) {
	return isDependent(*v1.get(), *v2.get());
}

//// ソート用のLessファンクタ。コンストラクタは比較する要素のインデックス番号
//template <unsigned int N>
//class VLess {
//public:
//	VLess(size_t index):index_(index){;}
//	bool operator()(const Vector<N> &v1, const Vector<N> &v2)
//	{
//		return v1[index_] < v2[index_];
//	}
//private:
//	const size_t index_;
//};

// i番目の要素の最大最小ペアを返す。
template<unsigned int N>
std::pair<double, double> getMinMax(size_t index, const std::vector<Vector<N>> &vecs)
{
	if(vecs.size() == 1) return std::make_pair(vecs.front().at(index), vecs.front().at(index));
	double maxVal = std::numeric_limits<double>::lowest();
	double minVal = std::numeric_limits<double>::max();
	for(const auto &vec: vecs) {
		maxVal = (std::max)(maxVal, vec.at(index));
		minVal = (std::min)(minVal, vec.at(index));
	}
	return std::make_pair(minVal, maxVal);
}



// こっちがメインの実装
/*
 * グラム-シュミット直交化法でベクトルを直交化する。
 *
 * ベクトルが一次従属の場合、vecsには独立なベクトルの数に等しい個数分だけ
 * 直交化されたベクトルが格納され、それ以外の部分にはゼロベクトルが入る。
 * 返り値は直交化されたベクトルの数
 *
 */
#include <iostream>
template <unsigned int DIM>
std::size_t orthogonalize(const std::vector<Vector<DIM>*> &vecs, std::size_t NUM_LOOPS)
{
	typedef Vector<DIM> Vec;
	if(vecs.size() > DIM) {
		throw std::invalid_argument("Number of vectors should be smaller than their dimension.");
	}


    // ベクトルが一次従属の場合、途中で0ベクトルになる。このときは退化していると判定する。
    // 一次従属判定は単に引数ベクトルのペアの独立・従属を見るだけでは不十分
	// なのでむしろとりあえず直交化を試行して、途中で0ベクトルが出現したら退化していると判定すべし

	// 作業用にbasesへコピーする。
    std::vector<Vec> bases;
	for(size_t i = 0; i < vecs.size(); ++i) {
        if(!vecs[i]->isZero()) bases.emplace_back(*vecs[i]);
	}

	//グラム・シュミット直交化する前に各軸不偏でbasesを直交に近づける
	constexpr double k = 0.2;
	for(size_t l = 0; l < NUM_LOOPS; ++l) {
		for(size_t i = 0; i < bases.size(); ++i) {
			Vec correction = Vec::ORIGIN();
			for(size_t j = 0; j < bases.size(); ++j) {
				if(i != j) correction = correction - math::dotProd(bases[i], bases[j])/(math::dotProd(bases[j], bases[j])) *bases[j];
			}
            //std::cout << "i, correction=" << i << correction << std::endl;
			bases[i] += k*correction;
			//*(vecs[i]) = bases[i] + k*correction;
		}
	}

	// ここからグラム・シュミット直交化.ここまででbasesは直交に近いベクトル集となっている。(あるいは引数そのまま)

	for(size_t i = 0; i < bases.size(); ++i) {
		Vec correction = Vec::ORIGIN();
		for(size_t j = 0; j < i; ++j) {
			if(!bases[j].isZero()) correction -= math::dotProd(bases[i], bases[j]) / math::dotProd(bases[j], bases[j])  * bases[j];
		}
		//std::cout << "i===" << i << ", vec===" << *(vecs[i]) << "corvec===" << correction <<std::endl;
		bases[i] += correction;
		if(bases[i].abs() < Vector<DIM>::EPS) {
			//std::cout << "i===" << i << ", Zero vector." << std::endl;
			for(size_t j = 0; j < bases.at(i).size(); ++j) bases.at(i)[j] = 0; // 微妙な誤差をゼロでクリアする。
		}
	}
//    std::cout << "orthogonalized vector ===" << std::endl;
//    for(size_t i = 0; i < bases.size(); ++i) std::cout << bases.at(i) << std::endl;

	// ここでゼロベクトルは無視して詰める。
	for(size_t i = 0; i < vecs.size(); ++i) *(vecs[i]) = Vec::ZERO(); // 一旦0クリア
	std::size_t nonZeroIndex = 0;
	for(size_t i = 0; i < bases.size(); ++i) {
		if(!bases[i].isZero()) {
			*(vecs[nonZeroIndex]) = bases[i];
			++nonZeroIndex;
		}
	}
	return nonZeroIndex;

}
// DIM次元ベクトルNUM個の直交化。行列の直交化のためarrayを引数に取る版が必要
template <unsigned int DIM, size_t NUM>
std::size_t orthogonalize(std::array<Vector<DIM>, NUM> *vecArray, std::size_t NUM_LOOPS) {
	typedef Vector<DIM> Vec;
	std::vector<Vec*> vecs(vecArray->size());
	for(size_t i = 0; i < vecs.size(); ++i) {
		vecs[i] = &((*vecArray)[i]);
	}
	return orthogonalize(vecs, NUM_LOOPS);
}
template <unsigned int DIM, unsigned int NUM>
std::size_t orthogonalize(std::array<std::shared_ptr<Vector<DIM>>, NUM> *vecs, std::size_t &NUM_LOOPS) {
	std::array<Vector<DIM>, NUM> tmpVecs;
	for(size_t i = 0; i < NUM; ++i) {
		tmpVecs[i] = *(vecs->at(i));
	}
	std::size_t numVec =orthogonalize(&tmpVecs, NUM_LOOPS);
	for(size_t i = 0 ; i < NUM; ++i) (*vecs)[i] = tmpVecs[i];
	return numVec;
}
// 可変長版。パラメータパックはシグネチャがかぶらないように作るのが難しかったので
// initializer_listで作った。
template <unsigned int DIM>
std::size_t orthogonalize(std::initializer_list<Vector<DIM>*> ilist, size_t NUM_LOOPS)
{
	std::vector<Vector<DIM>*>vecs{ilist};
	return orthogonalize(vecs, NUM_LOOPS);
}





// 角度
template <unsigned int N>
double cosine(const Vector<N> &v1, const Vector<N> &v2)
{
	return dotProd(v1.normalized(), v2.normalized());
}
template <unsigned int N>
double cosine(const std::shared_ptr<Vector<N>> &v1, const std::shared_ptr<Vector<N>> &v2){
	return cosine(*v1.get(), *v2.get());
}


// vector同士の和・差
template <unsigned int N>
Vector<N> operator - (const Vector<N> &v1, const Vector<N> &v2) {
	std::array<double, N> arr;
	for(size_t i = 0; i < N; ++i) {
		arr[i] = v1.data_[i] - v2.data_[i];
	}
	return Vector<N>(arr);
}
template <unsigned int N>
std::shared_ptr<Vector<N>> operator - (const std::shared_ptr<Vector<N>> &v1, const std::shared_ptr<Vector<N>> &v2) {
	return std::make_shared<Vector<N>>(*v1.get() - *v2.get());
}

template <unsigned int N>
Vector<N> operator + (const Vector<N> &v1, const Vector<N> &v2) {
	std::array<double, N> arr;
	for(size_t i = 0; i < N; ++i) {
		arr[i] = v1.data_[i] + v2.data_[i];
	}
	return Vector<N>(arr);
}
template <unsigned int N>
std::shared_ptr<Vector<N>> operator + (const std::shared_ptr<Vector<N>> &v1, const std::shared_ptr<Vector<N>> &v2) {
	return std::make_shared<Vector<N>>(*v1.get() + *v2.get());
}
// 実数との積, 商
template <unsigned int N>
Vector<N> operator *(const double& d, const Vector<N>& v) {
	Vector<N> retVec = v;
	for(size_t i = 0; i < N; ++i) {
		retVec.data_[i] *= d;
	}
	return retVec;
}
template <unsigned int N>
std::shared_ptr<Vector<N>> operator *(const double& d, const std::shared_ptr<Vector<N>>& v) {
	return std::make_shared<Vector<N>>(d* (*v.get()));
}

template <unsigned int N>
Vector<N> operator *(const Vector<N>& v, const double &d) {
	return d*v;
}
template <unsigned int N>
std::shared_ptr<Vector<N>> operator *(const std::shared_ptr<Vector<N>>& v, const double& d) {
	return (d*v);
}

template <unsigned int N>
void operator *=(Vector<N>& v, const double d) {
	for(size_t i = 0; i < N; ++i) {
		v.data_[i] *= d;
	}
}
template <unsigned int N>
void operator *=(std::shared_ptr<Vector<N>>& v, const double d) {
	for(size_t i = 0; i < N; ++i) {
		v->data_[i] *= d;
	}
}

template <unsigned int N>
Vector<N> operator / (const Vector<N> &v, const double &d) {
	if(std::abs(d) < Vector<N>::EPS*N) throw std::invalid_argument("zerodiv in operator Vector/double");
	return v*(1.0/d);
}
template <unsigned int N>
std::shared_ptr<Vector<N>> operator / (const std::shared_ptr<Vector<N>> &v, const double &d) {
	return std::make_shared<Vector<N>>(*v.get()/d);
}



template <unsigned int N>
bool isSamePoint (const Vector<N> &v1, const Vector<N> &v2, double criterion = Vector<N>::EPS) {
	auto diffVec = v1 - v2;
	return math::dotProd(diffVec, diffVec) < criterion*criterion*N*N;
}

// 等価比較 operator == のoverloadは危険なので廃止。isSameを使う。
//template <unsigned int N>
//bool operator == (const Vector<N> &v1, const Vector<N> &v2) {
//	return isSamePoint(v1, v2);
//}
//template <unsigned int N>
//bool operator !=  (const Vector<N> &v1, const Vector<N> &v2) {
//	return !isSamePoint(v1, v2);
//}

// 流石に operator == (shared_ptr, shared_ptr)は同値判定に使われているのでoverrideするとやばい
template <unsigned int N>
bool isSamePoint(const std::shared_ptr<Vector<N>> &v1, const std::shared_ptr<Vector<N>> &v2, double criterion = Vector<N>::EPS) {
	return isSamePoint(*v1.get(), *v2.get(), criterion);
}

template <unsigned int N>
std::ostream& operator << (std::ostream& os, const Vector<N>& vec) {
	os << "{";
	os << vec.toString();
	os << "}";
	return os;
}
template <unsigned int N>
std::ostream& operator << (std::ostream& os, const std::shared_ptr<Vector<N>> &vec) {
	os << *vec.get();
	return os;
}

// 補助ルーチン
// 3次元限定の非テンプレート関数の宣言
std::pair<Vector<3>, Vector<3>> get2OrthogonalUnitVectors(const Vector<3> &vorg);
Vector<3> getOrthogonalUnitVector(const Vector<3> &refVec);

// 3次元ベクトルのdistanceは頻出なので展開したものを特殊化しておく
template <> double distance(const Vector<3> &v1, const Vector<3> &v2);
template <> double distance(const std::shared_ptr<Vector<3>> &v1, const std::shared_ptr<Vector<3>> &v2);



}  // end namespace math

#endif // NVECTOR_HPP
