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
#ifndef NMATRIX_HPP
#define NMATRIX_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <tuple>
#include "nvector.hpp"
#include "equationsolver.hpp"
#include "core/utils/message.hpp"

#ifndef M_PI
// cmathにない場合
    #define M_PI 3.14159265368979323846
#endif

namespace math {

// 指数が整数限定ではあるがconstexprなpow関数
template<class T>
constexpr double static_pow(T base, int exp) noexcept {
  //static_assert(exp >= 0, "Exponent must not be negative");
  return (exp == 0) ? 1
	  :  (exp > 0) ? static_pow(base, exp-1)*base
	  :              static_pow(base, exp+1)/base;
}


/*
 * 内部的にはstd::array<std::array<double, N>, N>、
 * つまり長さNのアレイがN個ある状態で保持している。
 * data[i]がそれぞれのarrayを返すので
 * data[i][j]で個別の要素にアクセスできる。
 * 内部のarrayについては行ベクトル要素としているので
 * data[i][j]でi行j列要素を返す。
 *
 * martix_はメモリ上ではarrayが連続しているので
 *  array0<double, N>, array1<double, N> ... arrayN-1<double, N>
 * となっている。
 * matrix[0] はarray0を返すので、データは同一行内データが連続して配置され
 * row-major配置となっている。
 * 故に行列ループでは列番号ループを深い方に配置した方がキャシュが聞いて高速となる。
 *
 * ＊重要＊
 * math::Vectorは行ベクトルとして扱う。
 * → ベクトルの変換は vec*Matrix のように右からの積を取る。
 * ※vtkは列ベクトルで左から積を取るので転置しなければならない
 *
 * キラリティ(chirality)は右手系とする。
 * → そもそもキラリティ効くところは？→外積、回転の向き
 *※vtkは右手系
 *
 * 回転角度は軸に対して右ネジの方向を正とする。
 * 従って横軸右側をxの正方向, 縦軸上側をyの正方向とすると回転角の正の方向は
 * 時計回りとなる。(通常の線形代数の教科書などと逆になる)
 * つまり回転角は右手系と比べると逆になる。
 * 例：Vec{1,0}を90°回転するとVec{0,-1}になる。
 *
 * 三角形の法線は
 * 時計回りが表
 *
 * 行ベクトル、行メジャー、右手系 という選択はDirectX(=実例で学ぶゲーム3D数学)を右手系にしたスタイルになる。
 *
 *
 *　まとめ
 * ・行ベクトルか列ベクトルか？　→　ベクトルに行列を掛けるときに前からか後ろからかに関わる。あと基底が行ベクトルか列ベクトル化
 * ・左手系か右手系か？　→　軸回り回転の回転角の正負が異なる。外積の符号も
 *
 * Matrixの回転関係はキラリティの影響を受ける。このヘッダはキラリティ依存演算は含まないが、matrix_utilsあたりは注意
 *
 *
 */
/*
 * 対角化について
 * ベクトルを列ベクトルとして定義している場合(大体の数学教科書はこっち)
 * P-1 A P で対角化するときには
 * Pは固有列ベクトルを行方向に並べたものとなる。
 *
 * 一方当コードのように基本ベクトルを行ベクトルとしている場合は
 * 対角化行列は固有行ベクトルを列方向に並べれば良い…と一見思うが、
 * 行ベクトル体系では行列は後ろから掛けるので、
 * 列ベクトル体系の A*B*Cは 行ベクトル体系の C*B*Aに等しいく、
 * このように作った対角化行列の正しい作用順は
 * P A P^-1
 * となる。
 *
 * よって方法としては
 * 1. P-1 A P で対角化できるようにPは固有列ベクトルで作成する
 * 2. P A P^-1 で対角化する
 * のどちらかを採る必要がある。
 *
 * 行ベクトル体系であることを考えると2．が自然
 */


template <unsigned int N>
class Matrix {
#include "nmatrix_inl.hpp"
	template <unsigned int M> friend Matrix<M> minorMatrix(const size_t &row, const size_t &col);
	template <unsigned int M> friend Matrix<M> operator * (const double &d, const Matrix<M> &mat);

public:
	typedef std::array<std::array<double, N>, N> array_type;
	// staticメンバ
	static constexpr double EPS = std::numeric_limits<double>::epsilon()*static_pow(10, N+1)*100;
	//static constexpr double EPS = 1e-7;
	// 0行列、単位行列はconstexprにしたいが、そのためにはC++14を待たなければならない。
	static const Matrix<N>& ZERO(){
		static const Matrix<N> mat(initZeroMatrix());
		return mat;
	}
	static const Matrix<N>& IDENTITY() {
		static const Matrix<N> mat(initUnitMatrix());
		return mat;
	}

