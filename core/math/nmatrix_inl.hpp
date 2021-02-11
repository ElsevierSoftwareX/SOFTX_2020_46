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
#ifndef NMATRIX_INL_HPP
#define NMATRIX_INL_HPP

// private static. 公開しない。
inline static Matrix<N> initZeroMatrix() {
	typename Matrix<N>::array_type mat;
	for(size_t j = 0; j < N; j++) {
		for(size_t i = 0; i < N; ++i) {
			mat[i][j] = 0.0;
		}
	}
	return Matrix<N>(mat);
}

inline static Matrix<N> initUnitMatrix() {
	typename Matrix<N>::array_type mat = Matrix<N>::ZERO().matrix();
	for(size_t i = 0; i < N; ++i) {
		mat[i][i] = 1.0;
	}
	return Matrix<N>(mat);
}

#endif // NMATRIX_INL_HPP
