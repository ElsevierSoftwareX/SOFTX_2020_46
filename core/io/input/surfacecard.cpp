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
#include "surfacecard.hpp"

#include <sstream>


#include "core/utils/string_utils.hpp"
#include "core/utils/container_utils.hpp"
#include "core/utils/message.hpp"
#include "core/formula/fortran/fortnode.hpp"
#include "cardcommon.hpp"
#include "ijmr.hpp"

namespace {
// NOTE このregexはcellと共通なのでcardCommonへ移すべし。
std::regex cellParamPattern(R"(([\w:*]+) *= *(\( *[-+*/%\w\d .{},]+ *\)|[-+\w\d.]+))");
}




// TRSFは複数回書いて良い。 TRSF=(10 0 0) TRSF=(0 10 0) とすると左側から順に適用される
inp::SurfaceCard inp::SurfaceCard::fromString(const std::string &cardStr,
											  bool checkValidUserInput,
											  bool disableFortran,
											  bool disableIjmr)
{
	/*
	 * expandIjmrとfort::eqは結構じかんが掛かる。
	 * 一方格子要素等で自動生成した面のようにfortran数式やijmr表現が含まれないことが保証されている
	 * 場合、それらをオフにできるようにしたい。
	 */
	std::string newCardStr = cardStr;

	if(!disableIjmr) {
		// サーフェイスカードはサーフェイス名を除いてijmr展開する。
		std::string firstElement = inp::cutFirstElement(" ", &newCardStr);
		// TODO phits警告
		inp::ijmr::expandIjmrExpression(" ", &newCardStr);
		if(inp::ijmr::checkIjmrExpression(firstElement)) {
			throw std::invalid_argument("Surface name should not be ijmr-expression, str=" + firstElement);
		}
		newCardStr = firstElement + " " + newCardStr;
	}

	std::unordered_map<char, char> qmarks{{'"', '"'}, {'{', '}'}, {'(', ')'}};

	auto inputs = utils::splitString(qmarks, " ", newCardStr, true);

	// NOTE 面倒なことにphitsでは[]を{}と同様に数式用引用符として使っている例がマニュアルの中にある。ので置換する。
	// だがしかしlattice要素のセル名に[]は使うのでその部分は置換してはならない
	assert(!inputs.empty());
	for(size_t i = 1; i < inputs.size(); ++i) {
		std::replace(inputs.at(i).begin(), inputs.at(i).end(), '[', '{');
		std::replace(inputs.at(i).begin(), inputs.at(i).end(), ']', '}');
	}


	// 最初にTrパラメータを読み込む
	std::string trStr;

//	mDebug() << "scardstr===" << newCardStr;
//	mDebug() << "params===" << inputs;

	// 引用符はsplit後には意味を持たないので除去対象とする。但し{}はfortran式の指示で意味を持つので除去しない
	//std::unordered_map<char, char> unnecessaryMarks{{'"', '"'}, {'(', ')'}};
	//std::unordered_map<char, char> unnecessaryMarks{{'"', '"'}};
	for(auto it = inputs.begin(); it != inputs.end();) {
		if(appendCanonicalTrStr(*it, &trStr)) {
			it = inputs.erase(it);
		} else {
			*it = utils::dequote('\"', *it);

			++it;
		}
	}
	// 次に汎用パラメータを読み込む
	std::map<std::string, std::string> otherParamMap;
	std::smatch paramMatch;
	for(auto it = inputs.begin(); it != inputs.end();) {
		if(std::regex_search(*it, paramMatch, cellParamPattern)) {
			otherParamMap[paramMatch.str(1)] = paramMatch.str(2);
			it = inputs.erase(it);
		} else {
			++it;
		}
	}

	// POLYはパラメータがstl=しか無く数値パラメータがないので inputs.sizeが2になることはある。
//	if(inputs.size() <= 2) {
	if(inputs.size() <= 1) {
		std::stringstream ss;
		ss << " Too short surface card \"" << cardStr << "\"";
		throw std::invalid_argument(ss.str());
	}

	std::string name = inputs.at(0);
	std::string symbol;
	int trNumber = SurfaceCard::NO_TR;
	size_t paramStartIndex;
	std::vector<double> paramValues;
	// symbol名は2番めのパラメータ(TRなし)3番めのパラメータ(2番めが数字の場合)
	// 2番目のパラメータが整数でなければそれがsymbol, 整数ならならTRありで3番目がTR
	if(!utils::isInteger(inputs.at(1))){
		symbol = inputs.at(1);
		paramStartIndex = 2;
	} else {
		trNumber = utils::stringTo<int>(inputs.at(1));
		symbol = inputs.at(2);
		paramStartIndex = 3;
	}

	for(size_t i = paramStartIndex; i < inputs.size(); ++i) {
		if(!disableFortran) {
			paramValues.emplace_back(fort::eq(inputs.at(i)));
		} else {
			paramValues.emplace_back(utils::stringTo<double>(inputs.at(i)));
		}
	}


	name = utils::canonicalName(name);
    checkNameCharacters(name, checkValidUserInput);
	return SurfaceCard(name, symbol, trNumber, paramValues, trStr, otherParamMap);
}

std::string inp::SurfaceCard::toString() const
{
	std::stringstream ss;
	ss << "name=" << name <<", symbol=" << symbol << ", TRnum=" << trNumber << ", TRSFstr=" << trStr << ", params=";
	for(auto &param: params) {
		ss << param << " ";
	}
	return ss.str();
}

std::unique_ptr<math::Matrix<4>>
	inp::SurfaceCard::getMatrixPtr(const std::unordered_map<size_t, math::Matrix<4>> &trMap) const
{
	// TRマップの整合性チェックをしておかないとout_of_range発生
	std::unique_ptr<math::Matrix<4>> matrix;
	if(trNumber != inp::SurfaceCard::NO_TR) {
		auto it = trMap.find(trNumber);
		if(it != trMap.end()) {
			matrix.reset(new math::Matrix<4>(trMap.at(trNumber)));
		} else {
			throw std::out_of_range("TR number = " + std::to_string(trNumber) + " used but not defined");
		}
	}
	return matrix;
}
