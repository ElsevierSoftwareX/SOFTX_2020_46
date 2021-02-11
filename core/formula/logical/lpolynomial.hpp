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
#ifndef LPOLYNOMIAL_HPP
#define LPOLYNOMIAL_HPP

#include <algorithm>
#include <cassert>
#include <regex>
#include <stdexcept>
#include <set>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <vector>


#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "core/formula/formula_utils.hpp"



namespace lg {


/*
 * FactorTypeが満たすべき条件
 * ・演算子 <, ==が定義済みであること
 * ・FactorType complimentedFactor(const FactorType&)が定義済みであること
 *    この条件を満たすには符号付き整数かstringのみ。
 */
template <class FactorType>
class LogicalExpression
{
	enum class KIND{MONOMIAL, NESTEDMONO, POLYNOMIAL};

	template <class FType, class FunctorType, class FunctorArgType>
	friend bool evaluateL(const LogicalExpression<FType> &polynomial, const FunctorType &functor, const FunctorArgType &arg);

	// terms_のサイズが1なら中身をfactors_へ移す。
	void initialize()
	{
		/*
		 * terms_のサイズが1でもfactorsが存在するとは限らない。
		 * 単項の入れ子になってterms.front().termsが存在する場合がある。
		 * なので単項入れ子を解消するまで要素を移していく
		 */
		while(terms_.size() == 1) {
			factors_ = terms_.front().factors_;
			terms_ = terms_.front().terms_;
		}
		// FactorTypeが非数値型(文字列を想定)の場合、factors_の要素の中に演算子文字が含まれていてはならない。
	}
	// set<FactorType>用のFactor比較関数
	class FactorComparator
	{
	public:
        bool operator()(const FactorType &f1, const FactorType &f2) const
		{
			int v1 = utils::toInt<FactorType>(f1);
			int v2 = utils::toInt<FactorType>(f2);
			return v1*v1 < v2*v2;
//			return (v1*v1 == v2*v2) ? v1 < v2 : v1*v1 < v2*v2;
		}
	};
public:
	typedef FactorType factor_type;
	static const char OP_ADD;
	static const char OP_MUL;
    LogicalExpression(){}
	//! 因子から単項単因子式生成
	explicit LogicalExpression(const FactorType &factor):factors_({factor}) {initialize();}
	//! 因子ベクトルから単項式生成
	explicit LogicalExpression(const std::vector<FactorType> &factors):factors_(factors) {initialize();}
	//! 多項式のコピー生成. terms_のサイズが１なら中身はfactors_へ移す
	explicit LogicalExpression(const std::vector<LogicalExpression<FactorType>> &terms):terms_(terms) {initialize();}
	//! vectorのムーブによるコンストラクタ
	explicit LogicalExpression(std::vector<LogicalExpression<FactorType>> &&terms):terms_(terms) {initialize();}
	// アクセサ
	const auto& factors() const {return factors_;}
	const auto& factorPolys() const{return factorPolys_;}
	const auto& terms() const {return terms_;}

	bool empty() const {return terms_.empty() && factors_.empty() && factorPolys_.empty();}




	//! 項数。入れ子の箇所は1個にまとめてカウントする。
	std::size_t size() const
	{
		if(factors_.empty()) {
			return terms_.empty() ? factorPolys_.size() : terms_.size();
		} else {
			return 1;
		}
	}
	//! 重複のない使用されている因子のsetを返す。表面のみを返す。
	std::set<FactorType, FactorComparator> uniqueFactorSet() const
	{
		assert(factors_.empty() || factorPolys_.empty() || terms_.empty());
		std::set<FactorType, FactorComparator> retval;
		if(empty()) {
			return retval;
		} else if(!factors_.empty()) {
			return factorSet();
		} else if (!factorPolys_.empty()) {
			for(const auto &poly: factorPolys_) {
				const std::set<FactorType, FactorComparator> tmpSet = poly.uniqueFactorSet();
				retval.insert(tmpSet.cbegin(), tmpSet.cend());
			}
		} else {
			for(const auto &term: terms_) {
				const std::set<FactorType, FactorComparator> tmpSet = term.uniqueFactorSet();
				retval.insert(tmpSet.cbegin(), tmpSet.cend());
			}
		}
		return retval;

	}
	//! 重複のない使用されている因子のvectorを返す(表面のみ)
	std::vector<FactorType> uniqueFactors() const
	{
		const std::set<FactorType, FactorComparator> tmpSet = uniqueFactorSet();
		return std::vector<FactorType>(tmpSet.cbegin(), tmpSet.cend());
	}

