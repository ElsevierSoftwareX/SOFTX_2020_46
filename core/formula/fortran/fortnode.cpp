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
#include "fortnode.hpp"

#include <cfenv>
#include <cmath>
#include <functional>
#include <limits>
#include <regex>
#include <unordered_map>

#include "core/formula/formula_utils.hpp"
#include "core/math/constants.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"


namespace {
// fortran演算子。優先度は
//１：カッコの中　→　２：べき乗　→　３：乗除算　→　４：加減算
// 単項演算は左側に0を置いた二項演算として扱う
constexpr char OP_POW[] = "**";
constexpr char OP_MUL[] = "*";
constexpr char OP_DIV[] = "/";
constexpr char OP_ADD[] = "+";
constexpr char OP_SUB[] = "-";

const std::regex &powPattern()
{
	// (演算子以外の任意の文字)＊＊(演算子以外の任意の文字)
	static std::regex pat(R"(([^-+*/]+)(\*\*)([^-+*/]+))");
	return pat;
}
const std::regex &mulPattern()
{
	static std::regex pat(R"(([^-+*/]+)(\*)([^-+*/]+))");
	return pat;
}
const std::regex &divPattern()
{
	static std::regex pat(R"(([^-+*/]+)(/)([^-+*/]+))");
	return pat;
}
const std::regex &addPattern()
{
	static std::regex pat(R"(([^-+*/]+)(\+)([^-+*/]+))");
	return pat;
}
const std::regex &subPattern()
{
	static std::regex pat(R"(([^-+*/]+)(-)([^-+*/]+))");
	return pat;
}
const std::regex &bFuncPattern()
{
	// +-sin 等符号はヒットしないようにする。＋ーは単項演算として処理するので
	static std::regex pat("(\b|^)(exp|log10|log|sqrt|asin|sinh|sin|acos|cosh|cos|atan2|atan|tanh|tan|abs|min|max|mod|sign|nint|int)");
	return pat;
}

const std::regex &listPattern() {
	static std::regex lPattern(R"(^.*,.*$)");
	return lPattern;
}

const std::regex &expFloatPattern() {
	static std::regex expFloatPat(R"(\d*\.*\d*[eE][-+]+\d+)");
	return expFloatPat;
}

std::vector<std::regex> &operatorPatterns() {

	static std::vector<std::regex> opPatterns{addPattern(), subPattern(), mulPattern(), divPattern(), powPattern()};
	return opPatterns;
}

// 引数が可変長の組み込み関数の値を計算する。
double calcVariadicFunction(const std::string &funcName, const std::vector<double> &args)
{
	// min, max
	static std::unordered_map<std::string, std::function<double(const std::vector<double>&)>> opMap
	{
	{"min", [](const std::vector<double> &dvec) {return *std::min_element(dvec.cbegin(), dvec.cend());}},
	{"max", [](const std::vector<double> &dvec) {return *std::max_element(dvec.cbegin(), dvec.cend());}}
    };
	return opMap.at(funcName)(args);
}

double calcTwoVariablesFunction(const std::string &funcName, double d1, double d2)
{
	static std::unordered_map<std::string, std::function<double(double, double)>> opMap
	{
	{"atan2", [](double d1, double d2){return std::atan2(d1, d2);}},
	{"mod", [](double d1, double d2){return std::fmod(d1, d2);}},
	{"sign", [](double d1, double d2){return (d2 < 0) ? -std::abs(d1): std::abs(d1);}}, // 第 1 引数の絶対値に第 2 引数の符号を掛ける
	{"min", [](double d1, double d2){return std::min(d1, d2);}}, // min/maxも引数が二個だとここへ来てしまう。
	{"max", [](double d1, double d2){return std::max(d1, d2);}},
	};
	return opMap.at(funcName)(d1, d2);
}

double calcSingleArgumentFunction(const std::string &funcName, double value)
{
	static std::unordered_map<std::string, std::function<double(double)>> opMap
	{
	{"exp",   [](double d1){return std::exp(d1);}},
	{"log10", [](double d1){if(d1 < 0) {
			                    throw std::domain_error(std::string("Negative arg for log10, arg=") +std::to_string(d1));
		                    } else {
			                    return std::log10(d1);
		                    }}},
	{"log",   [](double d1){if(d1 < 0) {
			                    throw std::domain_error(std::string("Negative argt for log, arg=") +std::to_string(d1));
		                    } else {
			                    return std::log(d1);
		                    }}},
	{"sqrt",  [](double d1){if(d1 < 0) {
			                    throw std::domain_error(std::string("Negative arg for sqrt, arg=") + std::to_string(d1));
		                    } else {
			                    return std::sqrt(d1);
		                    }}},
	{"asin",  [](double d1){if(std::abs(d1) > 1) {
			                    throw std::domain_error(std::string("|arg| > 1 for asin, arg=") + std::to_string(d1));
		                    } else {
			                    return std::asin(d1);
		                    }}},
	{"sinh",  [](double d1){double retval = std::sinh(d1);
		                    if (std::fetestexcept(FE_OVERFLOW)) {
								throw std::overflow_error(std::string("overflow by sinh, arg=") + std::to_string(d1));
							} else {
								return retval;
							}}},
	{"sin",   [](double d1){return std::sin(d1);}},
	{"acos",  [](double d1){if(std::abs(d1) > 1) {
			                    throw std::domain_error(std::string("|arg| > 1 for acos, arg=") + std::to_string(d1));
		                    } else {
			                    return std::acos(d1);
		                    }}},
	{"cosh",  [](double d1){double retval = std::cosh(d1);
		                    if (std::fetestexcept(FE_OVERFLOW)) {
								throw std::overflow_error(std::string("overflow by cosh, arg=") + std::to_string(d1));
							} else {
								return retval;
							}}},
	{"cos",   [](double d1){return std::cos(d1);}},
	{"atan",  [](double d1){return std::atan(d1);}},
	{"tanh",  [](double d1){return std::tanh(d1);}},
	{"tan",   [](double d1){return std::tan(d1);}},
	{"abs",   [](double d1){return std::abs(d1);}},
	{"float", [](double d1){return d1;}},
	{"int",   [](double d1){return static_cast<double>(static_cast<int>(d1));}},  // 切り捨て
	{"nint",  [](double d1){return std::round(d1);}},  // 四捨五入
    };
	return opMap.at(funcName)(value);
}

double calcBinaryOperation (const std::string &opStr, double leftValue, double rightValue)
{
	static std::unordered_map<std::string, std::function<double(double, double)>> opMap
	{
	{"+", [](double d1, double d2) {return d1+d2;}},
	{"-", [](double d1, double d2) {return d1-d2;}},
	{"*", [](double d1, double d2) {return d1*d2;}},
	{"/", [](double d1, double d2) {return d1/d2;}},
	{"**", [](double d1, double d2) {return std::pow(d1, d2);}},
    };

	return opMap.at(opStr)(leftValue, rightValue);
}

}  // end anonymous namespace





