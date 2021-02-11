#ifndef UTILS_VECTOR_HPP_
#define UTILS_VECTOR_HPP_

#include <vector>
#include <algorithm>
#include <ostream>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <sstream>

#include "utils_numeric.hpp"

template < class T>
std::ostream& dump_vector( const std::vector<T> vec, std::ostream& os )
{
	typedef typename std::vector<T>::const_iterator cit;
	for( cit it=vec.begin(); it!=vec.end(); it++)
	{
		os<<*it<<std::endl;
	}
	return os;
}

//! ベクトルが昇順ならtrueをそれ以外ならfalseを返す。
template< class T >
bool is_ascending_order( const std::vector<T> vec)
{
	using namespace std;
//	typedef std::vector<T> vec_type;
	typedef  typename std::vector<T>::const_iterator vit;

	T previous_element = *( vec.begin() );

	for( vit it = ++vec.begin(); it!=vec.end(); it++ )
	{
		if ( previous_element >= *it )
		{
			return false;
		}

		previous_element = *it;
	}

	return true;
}

//! 群構造の中央値が格納されたベクターから群境界値のベクトルを作成する。
// 境界値ベクトルのサイズは中央値ベクトルのサイズより１大きくなる。
// MEMO 再下限 or 再上限の情報が無ければ中心値から境界値には正しく変換できない。(等間隔群構造の場合は除く)
template<class T>
std::vector<T> centervalue_to_uppervalue( const std::vector<T> cvec )
{
	typedef typename std::vector<T>                 Tvector;
	typedef typename std::vector<T>::const_iterator  Titerator;

	if( std::distance(cvec.begin(), cvec.end() ) <= 1 )
	{
		throw std::invalid_argument("Error! Center value vector has too few element.");
	}


	Tvector upper_vec;
	T width =  *(++cvec.begin()) - *cvec.begin();
	T lowest_bound =  *cvec.begin() - 0.5*width;
	if( lowest_bound < T(0) )
	{
		std::cerr<<"Warning! Lowest boundd is lower than zero."<<std::endl;
	}

	upper_vec.push_back( lowest_bound );

	// 結局最下限あるいは再上限のデータが無ければ中心値と境界値を完全に変換することはできない。
	for( Titerator it= ++cvec.begin(); it!=cvec.end(); it++)
	{
		upper_vec.push_back(  0.5*(  *(it-1) + *it  ) );
	}

	upper_vec.push_back(  *(--cvec.end()) + 0.5*( *--cvec.end()-*(cvec.end()-2) )    );

	return upper_vec;
}



//! 群構造の境界値が格納されたベクターから中央値のベクトルを作成する。
// 境界値ベクトルのサイズは中央値ベクトルのサイズより１大きい。
template< class T >
std::vector<T> uppervalue_to_centervalue( const std::vector<T> uvec )
{
	typedef typename std::vector<T>                 Tvector;
	typedef typename std::vector<T>::const_iterator  Titerator;

	Tvector cvec;

	for( Titerator it=uvec.begin()+1; it!=uvec.end(); it++)
	{
		cvec.push_back(    0.5*(   *it + *(it-1)  )   );
	}
	return cvec;
}