	// ###################### 文字列化
	// 文字列に変換。この時の出力はsurface名で、convMapを逆引きしてsurface名に戻す。
	// convMapには裏面も登録しておかないとcomplimentが機能しない
	//! 文字列に変換する。文字列変換時に因子→文字列変換マップを使う。
	std::string toString(const std::unordered_map<std::string, FactorType> &convMap) const
	{
		assert(factors_.empty() || factorPolys_.empty() || terms_.empty());
		std::stringstream ss;

		if(!factors_.empty()) {
			//if(factors_.size() != 1) ss << "(";  // 積で連結している部分は括弧でくくる必要がない。
			for(size_t i = 0 ; i < factors_.size(); ++i) {
				// convMapを逆引きする。効率は悪いが同じマップでevaluateする時に正引きにする以上ここでは逆になる。
				auto it = convMap.cbegin();
				for(; it != convMap.cend(); ++it) {
					if(it->second == factors_.at(i)) break;
				}
				if(it == convMap.end()) {
					if(convMap.empty()) {
						// 空のconvMap適用時はostream& operator<< で変換する。
						ss << factors_.at(i);
					} else {
						throw std::runtime_error("factor \"" + utils::toString<FactorType>(factors_.at(i)) + "\" not found in convMap");
					}

				} else {
					ss << it->first;
				}
				if(i != factors_.size()-1) ss << OP_MUL;
			}
			//if(factors_.size() != 1) ss << ")";
		} else if(!factorPolys_.empty()) {
			//if(factorPolys_.size() != 1) ss << "(";
			for(size_t i = 0; i < factorPolys_.size(); ++i) {
				ss << factorPolys_.at(i).toString(convMap);
				if(i != factorPolys_.size()-1) ss << OP_MUL;
			}
			//if(factorPolys_.size() != 1)ss << ")";
		} else {
			if(terms_.size() != 1) ss << "(";
			for(size_t i = 0 ; i < terms_.size(); ++i) {
//				ss <<"[" << terms_.at(i).toString(convMap) <<"]";
				ss << terms_.at(i).toString(convMap);
				if(i != terms_.size()-1) ss << OP_ADD;
			}
			if(terms_.size() != 1) ss << ")";
		}

		return ss.str();
	}
	std::string toString() const {return toString(std::unordered_map<std::string, FactorType>());}

