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
#include "phitsdistribution.hpp"

#include <cassert>
#include <regex>

#include "core/formula/fortran/fortnode.hpp"
#include "core/io/input/phits/phits_metacards.hpp"
#include "core/utils/message.hpp"
#include "core/utils/numeric_utils.hpp"


// phitsの入力から最低限requiredeParamNamesを読み取る。
std::unordered_map<std::string, std::string>
src::phits::readParamsMap(std::vector<std::string> requiredParamNames,
						  std::vector<std::string> optionalParamNames,
						  const std::list<inp::DataLine> &sourceInput,
						  std::list<inp::DataLine>::const_iterator &it)
{
	assert(!requiredParamNames.empty());
	std::unordered_map<std::string, std::string> paramsMap;
	for(size_t i = 0; i < requiredParamNames.size(); ++i) {  // 必須パラメータの行数だけ読む
		if(it == sourceInput.end()) {
			throw std::invalid_argument("Unexpected EOF while reading user-defined energy distribution");
		}
		paramsMap.emplace(inp::phits::getParameterPair(it->data));
		++it;
	}
	// 4行入力の場合があるのでとりあえず次の行を読んで、e-type5入力ならiteratorを進める
	std::string regexStr = utils::concat(requiredParamNames, "|");
	if(!optionalParamNames.empty()) regexStr = regexStr +  "|" + utils::concat(optionalParamNames, "|");
	// ここでf(x)のようにパラメータ名に括弧が含まれる場合エスケープする。
	std::string::size_type pos = 0;
	while(pos = regexStr.find_first_of("()", pos), pos != std::string::npos) {
		regexStr = regexStr.substr(0, pos) + "\\" + regexStr.substr(pos);
		pos = pos+2;
	}
	std::regex paramNamePattern(regexStr);

	for(size_t j = 0; j < optionalParamNames.size(); ++j) {
		try{
			auto paramPair = inp::phits::getParameterPair(it->data);  // ここで例外の可能性あり
			// e-type=5のパラメータならiteratorを進める。そうでなければそこでe-type5分布入力終了なので進めない
			//std::regex etype5ParamNamePattern(R"(nm|eg1|eg2|f(x))");
			if(std::regex_search(paramPair.first, paramNamePattern)) {
				paramsMap[paramPair.first] = paramPair.second;
				++it;
			} else {
				// 次の行に目標としないパラメータが存在した場合はbreakして読み取り終了
				break;
			}
		} catch(std::exception &e) {
			(void)e;
			// 次の行はパラメータ行とは限らない。E分布サブセクションの最後で次の行は別のセクションかもしれない。
			// その場合は例外発生する可能性があるので、ここでは無視但しパラメータ読み取りは終了
			break;
		}
	}
	// 必須パラメータを満足しているかチェック
	for(auto &pname: requiredParamNames) {
		if(paramsMap.find(pname) == paramsMap.end()) {
			throw std::invalid_argument("parameter = " + pname + " is required but not found");
		}
	}
	return paramsMap;
}


// func の xの部分にepointに格納されたE値を代入してepoints[i], epoints[i+1]間の確率を格納したvectorを返す。
std::vector<double> src::phits::getProbabilityGroup(const std::vector<double> &epoints, const std::string &funcStr)
{
	fort::Node func(funcStr);
	std::vector<double> pdfs, pvec;
	pdfs.reserve(epoints.size());
	pvec.reserve(epoints.size()-1);
	for(size_t i = 0; i < epoints.size(); ++i) {
		//mDebug() << epoints.at(i) << func.calculate("x", epoints.at(i));
		pdfs.emplace_back(func.calculate("x", epoints.at(i)));
	}
	double total = 0;
	for(size_t i = 0; i < epoints.size()-1; ++i) {
		double pdf = 0.5*(pdfs.at(i)+pdfs.at(i+1));
		total += pdf*(epoints.at(i+1) - epoints.at(i));
		pvec.emplace_back(pdf);
	}
		// 規格化しないとMultiGroupDistributionコンストラクタで警告が出る。第二引数はpdf！
	if(total <= 0) {
		throw std::invalid_argument("Total probability for energy distribution should be positive nonzero. total="
									+ std::to_string(total));
	} else {
		for(auto &p: pvec) p /= total;
	}
	return pvec;
}


std::vector<double> src::phits::getPoints(double emin, double emax, int numPoints)
{
	if(emin < 0 || emax < 0) {
		throw std::invalid_argument("emin/emax should be positive.");
	} else if (emax < emin) {
		throw std::invalid_argument("emax should be > emin");
	} else if (utils::isSameDouble(std::abs(numPoints), 1.0) && utils::isSameDouble(emin, emax)) {
		throw std::invalid_argument("|nm| should be greater than 1 when eg1 != eg2");
	}
	std::vector<double> epoints{emin};
	epoints.reserve(std::abs(numPoints));
	if(numPoints > 0) {
		double dE = (emax - emin)/(numPoints-1);
		for(int i = 1; i < numPoints; ++i) {
			epoints.emplace_back(epoints.at(i-1) + dE);
		}
	} else if (numPoints < 0) {
		numPoints *= -1;
		double dE = std::exp((std::log(emax) - std::log(emin))/(numPoints - 1));
		for(int i = 1; i < numPoints; ++i) {
			epoints.emplace_back(epoints.at(i-1)*dE);
		}
	}
	assert(utils::isSameDouble(epoints.front(), emin));
	assert(utils::isSameDouble(epoints.back(), emax));
	return epoints;
}
