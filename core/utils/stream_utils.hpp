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
#ifndef STREAM_UTILS_HPP
#define STREAM_UTILS_HPP

#include <complex>
#include <deque>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// 有効arg桁で出力
#define SCIOUT(arg) std::scientific  << std::setw(arg+8) << std::setprecision(arg)

#include "core/math/nvector.hpp"
/*
 *
 *
 *
 * stream への << operatorを定義
 *
 */

#if defined(__GNUC__) && !defined(__clang__)
// 128bit float
inline
std::ostream &operator <<(std::ostream &os, __float128 d)
{
	os << static_cast<double>(d);
	return os;
}
#endif


// shared_ptr
template <class T>
std::ostream &operator << (std::ostream &os, const std::shared_ptr<T>& sp)
{
	if(sp == nullptr) {
		os << "nullptr";
	} else {
		os << *(sp.get());
	}
	return os;
}

// 複素数
template <class T>
std::ostream &operator <<(std::ostream &os, const std::complex<T> &c) {
	os << "{real=" << c.real() << ", imag=" << c.imag() << "}";
	return os;
}





// double 1次元アレイ
template<size_t N>
std::ostream& operator<<(std::ostream &os, const std::array<double, N> &arr){
	os << "{";
	for(size_t i = 0; i < N; ++i) {
		os << arr[i];
		if(i != N-1) os << ", ";
	}
	os << "}";
	return os;
}
// double 2次元アレイ
template<size_t N>
std::ostream& operator<<(std::ostream &os, const std::array<std::array<double, N>, N> &arr){
	for(size_t i = 0; i < N; ++i) {
		os << "{";
		for(size_t j = 0; j < N; ++j) {
			os << arr[i][j];
			if(j != N-1) os << ", ";
		}
		os << "}";
	}
	return os;
}

// T型1次元アレイ。だがしかしこのシグネチャはstd::内で定義されているoperator<<。
// の方にdeduceされて使えず::operator<<(os, arr);と呼ぶ必要が割とある。
template<class T, size_t N>
std::ostream &operator<<(std::ostream &os, const std::array<T, N> &arr)
{
	os << "{";
	for(size_t i = 0; i < N; ++i) {
		os << arr[i];
		if(i != N-1) os << ", ";
	}
	os << "}";
	return os;
}

// unordered_mapのstream出力
// unordered_map<Key, shared_ptr<Value>> <key, value>, <key, bool>を定義する。
template <class KEY, class VALUE>
std::ostream & operator<<(std::ostream &ost, const std::unordered_map<KEY, std::shared_ptr<VALUE>>& map) {
	ost << "{";
	for(auto it = map.begin(); it != map.end(); it++) {
//		ost << "{" << it->first << ", " << *((it->second).get()) << "}" << std::endl;
		ost << "{" << it->first << ", " << (it->second)->toString() << "}" << std::endl;
	}
	ost << "}";
	return ost;
}
template <class KEY, class VALUE>
std::ostream & operator<<(std::ostream &ost, const std::unordered_map<KEY, VALUE>& map) {
	ost << "{";
	for(auto it = map.begin(); it != map.end(); it++) {
		ost << "{" << it->first << ", " << it->second << "}" << std::endl;
	}
	ost << "}";
	return ost;
}
template <class KEY>
std::ostream & operator<<(std::ostream &ost, const std::unordered_map<KEY, bool>& map) {
	std::size_t sz = 0;
	for(auto it = map.begin(); it != map.end(); it++) {
		ost << "{" << it->first << ", " << std::boolalpha << it->second << "}";
		sz++;
		if(sz != map.size()) ost << std::endl;
	}
	return ost;
}
template <class KEY, class VALUE>
std::ostream & operator<<(std::ostream &ost, const std::map<KEY, std::shared_ptr<VALUE>>& map) {
    ost << "{";
    for(auto it = map.begin(); it != map.end(); it++) {
        ost << "{" << it->first << ", " << *((it->second).get()) << "}" << std::endl;
    }
    ost << "}";
    return ost;
}
template <class KEY, class VALUE>
std::ostream & operator<<(std::ostream &ost, const std::map<KEY, VALUE>& map) {
    ost << "{";
    for(auto it = map.begin(); it != map.end(); it++) {
        ost << "{" << it->first << ", " << it->second << "}" << std::endl;
    }
    ost << "}";
    return ost;
}
template <class KEY>
std::ostream & operator<<(std::ostream &ost, const std::map<KEY, bool>& map) {
    std::size_t sz = 0;
    for(auto it = map.begin(); it != map.end(); it++) {
        ost << "{" << it->first << ", " << std::boolalpha << it->second << "}";
        sz++;
        if(sz != map.size()) ost << std::endl;
    }
    return ost;
}


// vectorはこっち
template <class T>
std::ostream & operator<< (std::ostream &ost, const std::vector<T>& vec) {
	ost << "{" ;
	for(std::size_t i = 0; i < vec.size(); i++){
		ost << vec.at(i);
		if(i!=vec.size()-1) ost << ", ";
	}
	ost << "}";
	return ost;
}