	//! 文字列からの生成関数.
	// 文字列にはsurfaceコンプリメントが入っている場合を考慮すること。
	static LogicalExpression<FactorType> fromString(std::string equation,
	                              const std::unordered_map<std::string, FactorType> &convMap)
	{

//		static int n = 0;
//		++n;
//		mDebug() << "n===" << n << "多項式を文字列から生成。string===" << equation;
//		assert(n < 1000);


		const std::string OP_CHARS{OP_ADD, OP_MUL};
		// 冗長な空白と一番外側の冗長な括弧は取る。
		utils::removeRedundantBracket('(', ')', &equation, true);
		// ソース文字列へのポインタ、補う文字、左側に許容される文字、右側に許容される文字
		formula::fixOmittedOperator(&equation, ' ', OP_CHARS + "(#", OP_CHARS + ")");
		formula::checkValidEquation(equation, OP_CHARS);

		// 面コンプリメントがある場合先にその部分の論理式を構築して.complimented.toString()結果で該当部分を置換する。
		std::smatch sm;
                // regex recursionはC++では使えないので。正規表現のみでの一発処理は不可
		// LogicalExpression::fromString内で再帰的に実行
                while(std::regex_search(equation, sm, std::regex(R"(# *(\())"))) {
                    // ()#の位置
                    std::string::size_type lparPos = static_cast<size_t>(std::distance(equation.cbegin(), sm[1].first));
                    std::string::size_type rparPos = utils::findMatchedBracket(equation, lparPos, '(', ')');
                    std::string::size_type sharpPos = static_cast<size_t>(std::distance(equation.cbegin(), sm[0].first));
                    auto compEq = LogicalExpression<std::string>::fromString(equation.substr(lparPos+1, rparPos-lparPos-1));
                    std::string complimentedStr = "(" + compEq.complimented().toString() + ")";
                    equation.replace(sharpPos, rparPos-sharpPos+1, complimentedStr);
                }

		/*
		 * 文字列から多項式を生成する場合、
		 * まず文字列を各項ごとに切り出す。
		 *
		 * まず括弧内に入らない一番外側の論理和演算記号で分割すれば
		 * それぞれが項(Term)になる。項内の多項式(polynominal)はTermクラスの方で生成する。
		 */
		// 和演算は積よりも優先度が低いので括弧外の和演算子の区切りは必ず項の区切りになる。
		std::vector<std::string> termStrings = formula::splitOutmost(equation, '(', ')', OP_ADD, false);
		assert(!termStrings.empty());



		if(termStrings.size() == 1) {
			// 単項(入れ子の多項式を含む可能性はある)
			std::string eqstr = termStrings.front();
			utils::removeRedundantBracket('(', ')', &eqstr, true); // 冗長な空白と括弧は取る。
			/*
			 * 最も外側の論理積演算子で区切れば各因子になる…ように思えるが
			 * 実際は "1 2:3"のように演算子の優先順位があるのでそうかんたんにはならない。
			 * 括弧の外に論理和演算子が存在しない場合のみ、積演算子を区切りにして因子を切り出す。
			 */
			std::vector<std::string> factorStrings;
			if(formula::splitOutmost(eqstr, '(', ')', OP_ADD, false).size() == 1) {  // 最外殻論理和で区切った結果が1成分vectorなら区切りなしを意味する。
				factorStrings = formula::splitOutmost(eqstr, '(', ')', OP_MUL, false);
			} else {
				factorStrings.emplace_back(eqstr);
			}
			/*
			 * (2)(3)のようなケースはここで分割されない
			 * 再帰的に呼んでも分割されずに無限ループになる。ゆえに事前チェック・修復が不可欠
			 */
			//mDebug() << "factors===" << factorStrings;

			// ここまででfactorStringsは項ごとに別れる。上の例では{"1 2", "3"}
			// 文字列が論理和演算文字を含んでいたら多項式なので再帰、論理積のみなら単項なので中身を構築
			std::vector<std::string> factors;
			std::vector<LogicalExpression<FactorType>> polynomials;

			for(auto factorStr: factorStrings) {

				// 論理和文字がなければfactorのみの文字列なので括弧を削除して、splitして因子に追加する
				if(formula::isOnlyFactors(factorStr, std::string{OP_ADD})) {
					// この関数の冒頭でfixしてるからここで省略された演算子を修復する必要はない。
					formula::checkValidEquation(factorStr, OP_CHARS);
					auto it = std::remove_if(factorStr.begin(), factorStr.end(), [](char ch){return ch == '('|| ch== ')';});
					if(it != factorStr.end()) factorStr.erase(it, factorStr.end());
					for(auto &fac: utils::splitString(std::string{OP_MUL}, factorStr, true)) {
						if(fac.front() == '"' && fac.back() == '"') fac = fac.substr(1, fac.size()-2);
						factors.emplace_back(fac);
					}
				} else {
				// polynominalを含む文字列の場合
					polynomials.emplace_back(LogicalExpression::fromString(factorStr, convMap));
				}
			}

			// 因子の名前は正準化しておく。
			for(auto &fac: factors) fac = utils::canonicalName(fac);

			// 最後にvector<string>のfactorsを vector<FactorType>に変換する。
			std::vector<FactorType> factorVector;
			for(auto factor: factors) {
				if(factor.front() == '"' && factor.back() == '"') factor = factor.substr(1, factor.size()-2); // quote削除
				if(factor.front() == '+') factor = factor.substr(1);// +記号は記号なし扱いとする。
				if(convMap.empty()) {
					// (意図的に)emptyのconvMapで生成する場合文字列のstoiは不適切な結果になるので
					// stringTo<>で変換する。
					factorVector.emplace_back(utils::stringTo<FactorType>(factor));
				} else if(convMap.find(factor) == convMap.end()) {
					// convMapに無いfactorが出た場合
					mDebug() << "lg::Term::fromString failed. string =" << equation;
					mDebug() << "factor=" << factor << "not found in convMap";
		//			for(auto &val: convMap) {
		//				mDebug() << val;
		//			}
					throw std::out_of_range(std::string("Surface \"") + factor + "\" is used but not defined.");
				} else {
					// 通常処理
					factorVector.emplace_back(convMap.at(factor));
				}
			}

			if(!polynomials.empty()) {
				// polynomialsが存在する場合
				LogicalExpression<FactorType> retPoly(polynomials.front());
				for(size_t i = 1; i < polynomials.size(); ++i) {
					retPoly *= polynomials.at(i);
				}
				if(!factorVector.empty()) retPoly*=LogicalExpression<FactorType>(factorVector);
				return retPoly;
			} else {
				// polynomialsが空で存在しない場合
				return LogicalExpression<FactorType>(factorVector);
			}
		} else {
			// ORで繋がれた多項式の場合
			std::vector<LogicalExpression<FactorType>> terms;
			for(auto &str: termStrings) {
				terms.emplace_back(LogicalExpression<FactorType>::fromString(str, convMap));
			}
			return LogicalExpression<FactorType>(terms);
		}
	}