	// 正規直交化
	static void orthonormalize(Matrix<N> *srcMat, int numRelax = 20) {
		//orthogonalize(srcMat);
		std::array<Vector<N>, N> rowVecs;
		for(size_t i = 0; i < N; ++i) {
			rowVecs[i] = srcMat->rowVector(i);
		}

		if(math::orthogonalize(&rowVecs, numRelax) == rowVecs.size()) {
			for(size_t i = 0; i < N; ++i) {
				try {
					rowVecs[i] = rowVecs[i].normalized();
				} catch (std::invalid_argument &e) {
					std::stringstream ss;
					ss << "In orthonormalizing matrix =" << *srcMat << ", " << e.what();
					throw std::invalid_argument(ss.str());
				}
			}
			*srcMat = Matrix<N>(rowVecs);
		} else {
			// 行ベクトルが一次従属の場合直交化に失敗するのでその処理をしなければならない。
			std::stringstream ss;
			ss << "Orthogonalize vectors failed, vec=\n";
			for(size_t i = 0; i < N; ++i) {
				ss << rowVecs[i];
			}
			throw std::invalid_argument(ss.str());
		}

	}

	/*
	 * コンストラクタ
	 * ・引数なしでゼロ行列
	 * ・二次元アレイ(array<array,N>,n>)で初期化
	 * ・N*N個のinitializer_listで初期化
	 * ・行ベクトルのアレイ(array<Vector<N>,N>で初期化
	 * のいずれかをとる。
	 *
	 * あとsemanticsのはっきりしたfromRowVectors(), fromColVectors関数を作るべし。
	 */
	Matrix(){*this = ZERO();}  // 省略時は0行列
    explicit Matrix(const std::array<std::array<double, N>, N>& mat):matrix_(mat){;} // 二次元アレイ初期化
	explicit Matrix(const std::initializer_list<double> &list) {  // N*N個要素のinitializer_list
		// #C++14ならstatic_assertでコンパイル時チェックにできる
		assert(list.size() == N*N);
		if(list.size() != N*N) {
			std::cerr << "Error: Size of initializer_list." << std::endl;
			std::exit(EXIT_FAILURE);
		}
		size_t nindex = 0;
		for(auto it = list.begin(); it != list.end(); ++it) {
			matrix_[nindex/N][nindex%N]  = *it;
			++nindex;
		}
	}
	explicit Matrix(const std::array<math::Vector<N>, N> &vecs){  //行ベクトルアレイでもOK
		for(size_t i = 0; i < N; ++i) { matrix_[i] = vecs[i].data(); }
	}
	static Matrix<N> fromRowVectors(const std::initializer_list<Vector<N>> &list)
	{
		std::array<math::Vector<N>, N> varray;
		size_t i = 0;
		for(const auto& vv: list) {
			varray[i] = vv;
			i++;
		}
		return Matrix<N>(varray);
	}
	static Matrix<N> fromColVectors(const std::initializer_list<Vector<N>> &list)
	{
		return fromRowVectors(list).transposed();
	}

	// i, jでconstアクセスできるメソッド
	double &operator()(const std::size_t& i, const std::size_t& j) { return matrix_[i][j]; }
	double operator()(const std::size_t& i, const std::size_t& j) const { return matrix_[i][j]; }
	const std::array<std::array<double, N>, N>& matrix() const { return matrix_;}
	size_t size() const {return matrix_.size();}


