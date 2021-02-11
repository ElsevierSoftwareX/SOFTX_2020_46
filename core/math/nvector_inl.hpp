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
#ifndef NVECTOR_INL_HPP
#define NVECTOR_INL_HPP

static constexpr double INVALID_VALUE = std::numeric_limits<double>::max();

inline static Vector<N> initZeroVector() {
	std::array<double, N> arr;
	for(size_t i = 0; i < N; ++i) {
		arr[i] = 0;
	}
	return Vector<N>(arr);
}
inline static Vector<N> initNotValid() {
	std::array<double, N> arr;
	for(size_t i = 0; i < N; ++i) {
		arr[i] = INVALID_VALUE;
	}
	return Vector<N>(arr);
}




#endif // NVECTOR_INL_HPP