	//! 文字列からの生成関数.
	static LogicalExpression fromString(const std::string &equation)
	{
		return fromString(equation, std::unordered_map<std::string, FactorType>());
	}

	//! コンプリメント
	LogicalExpression complimented() const
	{
		assert(factors_.empty() || factorPolys_.empty() || terms_.empty());
		if(empty()) {
			return *this;
		} else if(!factors_.empty()) {
			// ここのcomplimentedは因子型の補集合化をするtemplate関数
			LogicalExpression<FactorType> retPoly(utils::complimentedFactor(factors_.front()));
			for(size_t i = 1; i < factors_.size(); ++i) {
				retPoly += LogicalExpression<FactorType>(utils::complimentedFactor(factors_.at(i)));
			}
			return retPoly;
		} else if(!factorPolys_.empty()) {
			LogicalExpression<FactorType> retPoly(factorPolys_.front().complimented());
			for(size_t i = 1; i < factorPolys_.size(); ++i) {
				retPoly += factorPolys_.at(i).complimented();
			}
			return retPoly;
		} else  {
			LogicalExpression<FactorType> retPoly(terms_.front().complimented());
			for(size_t i = 1; i < terms_.size(); ++i) {
				retPoly *= terms_.at(i).complimented();
			}
			return retPoly;
		}
	}


	// ######################  演算子
	//! 加算複合代入。項を付け加える。内部的には単項式の場合多項式へ変更する。
	LogicalExpression<FactorType>& operator+=(const LogicalExpression<FactorType> &lp) {
		assert(factors_.empty() || factorPolys_.empty() || terms_.empty());
		assert(lp.factors_.empty() || lp.factorPolys_.empty() || lp.terms_.empty());
		// factor, factorPolysがある場合はtermsへ移してからtermsへlpをemplace_backする。
		if(!factors_.empty()) {
			terms_.emplace_back(factors_);
			factors_.clear();
		} else if(!factorPolys_.empty()) {
			// Poly(vector<Poly>)の場合vector要素間は和演算で繋げられるので、ここではoperator *を使う
			LogicalExpression<FactorType> tmpPoly(factorPolys_.front());
			for(size_t i = 1; i < factorPolys_.size(); ++i) {
				tmpPoly *= factorPolys_.at(i);
			}
			terms_.emplace_back(tmpPoly);
			factorPolys_.clear();
		}
		terms_.emplace_back(lp);
		return *this;
	}

	//! 加算。項を付け加えた多項式を返す。
	friend LogicalExpression<FactorType> operator+(LogicalExpression<FactorType> lhs, const LogicalExpression<FactorType> &rhs)
	{
		lhs += rhs;
		return lhs;
	}