	// 単項マイナス演算子
	Matrix<N> &operator -() {
		for(size_t i = 0; i < N; ++i) {
			for(size_t j = 0; j < N; ++j) {
				matrix_ [i][j] *= -1.0;
			}
		}
		return *this;
	}
	// 転置行列
	Matrix<N> transposed() const {
		typename Matrix<N>::array_type retArray;
		for(size_t i = 0; i < N; ++i) {
			for(size_t j = 0; j < N; ++j) {
				retArray[i][j] = matrix_[j][i];
			}
		}
		return Matrix<N>(retArray);
	}
	// 行ベクトル、列ベクトルのslice
	Vector<N> rowVector(const size_t &n) const {
		return Vector<N>(matrix_[n]);
	}
	Vector<N> colVector(const size_t &n) const {
		typename Vector<N>::array_type arr;
		for(size_t i = 0; i < N; ++i) {
			arr[i] = matrix_[i][n];
		}
		return Vector<N>(arr);
	}
	// 直交行列チェック。
	bool isOrthogonal() const {
		return isSameMatrix((*this)*this->transposed(), Matrix<N>::IDENTITY());
	}
	// 対角行列チェック
	bool isDiagonal() const {
		std::cout << "EPS===" << EPS << std::endl;
		for(size_t i = 0; i < N; ++i) {
			for(size_t j = 0; j < N; ++j) {
				if(i != j && std::abs(this->operator()(i, j)) > EPS) return false;
			}
		}
		return true;
	}
	// 正則行列チェック
	bool isRegular() const {
		return std::abs(determinant()) > Matrix<N>::EPS;
	}
	// 対称行列チェック
	bool isSymmetric() const {
		for(size_t i = 0; i < N; ++i) {
			for(size_t j = 0; j < N - i; ++j) {
				if(std::abs(this->operator ()(i, j) - this->operator ()(j, i)) > EPS) return false;
			}
		}
		return true;
	}
	// 逆行列
	Matrix<N> inverse() const {
		if(!isRegular()) return ZERO();
		Matrix<N> retMatrix;
		for(size_t i = 0; i < N; ++i) {
			for(size_t j = 0; j < N; ++j) {
				retMatrix.matrix_[i][j] = coFactor(j, i);
			}
		}
		return (1.0/determinant())*retMatrix;
	}
	// row とcolを除去した部分小行列
	Matrix<N-1> minorMatrix(const size_t &row, const size_t &col) const {
		typename Matrix<N-1>::array_type retArray;
		for(size_t i = 0; i < N; ++i) {
			for(size_t j = 0; j < N; j++) {
				if(i != row && j != col) {
					size_t rindex = i, cindex = j;
					if(rindex > row) rindex = i - 1;
					if(cindex > col) cindex = j - 1;
					retArray[rindex][cindex] = matrix_[i][j];
				}
			}
		}
		return Matrix<N-1>(retArray);
	}
	// 行列式
	double determinant() const {
		double det = 0;
		if(N > 1) {
			for(size_t j = 0; j < size(); ++j) {
				det += matrix_[0][j] * coFactor(0, j);
			}
		}
		return det;
	}
	// 余因子は部分小行列の行列式
	double coFactor(const size_t &row, const size_t &col) const {
		Matrix<N-1> minorMat = minorMatrix(row, col);
		return std::pow(-1, row + col)* (minorMat.determinant());
	}
    // 階数
    int rank() const {
        // N==1ならば要素が0でなければランクは1
        if(N==1) return (std::abs(this->operator ()(0, 0)) < Matrix<N>::EPS) ? 0 : 1;

        // グラムシュミット直交化法による方法
        // グラム-シュミットで直交化できた行ベクトルの数に等しいのでそれを使う。
        std::vector<Vector<N>> vecs(N);
        std::vector<Vector<N>*> vecps(N); // orthogonalize関数はポインタのベクトルを引数に取るので変換する
        for(size_t i = 0; i < N; ++i) {
            vecs[i] = rowVector(i);
            if(!vecs[i].isZero()) vecs[i] = vecs[i].normalized();
            vecps[i] = &vecs[i];
        }
        return orthogonalize(vecps, 20);

        //            // 非ゼロ固有値の数による方法 (正直あまり↑と変わらない)は非対称行列には適用できない。
        //            std::vector<double> evalues;
        //            std::vector<math::Vector<N>> evectors;
        //            this->symEigenVV(&evalues, &evectors, true, true);
        //            auto count = std::count_if(evalues.cbegin(), evalues.cend(),
        //                                       [](double d) {return std::abs(d) > math::Matrix<N>::EPS;});
        //            return static_cast<int>(count);
    }