//


/*
 * 最も優先度の低い演算子でstrを分割し、
 * 左右の子ノードを作成する。
 *
 */
fort::Node::Node(const std::string &str)
{
	const char REP_CHAR = '@'; //
	std::string eqstr = str;
	utils::tolower(&eqstr);
	utils::removeRedundantBracket('(', ')', &eqstr, true);
	// とりあえず空白は削除
	auto it = std::remove_if(eqstr.begin(), eqstr.end(), [](char ch){return ch == ' ';});
	if(it != eqstr.end()) eqstr.erase(it, eqstr.end());

	// 以降では基本となる文字列のサイズを変えない(posが無効になるから)
	const std::string refString = eqstr;

	/*
	 * 先に組み込み関数を処理したいが多重括弧問題は単なるregex_searchでは解決できない…
	 * ので先に括弧を置換処理してしまって、組み込み関数置換では関数名部分のみを対象とする。
	 */

	// 1.演算子では最高優先度の括弧と括弧内が以降での演算子検索にヒットしないように置換
	//std::vector<std::pair<std::string::size_type, int>> parPositions;
	std::pair<std::string::size_type, std::string::size_type> posPair;
	while(posPair = formula::findOutmostParenthesis(eqstr), posPair.first != std::string::npos) {
		eqstr.replace(posPair.first, posPair.second - posPair.first + 1, posPair.second - posPair.first + 1, REP_CHAR);
		//parPositions.emplace_back(posPair.first, posPair.second - posPair.first + 1);
	}


	/*
	 * min/maxのように二個以上の引数を取る場合はleft, rightで処理しきれないので
	 * カンマで区切った部分は一まとまりにして分離せず、値を計算する時に分離する。
	 * 故にツリー構築時は演算子検索にヒットしないように置換する。
	 */
	std::smatch sm;
	// ここの段階で既に()内は置換されているので","が出てきた場合全体を1つのリストとみなして全体を置換する。
	//std::regex listPattern(R"(^.*,.*$)");
	while(std::regex_search(eqstr, sm, listPattern())) {
		std::string::size_type pos = std::distance(eqstr.cbegin(), sm[0].first);
		eqstr.replace(pos, sm.str().size(), sm.str().size(), REP_CHAR);
	}

	// 指数表記も+-演算子にマッチされたくないので該当部は置換する。
	//std::regex expFloatPattern(R"(\d*\.*\d*[eE][-+]+\d+)");
	if(std::regex_search(eqstr, sm, expFloatPattern())) {
		//mDebug() << "sm=" << sm.str();
		std::string::size_type pos = std::distance(eqstr.cbegin(), sm[0].first);
		eqstr.replace(pos, sm.str().size(), sm.str().size(), REP_CHAR);
	}



	// 括弧と組み込み関数がなければ、あとは演算子の優先順位は固定なので優先されない方から検索すれば良い。
	// この時eqstrではなく@置換前の文字列に戻して分割する
//	mDebug() << "refString=" << refString;
//	mDebug() << "eqString =" << eqstr;
	std::string::size_type opPos = std::string::npos;
	//std::vector<std::regex> operatorPatterns{addPattern(), subPattern(), mulPattern(), divPattern(), powPattern()};
	// 単なるstd::string::findだと乗算がべき乗にヒットしてしまう。
	for(auto &opPat: operatorPatterns()) {
		if(std::regex_search(eqstr, sm, opPat)) {
			expression_ = sm.str(2);
			opPos = std::distance(eqstr.cbegin(), sm[2].first);
			break;
		}
	}



	/*
	 * -sin(45.0)のような単項組み込みの場合はどうするんだっけ？
	 *
	 * ・とりあえずexpression_に符号を含める。→ だめ。組み込み関数マップではsin, cosのように符号なし関数が登録してある
	 * まっとうにやるなら-sin(45)は 単項マイナス → 組み込みsin →引数 のように単項演算ノード2個で表現すべし。
	 * アドホックに解決するなら組み込み関数マップに+-符号なしの3通りを登録する。
	 *
	 * ・まっとうに単項ノード2個で解決する。
	 *
	 *
	 */


	if(opPos == std::string::npos) {
		//mDebug() << "二項演算子が文字列===" << eqstr << "に見つからない! よってこの文字列は........";

		// 二項演算子が見つからない場合、単項演算か、末端数値ノードかのどちらか。
		if(std::regex_search(refString, sm, bFuncPattern())) {
			// 組み込み関数単項演算の可能性をチェック。演算子組み込み関数部分を探索し、該当すればexpressionに代入する。
//			for(size_t i = 0; i < sm.size(); ++i) {
//				mDebug() << "i===" << i << "str=" << sm.str(i);
//			}

			auto pos = std::distance(refString.cbegin(), sm[0].first);
			expression_ = sm.str();
			//mDebug() << "単項組み込み関数ノード！expression=" << expression_ << ",left=" << refString.substr(pos+expression_.size());
			left_.reset(new Node(refString.substr(pos+expression_.size())));
		} else if(eqstr.front() == '-' || eqstr.front() == '+') {
			// 単項演算は+か-。いずれもleftを0にした二項演算として扱う
			expression_ = eqstr.substr(0,1);
			//mDebug() << "±単項演算ノード！expression = " << expression_ << ", left= 0, right=" << refString.substr(1);
			left_.reset(new Node("0"));
			right_.reset(new Node(refString.substr(1)));
		} else {
			// ノード末端の場合
			expression_ = refString;
			//mDebug() << "ノード末端発見node end. =" << refString;
			return;
		}
	} else {

		// 二項演算発見の場合
//		mDebug() << "expression = " << expression_ << ", left=" << refString.substr(0, opPos)
//		         << ", right=" << refString.substr(opPos+expression_.size());
		left_.reset(new Node(refString.substr(0, opPos)));
		right_.reset(new Node(refString.substr(opPos+expression_.size())));
		return;
	}

}



