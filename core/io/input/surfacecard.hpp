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
#ifndef SURFACECARD_HPP
#define SURFACECARD_HPP

#include <limits>
#include <map>
#include <memory>
#include <regex>
#include <unordered_map>
#include <string>
#include <vector>

#include "core/math/nmatrix.hpp"

namespace inp{

struct SurfaceCard
{
	SurfaceCard(const std::string &n, const std::string &sym, int trn,
				const std::vector<double> &pms, const std::string &trString,
				const std::map<std::string, std::string>& sMap)
		:name(n), symbol(sym), trNumber(trn), params(pms), trStr(trString), paramMap(sMap) {;}

	static SurfaceCard fromString(const std::string& cardStr,
								  bool checkValidUserInput = false,
								  bool disableFortran = false, bool disableIjmr = false);
	static constexpr int NO_TR = std::numeric_limits<int>::max();

	std::string file;
	unsigned long line;
	std::string name;
	std::string symbol;
	int trNumber;
	std::vector<double> params;
	std::string trStr;  // 拡張 surface TR文字列
	std::map<std::string, std::string> paramMap;  // paramName=paramVaueのような入力を格納するマップ

	std::string pos() const {return file + ":" + std::to_string(line);}
	std::string toString() const;
	std::unique_ptr<math::Matrix<4>> getMatrixPtr(const std::unordered_map<size_t, math::Matrix<4>> &trMap) const;
};


} // end namespace geom
#endif // SURFACECARD_HPP