        // トレース
	double trace() const{
		double retval = 0;
		for(size_t i = 0; i < N; ++i) {
			retval += this->operator ()(i, i);
		}
		return retval;
	}


    // Jacobi法で対称行列の固有値と固有ベクトルを計算する。
    // 固有値は降順かつゼロは末尾にソートされる。
	// 返り値は符号数
	std::tuple<int, int> symEigenVV(std::vector<double>* vals, std::vector<Vector<N>>* vecs, bool doSort, bool warn = true) const
	{
		// どうせ対称行列しか使わないんだからjacobi法で固有値・固有ベクトル求める方法を実装しておく。
		// Jacobi法なので非対称行列はエラーにする。
		if(!isSymmetric()) {
			throw std::invalid_argument("Calculation of eigen values and vectors are implemented only for a symmetric matrix");
		}
                //const double CRITERION = EPS*1000;
                const double CRITERION = EPS;
                const size_t MAX_ITERATION = 100000;
		size_t loopCounter = 0;
		auto targetMatrix = *this;
		size_t p=0, q=0;
		double nonDiagonalMaxElement = targetMatrix.maxSymmetricNonDiagonalElement(&p, &q);
		Matrix<N> givensMat, givensMatTr;
		Matrix<N> eigenVecsMat = Matrix<N>::IDENTITY();
//		mDebug() << "Initial loop===" << loopCounter << "non-diagonal-max index ===" << p << q
//				 << "non-diagonal-max===" << nonDiagonalMaxElement << "\nmatrix===" << targetMatrix;
		double prev_nonDiagonalMax = nonDiagonalMaxElement;
		do{
			// ギブンス回転行列を作る
			givensMat = Matrix<N>::IDENTITY();
			double beta = -1*nonDiagonalMaxElement;
			double alpha = 0.5*(targetMatrix(p, p) - targetMatrix(q,q));
			// ここでα=β=0 ならgammaがnanになる。
			// が、そもそもβ=0ならgivens回転行列を作用させる目的(targetMatrixの対角化)を達成している
			// から適当にbreakすれば良い。
			if(std::abs(beta) < Matrix<N>::EPS) {
				eigenVecsMat = Matrix<N>::IDENTITY();
				break;
			}
			double gamma = std::abs(alpha)/std::sqrt(alpha*alpha + beta*beta);
			double sgn = (alpha*beta > 0) ? 1 : -1;
			double cosTheta = std::sqrt(0.5*(1+gamma)), sinTheta = std::sqrt(0.5*(1-gamma))*sgn;
			givensMat(p, p) = cosTheta;
			givensMat(q, q) = cosTheta;
			givensMat(p, q) = sinTheta;
			givensMat(q, p) = -sinTheta;
			givensMatTr = givensMat.transposed();
			targetMatrix = givensMatTr*targetMatrix*givensMat;
			//eigenVecs = eigenVecs*givensMat;        // 列ベクトルが固有ベクトル
			eigenVecsMat = givensMatTr*eigenVecsMat;  // 行ベクトルが固有ベクトル
			nonDiagonalMaxElement = targetMatrix.maxSymmetricNonDiagonalElement(&p, &q);
			if(++loopCounter > MAX_ITERATION) {
				throw std::invalid_argument("In symEigenVV, number of iteration exceeds the limit");
			}
			// 収束判定
			double diff = prev_nonDiagonalMax - nonDiagonalMaxElement;
                        //mDebug() << "loop===" << loopCounter << "non-diagonal-max===" << nonDiagonalMaxElement << "diff===" << diff;
			if(std::abs(nonDiagonalMaxElement) < CRITERION || std::abs(diff) < Matrix<N>::EPS) break;
			prev_nonDiagonalMax = nonDiagonalMaxElement;
		} while(true);

		if(std::abs(nonDiagonalMaxElement) > CRITERION) {
			if(warn) mWarning() << "In calculating eigenvalues, non-diagonal element is larger than the criterion, residual ="
							  << std::abs(nonDiagonalMaxElement);
		}


		// targetMatrixとeigenVecMatからvalsとvecsへパッキングする。
        // 固有値が降順かつ0を末尾になるように並べ替えをする。
		typedef std::pair<double, Vector<N>> VVpair; // 固有値と固有ベクトルは連動してソートする必要があるのでペアを作る。
		std::vector<VVpair> vvVec;
		vvVec.reserve(N);
        for(size_t i = 0; i < N; ++i) vvVec.emplace_back(std::make_pair(targetMatrix(i,i), eigenVecsMat.rowVector(i)));
        if(doSort) std::sort(vvVec.begin(), vvVec.end(),
                             [](const VVpair &p1, const VVpair &p2){
            if(std::abs(p1.first) < math::EPS) {
                return false;
            } else {
                return p1.first > p2.first;
            }
        });
		vals->clear();
		vals->reserve(N);
		vecs->clear();
		vecs->reserve(N);
		for(size_t i = 0; i < N; ++i) {
			vals->emplace_back(std::move(vvVec.at(i).first));
			vecs->emplace_back(std::move(vvVec.at(i).second));
		}
		// returnする符号数を計算
		std::tuple<int, int> sig;
		// 行列計算の数値誤差はわりと大きいので
		// ゼロ判定は単にEPSを使うのではなく、
		// 絶対値最大の固有値で規格化した値をEPSと比較する。
                // → しかしそうするとrankとかのゼロ判定基準と矛盾が出るので注意。
		//mDebug() << "Debug vals ===" << *vals << ", EPS===" << Matrix<N>::EPS;
		const double maxval = std::abs(*std::max_element(vals->cbegin(), vals->cend()));
		if(std::abs(maxval) < Matrix<N>::EPS) return std::make_tuple(0, 0);
                double sigCriterion = Matrix<N>::EPS*maxval;
		for(size_t i = 0; i < vals->size(); ++i) {
			//mDebug() << "val/maxval ===" << vals->at(i)/maxval << "dps===" <<  Matrix<N>::EPS;
                        if(vals->at(i)/maxval >=  sigCriterion) {
				++std::get<0>(sig);
                        } else if(vals->at(i)/maxval < -sigCriterion) {
				++std::get<1>(sig);
			}
		}
		return sig;
	}