template< class XTYPE, class YTYPE >
YTYPE integral_histgram( const std::vector<XTYPE> xvec, const std::vector<YTYPE> yvec,
				      const XTYPE x_lower, const XTYPE x_upper )
{
	using namespace std;
	typedef typename vector<XTYPE>::const_iterator Xiterator;
	typedef typename vector<YTYPE>::const_iterator Yiterator;

	// 積分境界の大小が逆転している場合は例外を投げる。
	if( x_upper < x_lower )
	{
		std::stringstream ss;
		ss << "Error!"<<__FILE__<<":"<<__LINE__
		   <<" Integral upper limit(="<<x_upper
		   << ") is smaller than lower limit(="<<x_lower<<")." <<endl;
		throw invalid_argument( ss.str() );
	}
	// 積分範囲が0なら0を返す。
	else if( is_equal_numeric<XTYPE>(x_lower, x_upper) )
	{
//		std::cerr<<"Warning "<<__FILE__<<":"<<__LINE__
//			  <<" lower and upper integral limit is the same."<<endl;
		return YTYPE(0);
	}
	// xvec.size-1 == yvec.size でなければだめ。
	// ここでは yvec はヒストグラムの値で、xvecはヒストグラムのx境界値のため。
	else if ( ( xvec.size()-1 != yvec.size() ) || ( xvec.size() <= 1 )  )
	{
		std::stringstream ss;
		ss<<"Error!"<<__FILE__<<":"<<__LINE__
		  <<" Size of xvector(="<<xvec.size()<<") should be that of yvector(="
		  <<yvec.size()<<")+1 and more than 2."<<endl;
		throw invalid_argument( ss.str() );
	}


	// x_upper, x_lowerが所属する xvecの群のエネルギー上限を指すイテレータ
	const Xiterator it_xu = find_if( xvec.begin(), xvec.end(), bind2nd( greater<XTYPE>(), x_upper) );
	const Xiterator it_xl = find_if( xvec.begin(), xvec.end(), bind2nd( greater<XTYPE>(), x_lower) );


	if( it_xu == xvec.begin() )
	{
		// 積分範囲の上限がヒストグラムの存在範囲を下回っているので自動的に積分は0
		return YTYPE(0);
	}
	else if( it_xl == xvec.end() )
	{
		// 積分範囲の下限がヒストグラム存在範囲を上回っているので積分値は0
		return YTYPE(0);
	}


	// ここまでのエラー処理で想定されるケースは
	// 1. 積分範囲がヒストグラム存在範囲に収まる  → 特に対応不要
	// 2. 積分範囲上限のみがヒストグラム存在範囲上側にはみ出る it_xu == xvec.end()
	// 3. 積分範囲下限のみがヒストグラム存在範囲下側にはみ出る it_xl == xvec.begin()



	// x_upper と x_lower が同じ区間内に存在する場合。
	if( it_xu == it_xl )
	{
		Yiterator uit_ul = yvec.begin();
		advance( uit_ul, distance( xvec.begin(), it_xu )-1 );
		YTYPE density = *uit_ul / ( *it_xu - *(it_xu-1) );
		return static_cast<YTYPE>( density * (x_upper - x_lower )  );
	}

	// x_upper が所属する群の寄与。 ここで上記2. it_xu == xvec.end() に対処
	YTYPE contrib_xugroup =YTYPE(0);
	Yiterator uit_upper = yvec.begin();
	advance( uit_upper, distance( xvec.begin(), it_xu )-1 );
	// これでuit_upper が x_upperの所属する群のyvecを指す。
	// あるいは x_upper が上側にはみ出ている場合は uit_upper == yvec.end()
	if( !(it_xu == xvec.end()) )
	{
		YTYPE udensity = *uit_upper / ( *it_xu - *(it_xu-1) );
		contrib_xugroup = static_cast<YTYPE>(  udensity * ( x_upper - *(it_xu-1) )  );
	}


	// x_lower が所属する群の寄与。 ここで上記3. の it_xl == xvec.begin() に対応
	YTYPE contrib_xlgroup =YTYPE(0);
	Yiterator uit_lower = yvec.begin();
	advance( uit_lower, distance( xvec.begin(), it_xl )-1 );
	// ↑これでuit_lower が x_lowerの所属する群のyvecを指す。
	// あるいは x_lower が下側にはみ出ている場合は uit_lower == yvec.begin()
	if(  it_xl != xvec.begin() )
	{
		YTYPE ldensity = *uit_lower /  ( *it_xl - *(it_xl-1) );
		contrib_xlgroup = static_cast<YTYPE>(  ldensity * ( *(it_xl)  - x_lower )  );
	}


	// x_upper と x_lowerの間に群全体が含まれる群の寄与は
	//[--it_xu から ++it_xl] の間に対応したyvecの和、即ち[++uit_lowerから--uit_upper ] の和
	YTYPE contrib_inter = YTYPE(0);
	if( it_xu != it_xl ) //
	{
		for( Yiterator uit = ++uit_lower; uit != uit_upper; uit++)
		{
			contrib_inter += *uit;
		}
	}



	return contrib_xugroup + contrib_xlgroup
		+ contrib_inter;

}



template< class XTYPE, class YTYPE >
std::vector<YTYPE> adjust_group( const std::vector<XTYPE> org_xvec, const std::vector<YTYPE> org_yvec,
					    const std::vector<XTYPE> new_xvec  )
{
	typedef typename std::vector<XTYPE>::const_iterator xiterator;

	if( !is_ascending_order( new_xvec ) )
	{
		dump_vector( new_xvec, std::cerr );
		throw std::invalid_argument("Error! New X vector is not ascending order while adjusting group");
	}
	else if( new_xvec.size() <= 1 )
	{
		throw std::invalid_argument("Error! Size of new X vector should be  >=2 while adjusting group");
	}

	std::vector<YTYPE> new_yvec;

	for( xiterator it=++new_xvec.begin(); it!=new_xvec.end(); it++)
	{

		new_yvec.push_back( integral_histgram<XTYPE, YTYPE>( org_xvec, org_yvec ,*(it-1), *it) );
	}

	assert( new_yvec.size() == new_xvec.size()-1 );

	return new_yvec;
}


//! vectorの全成分が criteria ”以下”ならtrueを返す。
template< class T >
bool is_zero_vector( const std::vector<T> vec, T criteria=T(0) )
{
	for( typename std::vector<T>::size_type i=0; i<vec.size(); i++)
	{
		if( std::abs(vec[i]) > criteria )  return false;
	}

	return true;
}



//// 数値型要素を型変換
//template< class T, class U >
//std::vector<T> cast_elements( const std::vector<U> svec)
//{
//	std::vector<T> rvec( svec.size() );
//
//	for( typename std::vector<U>::size_type i=0; i<svec.size(); i++)
//	{
//		rvec[i]=boost::numeric_cast<T>( svec[i] );
//	}
//
//	return rvec;
//}


#endif /*UTILS_VECTOR_HPP_*/