// vectorはこっち
template <class T>
std::ostream & operator<< (std::ostream &ost, const std::deque<T>& vec) {
    ost << "{" ;
    for(std::size_t i = 0; i < vec.size(); i++){
        ost << vec.at(i);
        if(i!=vec.size()-1) ost << ", ";
    }
    ost << "}";
    return ost;
}


// listはこっち
template <class T>
std::ostream & operator<<(std::ostream &ost, const std::list<T>& tList)
{
	ost << "{";
	typename std::list<T>::const_iterator it = tList.begin();
	if(it != tList.end()) {
		while(true) {
			ost << *it;
			++it;
			if(it == tList.end()) {
				break;
			} else {
				//ost << ", ";
				ost << std::endl;
			}
		}
	}
	ost << "}";
	return ost;
}

// set
template <class T>
std::ostream &operator << (std::ostream &ost, const std::set<T> &tSet)
{
	return ost << std::vector<T>(tSet.begin(), tSet.end());
}

// unordered_set
template <class T>
std::ostream &operator << (std::ostream &ost, const std::unordered_set<T> &tSet)
{
	return ost << std::vector<T>(tSet.begin(), tSet.end());
}

// pair
template <class T, class U>
std::ostream &operator << (std::ostream &ost, const std::pair<T, U> &tPair)
{
	return ost << "{" << tPair.first << ", " <<  tPair.second << "}";
}

//template <unsigned int N>
//std::ostream &operator << (std::ostream &ost, const math::Vector<N> vec)
//{
//    ost << "{";
//    for(std::size_t i = 0; i < N; ++i) {
//        ost << vec.at(i);
//        if(i != N-1) ost << ", ";
//    }
//    ost << "}";
//    return ost;
//}





/*
 * この実装では
 * concTuple(tuple<T=int>)とconcTuple(tuple<HEAD=int, ARGS=<>>)
 * がテンプレレート解決時にambiguousとなってしまう。clangでは。
 */

//template < std::size_t... Ns , typename... Ts >
//auto tupleTail_impl( std::index_sequence<Ns...> , std::tuple<Ts...> tup )
//{	// ここでテンプレート引数が  std::get<1>(tup), std::get<2>(tup)と展開される
//	return  std::make_tuple( std::get<Ns+1u>(tup)... );
//}

//template <typename... Ts>
//auto tupleTail(std::tuple<Ts...> tup)
//{	// tail_implにはタプルtのサイズ-1までのシーケンスとtが渡される
//	return  tupleTail_impl(std::make_index_sequence<sizeof...(Ts) - 1u>() , tup);
//}

//// 連結の末端
//template<class T> static std::string concTuple(std::tuple<T> v) {
//    std::stringstream ss;
//    ss << std::get<0>(v);
//    return ss.str();
//}

//// tupleを文字列化して連結する。
//template<class HEAD, class... Args>
//std::string concTuple(std::tuple<HEAD, Args...> tup)
//{
//	std::stringstream ss;
//	HEAD val = std::get<0>(tup);
//	ss << val;
//	return ss.str() + ", " + concTuple(tupleTail(tup));
//}

// 無様な２成分実装
template<class T>
std::string concTuple(const std::tuple<T, T> &tup)
{
    std::stringstream ss;
    ss << std::get<0>(tup) << ", " << std::get<1>(tup);
    return ss.str();
}
// tuple
template <class ...ARGS>
std::ostream &operator << (std::ostream &ost, const std::tuple<ARGS...> &tTuple)
{
	ost << "{";
	ost << concTuple(tTuple);
	ost << "}";
	return ost;
}

/*
 * STL及び組み込み方のSTLコンテナ(ex. std::vector<int>)に対する
 * operator<<(std::ostream&, vector<int>)
 * はstd::内で定義されているtemplate関数にdeduceしようとして失敗して
 * エラーになるためユーザー定義でoverloadした関数を呼んでくれない。
 * 解決するには
 * ・使用箇所と同じ名前空間にユーザー定義してstd::より先に検索されるか
 * ・グローバル名前空間において::operator<<(...)で呼び出
 * の何れかが必要。でどちらも非現実的となる。
 *
 * そこでT型を保持する適当なユーザー定義型(PRINTABLE型)を経由して出力する。
 *
 */
template <class T>
class PRINTABLE
{
public:

//	PRINTABLE(const std::initializer_list<T> &il)
//	{
//		for(auto &val: il) {
//			buff_.emplace_back(val);
//		}
//	}

	PRINTABLE(const T& vals)
	{
		for(const auto &val: vals) {
			buff_.emplace_back(val);
		}
	}
//	explicit PRINTABLE(const std::vector<T> &container): buff_(container){;}
//	template <size_t M>
//	explicit PRINTABLE(const std::array<T, M> &container): buff_(container.begin(), container.end()){;}

	std::vector<typename T::value_type> buff_;
};

template<class T>
std::ostream &operator<<(std::ostream &os, const PRINTABLE<T> &p) {
	os << p.buff_;
	return os;
}



#endif // STREAM_UTILS_HPP