// NOTE calculate(string, double)に統合予定
// 値を計算する。末端ノードは数値なので文字列expression_をdouble化して返し、それ以外はepxression_に応じた演算を行う。
double fort::Node::calculate() const
{
	if(!left_ && !right_) {
		// 右にも左にも枝が無いのは末端なのでexpression_は数値であるはず。なのでdouble化して返す。
		// piは引数を取らないので組み込み関数では扱わずここで数値に変換して処理する。
		return (expression_ != "pi") ? utils::stringTo<double>(expression_) : math::PI;
	} else if (left_ && !right_) {
		// 左だけに枝を持つのは単項演算ノード。単項演算は引数可変長組み込み関数があることに注意
		if(left_->expression().find(",") != std::string::npos) {
			auto nodeStrings = utils::splitString(",", left_->expression(), true);
			std::vector<double> nodeResults;
			//mDebug() << "nodestrings=" << nodeStrings;
			for(auto nodeStr: nodeStrings) {
				nodeResults.emplace_back(Node(nodeStr).calculate());
			}
			// 組み込みは二変数までが大半なので別にする。
			if(nodeResults.size() == 2) {
				return calcTwoVariablesFunction(expression_, nodeResults.at(0), nodeResults.at(1));
			} else {
				return calcVariadicFunction(expression_, nodeResults);
			}
		} else {
			return calcSingleArgumentFunction(expression_, left_->calculate());
		}
	} else {
		return calcBinaryOperation(expression_, left_->calculate(), right_->calculate());
	}
}

