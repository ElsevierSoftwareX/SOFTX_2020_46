#ifndef UTILS_NUMERIC_HPP_
#define UTILS_NUMERIC_HPP_

#include <cmath>
#include <numeric>


template < class T >
bool is_zero_numeric( const T arg )
{
	return std::abs( arg )  <  1.0/std::numeric_limits<T>::max();
}


template < class T >
bool is_equal_numeric( const T arg1, const T arg2 )
{
	return  std::abs( arg1 - arg2 )/(0.5*(arg1+arg2))  
		 < std::pow(10.0, - std::numeric_limits<T>::digits10) ; 
}

#endif /*UTILS_NUMERIC_HPP_*/