	// 符号数(signature).
	std::tuple<int, int> signature() const{
		std::vector<double> dummy1;
		std::vector<Vector<N>> dummy2;
		return symEigenVV(&dummy1, &dummy2, false, false);
	}

	// #######  4x4特有メソッド
	// 4x4行列の左上3x3部分, 3x3行列の左上2x2部分、が回転行列なのでそこを抜き出す。
	inline Matrix<N-1> rotationMatrix() const {
		static_assert(N==4 || N == 3, "rotationMatrix() for N!=4/3 is not valid.");
		typename Matrix<3>::array_type retArray;
		for(size_t j = 0; j < N-1; ++j) {
			for(size_t i = 0; i < N-1; ++i) {
				retArray[i][j] = matrix_[i][j];
			}
		}
		return Matrix<3>(retArray);
	}
	// 並進ベクトル抜き出し
	inline Vector<3> translationVector() const {
		static_assert(N==4, "translationVector() for N!=4 is not defiend.");
		typename Vector<3>::array_type retVec;
		retVec[0] = matrix_[3][0];
		retVec[1] = matrix_[3][1];
		retVec[2] = matrix_[3][2];
		return Vector<3>(retVec);
	}
	// 回転行列セット
	void setRotationMatrix(const Matrix<3> &rotMat) {
		static_assert(N==4, "setRotationMatrix for N!=4 is not defiend.");
		for(size_t i = 0; i < 3; ++i) {
			for(size_t j = 0; j < 3; ++j) {
				matrix_[i][j] = rotMat(i, j);
			}
		}
	}
	// 並進ベクトルセット
	void setTranslationVector(const Vector<3> & transVec) {
		static_assert(N==4, "setTranslationVector() for N!=4 is not defiend.");
		matrix_[3][0] = transVec.data()[0];
		matrix_[3][1] = transVec.data()[1];
		matrix_[3][2] = transVec.data()[2];
	}
	// 1次元アレイに変換
	std::array<double, N*N> toArray() const {
		std::array<double, N*N> retArray;
		for(std::size_t i = 0; i < N; ++i) {
			for(std::size_t j = 0; j < N; ++j) {
				retArray[i*N + j] = matrix_[i][j];
			}
		}
		return retArray;
	}


private:
	// marix_[i][j] i:行番号、j:列番号とする。
	std::array<std::array<double, N>, N> matrix_;