	//! 乗算複合代入
	LogicalExpression<FactorType>& operator *=(const LogicalExpression &lp)
	{
		//mDebug() << "operator *= lhs===" << this->toString() << "rhs===" << lp.toString();
		assert(factors_.empty() || factorPolys_.empty() || terms_.empty());
		assert(lp.factors_.empty() || lp.factorPolys_.empty() || lp.terms_.empty());
		if(!factors_.empty()) {
			factorPolys_.emplace_back(factors_);
			factors_.clear();
		} else if(!terms_.empty()) {
			factorPolys_.emplace_back(terms_);
			terms_.clear();
		}
		factorPolys_.emplace_back(lp);
		return *this;
	}

	//! 乗算
	friend LogicalExpression<FactorType> operator*(LogicalExpression<FactorType> lhs, const LogicalExpression<FactorType> &rhs)
	{
		lhs *= rhs;
		return lhs;
	}

	//! 比較演算子(std::setへの格納に必要) 単項 < 単項(入れ子) < 多項式、同種なら先頭の成分の<を実行
	friend bool operator<(const LogicalExpression<FactorType> &lhs, const LogicalExpression<FactorType> &rhs)
	{
		if(lhs.empty()) return rhs.empty() ? false : true;
		auto lkind = lhs.kind(), rkind = rhs.kind();
		if(lkind != rkind) {
			return static_cast<int>(lkind) < static_cast<int>(rkind);
		} else {
			if(lkind == KIND::MONOMIAL) {
				auto lsize = lhs.factors_.size(), rsize = rhs.factors_.size();
				return (lsize != rsize) ? lsize < rsize : lhs.factors_.front() < rhs.factors_.front();
			} else if(lkind == KIND::NESTEDMONO) {
				auto lsize = lhs.factorPolys_.size(), rsize = rhs.factorPolys_.size();
				return (lsize != rsize) ? lsize < rsize : lhs.factorPolys_.front() < rhs.factorPolys_.front();
			} else {
				auto lsize = lhs.factorPolys_.size(), rsize = rhs.factorPolys_.size();
				return (lsize != rsize) ? lsize < rsize : lhs.terms_.front() < rhs.terms_.front();
			}
		}
	}
	//! 等号。factors_内、terms_内は順不同
	friend bool operator ==(const LogicalExpression<FactorType> &lhs, const LogicalExpression<FactorType> &rhs) {
		if((lhs.terms_.empty() != rhs.terms_.empty()) || (lhs.factors_.empty() != rhs.factors_.empty())
		|| (lhs.factorPolys_.empty() != rhs.factorPolys_.empty())) {
			return false;
		} else if(!lhs.factors_.empty()) {
			// 単項式の場合factors_のユニーク成分が同じなら同値とする
			return lhs.factorSet() == rhs.factorSet();
		} else if(!lhs.factorPolys_.empty()) {
			if(lhs.factorPolys_.size() != rhs.factorPolys_.size()) return false;
			std::set<LogicalExpression<FactorType>> lhsSet(lhs.factorPolys_.cbegin(), lhs.factorPolys_.cend());
			std::set<LogicalExpression<FactorType>> rhsSet(rhs.factorPolys_.cbegin(), rhs.factorPolys_.cend());
			for(auto lit = lhsSet.cbegin(), rit = rhsSet.cbegin(); lit != lhsSet.cend(); ++lit,++rit) {
				if(lit->factorSet() != rit->factorSet()) return false;
			}
		} else {
			// 多項式の場合コンテナを比較std::setなので一意かつ順序は保証されているのでそのままset::operator==で比較すれば良い
			if(lhs.terms_.size() != rhs.terms_.size()) return false;
			std::set<LogicalExpression<FactorType>> lhsSet(lhs.terms_.cbegin(), lhs.terms_.cend());
			std::set<LogicalExpression<FactorType>> rhsSet(rhs.terms_.cbegin(), rhs.terms_.cend());
			for(auto lit = lhsSet.cbegin(), rit = rhsSet.cbegin(); lit != lhsSet.cend(); ++lit,++rit) {
				if(lit->factorSet() != rit->factorSet()) return false;
			}
		}
		return true;
	}
	friend bool operator !=(const LogicalExpression<FactorType> &lhs, const LogicalExpression<FactorType> &rhs) {return !(lhs == rhs);}



private:
	/*
	 * 論理多項式はツリー構造を取るのでメンバに多項式自体をメンバに持つ。
	 * 単項式の場合は
	 * ・項を構成する因子を持つ。
	 * ・項を構成する因子(多項式の入れ子)
	 * の何れかを持つ。
	 *
	 * 因子(多項式の入れ子)を持たせずに毎回展開してしまうと項数が爆発するので
	 * 式を評価するまで展開せずにvectorで保持する。
	 *
	 * よってterms_, factors_, factorPolys_はどれかひとつだけが非emptyである。
	 */
	// 単項式を構成する因子vector
    std::vector<FactorType> factors_;  // 因子の積のみで構成される場合はこの変数で保持
    std::vector<LogicalExpression<FactorType>> factorPolys_;  // 多項式の積のみで構成される場合
	// 多項式を構成する各項(項もまた多項式)
    std::vector<LogicalExpression<FactorType>> terms_;  // 複数の項の和で構成される場合。