// 値を計算する。この時末端ノードの文字列expression_に含まれるargStrはvalueに置換してからcalculate()相当を実行する。
double fort::Node::calculate(const std::string &argStr, double value) const
{
	if(!left_ && !right_) {
		std::string tmpStr = expression_;
		// 右にも左にも枝が無いのは末端なのでexpression_は数値であるはずなのでdouble化して返す。
		if(!argStr.empty()) {
			// argStrの前は文頭、演算子、左括弧の何れかで後ろは行末演算子右括弧の何れか
			std::string regexStr = "([-+*/%(]|^)(" + argStr + ")([-+*/%)]|$)";
			std::regex argStrPattern(regexStr);
			std::smatch sm;
			while(std::regex_search(tmpStr, sm, argStrPattern)) {
				tmpStr = std::string(tmpStr.cbegin(), sm[2].first)
				        + utils::toString(value, std::numeric_limits<double>::digits10)
						+ std::string(sm[2].second, tmpStr.cend());
			}
		}
		return utils::stringTo<double>(tmpStr);
	} else if (left_ && !right_) {
		// 左だけに枝を持つのは単項演算ノード。単項演算は引数可変長組み込み関数があることに注意
		if(left_->expression().find(",") != std::string::npos) {
			// expressionにコンマがある場合2変数以上の組み込み関数なので文字列を引数ごとに分割する。
			auto nodeStrings = utils::splitString(",", left_->expression(), true);
			std::vector<double> nodeResults;
			//mDebug() << "nodestrings=" << nodeStrings;
			for(auto nodeStr: nodeStrings) {
				if(!argStr.empty()) {
					nodeResults.emplace_back(Node(nodeStr).calculate(argStr, value));
				} else {
					nodeResults.emplace_back(Node(nodeStr).calculate());
				}
			}
			// 組み込みは二変数までが大半なので別にする。ここでのexpression_は関数/演算なのでargStrはチェックしない
			if(nodeResults.size() == 2) {
				// 2変数組み込み関数
				return calcTwoVariablesFunction(expression_, nodeResults.at(0), nodeResults.at(1));
			} else {
				// 可変引数組み込み関数
				return calcVariadicFunction(expression_, nodeResults);
			}
		} else {
			// 単項演算
			if(!argStr.empty()) {
				return calcSingleArgumentFunction(expression_, left_->calculate(argStr, value));
			} else {
				return calcSingleArgumentFunction(expression_, left_->calculate());
			}
		}
	} else {
		// 左右に枝を持つ場合は二項演算ノード
		if(!argStr.empty()) {
			return calcBinaryOperation(expression_, left_->calculate(argStr, value), right_->calculate(argStr, value));
		} else {
			return calcBinaryOperation(expression_, left_->calculate(), right_->calculate());
		}
	}
}



double fort::eq(std::string str)
{
	utils::trim(&str);
	while(str.front() == '{' && str.back() == '}') {
		str = str.substr(1, str.size()-2);
		utils::trim(&str);
	}

	return Node(str).calculate();
}