	// 対称行列の非対角要素の絶対値最大成分を返す。引数にはその時の行indexと列indexが格納される
	double maxSymmetricNonDiagonalElement(size_t *p, size_t *q)
	{
		double retval = 0, tmp;
		for(size_t i = 0; i < N; ++i) {
			for(size_t j = i+1; j < N; ++j) {  // 上半分のみの比較
				tmp = this->operator ()(i, j);
				if(std::abs(retval) < std::abs(tmp)) {
					retval = tmp;
					*p = i;
					*q = j;
				}
			}
		}
		return retval;
	}
};


// determinantの再帰をストップさせるN=1特殊化
template <>
class Matrix<1> {
public:
	typedef std::array<std::array<double, 1>, 1> array_type;
	double determinant() const {
		return matrix_[0][0];
	}
	explicit Matrix<1>(const array_type& arr):matrix_(arr){;}
private:
std::array<std::array<double, 1>, 1> matrix_;

};


// テンソル積によるベクトルからの生成
// Vector<N>は行ベクトル。
// 故に第1引数は列ベクトルとして転値されているとして扱う
template <unsigned int N>
Matrix<N> tensorProd(const math::Vector<N> &v1, const math::Vector<N> &v2)
{
	std::array<std::array<double, N>, N> retArray;
	for(size_t j = 0; j < N; ++j) {
		for(size_t i = 0; i < N; ++i) {
			// v1は転置された列ベクトルとして扱う
			retArray[i][j] = v1.at(j)*v2.at(i);
		}
	}
	return Matrix<N>(retArray);
}

// 行列と行列の積 とりあえず正方行列同士
template <unsigned int N>
Matrix<N> operator * (const Matrix<N>& a, const Matrix<N>& b) {
	typename Matrix<N>::array_type tmpArray;
	double sum;
	for(std::size_t j = 0; j < N; j++) {
		for(std::size_t i = 0; i < N; i++) {
			sum = 0.0;
			for(std::size_t k = 0; k < N; k++) {
				sum += a(i,k)*b(k,j);
			}
			tmpArray[i][j] = sum;
		}
	}
	return Matrix<N>(tmpArray);
}
// 行列と行列の和
template<unsigned int N>
Matrix<N> operator +(const Matrix<N>& a, const Matrix<N>& b) {
	typename Matrix<N>::array_type tmpArray;
	for(std::size_t j = 0; j < N; j++) {
		for(std::size_t i = 0; i < N; i++) {
			tmpArray[i][j] = a(i, j) + b(i, j);
		}
	}
	return Matrix<N>(tmpArray);
}
// 行列と行列の差
template<unsigned int N>
Matrix<N> operator - (const Matrix<N>& a, const Matrix<N>& b) {
	typename Matrix<N>::array_type arr;
	for(std::size_t j = 0; j < N; j++) {
		for(std::size_t i = 0; i < N; i++) {
			arr[i][j] = a(i, j) - b(i, j);
		}
	}
	return Matrix<N>(arr);
}



template <unsigned int N>
bool isSameMatrix(const Matrix<N> &a, const Matrix<N> &b)
{
	for(std::size_t i = 0; i < N; i++) {
		for(std::size_t j = 0; j < N; j++) {
			if(std::abs(a(i, j) - b(i, j)) > Matrix<N>::EPS) return false;
		}
	}
	return true;
}



// vectorと行列の積
template<unsigned int N>
Vector<N> operator * (const Vector<N>& v, const Matrix<N>& m) {
	Vector<N> retVec;
	for(size_t i = 0; i < N; ++i) {
		retVec.data_[i] = math::dotProd(v, m.colVector(i));
	}
	return retVec;
}

// 定数と行列の積
template <unsigned int N>
Matrix<N> operator * (const double &d, const Matrix<N> &mat) {
	Matrix<N> retMat = mat;
	for(size_t i = 0; i < N; ++i) {
		for(size_t j = 0; j < N; ++j) {
			retMat.matrix_[i][j] *= d;
		}
	}
	return retMat;
}



// ストリーム書き出し
template <unsigned int N>
std::ostream & operator <<(std::ostream &os, const math::Matrix<N> &mat)
{
	os << "{\n";
	for(std::size_t i = 0; i < N; i++) {
		os << "{";
		os << mat.rowVector(i);
		os << "}\n";
	}
	os << "}";
	return os;
}

// ロドリゲスの公式でvecを軸axisに対してθ回転する行列を作成する
inline
Matrix<3> generateRotationMatrix2(const Vector<3>& axis, double radians)
{
	assert(std::abs(axis.abs()) > math::EPS);

    auto n = axis.normalized();
    double n1 = n.x(), n2 = n.y(), n3 = n.z();
	double c = std::cos(radians);
	//double s = -std::sqrt(1-c*c); ダメです。
	double s = std::sin(radians);
	assert(!std::isnan(s));

    // 行ベクトルに対する回転行列なので列ベクトルに対する回転の転置になる。
    Matrix<3> mat {
        c + n1*n1*(1-c),     n1*n2*(1-c) + n3*s,  n1*n3*(1-c) - n2*s,
        n1*n2*(1-c)-n3*s,    c + n2*n2*(1-c),     n2*n3*(1-c) + n1*s,
        n1*n3*(1-c) + n2*s,  n2*n3*(1-c) - n1*s,  c + n3*n3*(1-c)
    };
	return mat;
}

// v2 をv1へ回転する行列を生成する
inline
Matrix<3> generateRotationMatrix1(const Vector<3> &vec1, const Vector<3> &vec2)
{
	assert(std::abs(vec1.abs()) > math::EPS);
	assert(std::abs(vec2.abs()) > math::EPS);
    if(math::isDependent(vec1, vec2)) {
        // 2つのベクトルが同方向
        if(math::dotProd(vec1, vec2) > 0) {
            return Matrix<3>::IDENTITY();
        } else {
            // 正反対の方も回転で対応する必要が有る。
            // これはダメ。座標反転だと位置関係が変わる
            //return -1*Matrix<3>::IDENTITY();
            // 従ってvec1に直交する方向を適当に選んでその軸周りで180度回転させる。行列
            auto axis = math::getOrthogonalUnitVector(vec1);
            return generateRotationMatrix2(axis, M_PI);
        }
    }

	auto v1 = vec1.normalized(), v2 = vec2.normalized();
    return generateRotationMatrix2(math::crossProd(v1, v2), -std::acos(math::dotProd(v1, v2)));
}

/*
 *  0.5√2,  0,    0.5√2,  0
 *  0,       1,    0,       0
 * -0.5√2,  0,    0.5√2,  0
 * Vx,      Vy,    Vz,      1
 *
 * はy軸回り45°回転と (Vx, Vy, Vz)並進の行列である。
 * この変換を適用すると、回転→並進の順序で適用される。
 *
 */
// 3次元ベクトルのアフィン変換
template <unsigned int N>
void affineTransform(Vector<N> *vec, const Matrix<N+1> &matrix)
{
    typedef Vector<N+1> VecType;
    typename Vector<N+1>::array_type tmpArray;
    typename Vector<N>::array_type tmpArray2 = vec->data();
    for(size_t i = 0; i < tmpArray.size()-1; ++i) {
        tmpArray.at(i) = tmpArray2.at(i);
    }
    tmpArray[tmpArray.size()-1] = 1.0;
	// ここでtmpArrayは {vx, vy, vz, 1}となっている。
    *vec = (VecType(tmpArray)*matrix).template slice<N>(0);
}

template <unsigned int N>
Matrix<N+1> createAffineMatrix(const Matrix<N> &mat, const Vector<N> &trans)
{
	// N < 3の場合しか機能しない。というかそもそもsetRotaionMatrix自体
	// 4次の行列にしかないからコンパイルエラーは出るはずだけど。
	static_assert(N <= 3, "AffineMatrix is defined for N<=3");
	auto retMat = Matrix<N+1>::IDENTITY();
	retMat.setRotationMatrix(mat);
	retMat.setTranslationVector(trans);
	return retMat;
}





}  // end namespace math


#endif // NMATRIX_HPP
