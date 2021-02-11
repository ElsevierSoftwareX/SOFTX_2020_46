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
#include "logicalfunc.hpp"

#include "lpolynomial.hpp"

std::string lg::complimentedString(const std::string &equation)
{
//	auto lpoly = LPolynomial<std::string>::fromString(equation);
//	lpoly = lpoly.complimented();
//	return lpoly.toString();
	/*
	 * 実は面コンプリメントは入れ子にできるたとえば#(-2:#(1))
	 * なのでLogicalExpression::fromStringは#を考慮する必要がある。
	 */
	return LogicalExpression<std::string>::fromString(equation).complimented().toString();
}