	// factors_をセット化して返す.この時重複は排除され一意になる。
	// 表面と裏面は同一視する。
	auto factorSet() const
	{
		return std::set<FactorType, FactorComparator>(factors_.cbegin(), factors_.cend());
	}
	// 単項式なら1、単項式(多項式の入れ子あり)なら2、多項式なら3を返す。
	KIND kind() const
	{
		if(!factors_.empty()) {
			return KIND::MONOMIAL;
		} else if (!factorPolys_.empty()) {
			return KIND::NESTEDMONO;
		} else {
			return KIND::POLYNOMIAL;
		}
	}
};

template <class FactorType> const char LogicalExpression<FactorType>::OP_ADD = ':';
template <class FactorType> const char LogicalExpression<FactorType>::OP_MUL = ' ';




/*
 * FactorType： LPolynomialの因子型  想定される型は int
 * FunctorType：ファンクタの型。operator()(FactorType index, FunctorArgType pos)で posがindex面の表側ならtrueを返す。実装は
 *             indexに対応したsurfaceのisForward(pos)を呼び出す
 * FunctorArgType： ファンクタの引数型   想定される型は math::Point
 */
template <class FactorType, class FunctorType, class FunctorArgType>
bool evaluateL(const LogicalExpression<FactorType> &polynomial, const FunctorType &functor, const FunctorArgType &arg)
{
	assert(!polynomial.empty());
	if(!polynomial.factors_.empty()) {
		for(const auto& fac: polynomial.factors_) {
			if(!functor(fac, arg)) return false;  // 積は一つでもfalseがあるとfalse
		}
		return true;
	} else if(!polynomial.factorPolys_.empty()) {
//        // 積は一つでもfalseがあるとfalse
//        return !std::any_of(polynomial.factorPolys_.cbegin(), polynomial.factorPolys_.cend(),
//                            [&functor, &arg](const LogicalExpression<FactorType> poly)
//                            {
//                                return !evaluateL(poly, functor, arg);
//                            });
//      わかりやすくループで書くとこうなる
        for(const auto &poly: polynomial.factorPolys_) {
            if(!evaluateL(poly, functor, arg)) return false;
        }
        return true;

	} else {
		for(const auto &term: polynomial.terms_) {
			if(evaluateL(term, functor, arg)) return true; // 和は一つでもtrueがあるとtrue
		}
		return false;
	}
	throw std::runtime_error("Programing error: this line should not be reached. polynomial=" + polynomial.toString());
}





template <class T>
std::string complimentedString(const std::string &equation)
{
    /*
     * 実は面コンプリメントは入れ子にできるたとえば#(-2:#(1))
     * なのでLogicalExpression::fromStringは#を考慮する必要がある。
     */
    return LogicalExpression<T>::fromString(equation).complimented().toString();
}

}  // end namespace lg
#endif // LPOLYNOMIAL_HPP
