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
#include "string_utils.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "message.hpp"


// cが特殊文字の場合エスケープ記号を加えた文字列を返す。
inline
std::string escapeSpecialChar(char c)
{
	static const std::string escapeNeededChars("\\*+.?{}()[]^$|");
	if(escapeNeededChars.find(c) != std::string::npos) {
		return std::string("\\") + std::string{c};
	} else {
		return std::string{c};
	}
}

// leftMark, rightMarkの引用符による引用入れ子深さを計算する。
// 第三引数がfalseなら先にrightMarkが出現するなどし一時的に入れ子深さが負になることを許容せず
// その場合はinvalid_argumentが送出のされる。
int calcNestDepth(char leftMark, char rightMark, const std::string &str, bool acceptNegativeNest)
{
	int nestDepth = 0;
	if(leftMark == rightMark) {
		// 左右の区別が無い場合は単純に数を数えて2の剰余を取れば良い
		// acceptNegativeNest==trueなら常に負にとりそれ以外の場合は正にとる。
		// これは負のネスト深さを許容するかではなく、要素を連結していく中で
		// 最初の要素の場合は正にとり、それ以外の場合は負に取るのが適切である。
		// という振る舞いとたまたま一致しているからこの扱いでうまく行く。
		if(acceptNegativeNest) {
			nestDepth = -static_cast<int>(std::count(str.cbegin(), str.cend(), leftMark))%2;
		} else {
			nestDepth = static_cast<int>(std::count(str.cbegin(), str.cend(), leftMark))%2;
		}
	} else {
		// 引用符に左右の区別が有る場合
		std::string::size_type markPos = 0;
		const std::string markStr{leftMark, rightMark};
		markPos = str.find_first_of(markStr, markPos);
		while(markPos != std::string::npos) {
			if(str.at(markPos) == leftMark) {
				++nestDepth;
			} else {
				--nestDepth;
			}
			if(!acceptNegativeNest && nestDepth < 0) {
				throw std::invalid_argument("");
			}
			markPos = str.find_first_of(markStr, markPos+1);
		}
	}
	return nestDepth;
}


/*!
 * 最も一般化された文字列分割関数(マルチバイト対応)
 * @param[in] (qmarkMap) 対になる引用符の左側をキーとして右側を値として格納するマップ
 * @param[in] (delims) セパレータ文字(複数種可)をつなげた文字列
 * @param[in] (sourceStr) 分割対象文字列
 * @param[in] (ignoreEmpty) 空の要素を無視するフラグ
 * @return 文字列を分割したstd::vector<std::string>を返す
 * @sa split
 *
 * delimsに含まれる文字(複数可)をセパレータにしてsourceStrを分割したvector<string>を返す。
 * qmarkMap.at(n).first及びqmarkMap.at(n).secondをそれぞれ左側引用符、右側引用符として
 * 引用符で引用した内部に関しては分割を行わない。また引用符は削除しない。 * 最後の引数がtrueなら空の要素は無視する。
 *  split2(map{{'(', ')'},{'"', '"'}}, "+=", "(a+b)=c+d", true) はvector{"(a+b)", "c", "d"}を返す。
 * 引用符はchar型であるためマルチバイト文字は許容されない。delims, sourceStrにはマルチバイト文字が許容される。
 */
std::vector<std::string> utils::splitString(const std::unordered_map<char, char> &qmarkMap,
									  const std::string &delims, const std::string &sourceStr, bool ignoreEmpty)
{
	/*
	 * 方針。
	 * ・とりあえずdelimsで区切ってvector化してqmarkを含んでいる部分は連結。
	 *   元のdelimsのどれがヒットしていたかがわからないので別途保持しておくとか面倒な作業が出るが仕方ない。
	 *
	 */

	std::vector<std::string> strVec;
	std::vector<char> seps;  //分割された要素の間にあったseparatorを保存する。
	strVec.emplace_back(sourceStr);
	for(auto &delim: delims) {
		std::vector<std::string> tmpVec; // 1つのdelimiterで分割した結果
		std::vector<char> tmpSeps;
		size_t numInsertedSeps = 0;  // sepsにinsertすると sepsの要素番号とretVecの要素番号がずれるのでその補正
		for(size_t j = 0; j < strVec.size(); ++j){
		//for(auto &str: retVec) {
			std::vector<std::string> splitResult;
			std::vector<char> splitSeps;
			std::regex separator{escapeSpecialChar(delim)}; // 分割用のセパレータ
			auto it = std::sregex_token_iterator(strVec.at(j).begin(), strVec.at(j).end(), separator, -1);
			size_t numChar = 0;

			//mDebug() << "splitting string \"" << retVec.at(j) << "\"";
			while (it != std::sregex_token_iterator()) {
				//mDebug() << "adding splited word \"" << *it << "\"";
				++numChar += std::string(*it).size();
				splitResult.push_back(*it++);
				splitSeps.push_back(delim);
			}
			// a*b* を*で分割する場合 sregex_token_iteratorはaとbだけを返すが、複数文字でsplitするときには
			// 分割する順番で結果が変わらないようにa, b,空を返したいので、最後の文字の後にセパレータが有る場合空文字を追加する。
			if(numChar-1 < strVec.at(j).size()) {
				splitResult.push_back("");
				splitSeps.push_back(delim);
			}
			splitSeps.pop_back();  // separaterは分割された要素より1個少ないので最後の1個は取り除く
			tmpVec.insert(tmpVec.end(), splitResult.begin(), splitResult.end());

			// ここのループではj番目のstring要素に対する分割を行ったので、sepsのj番目にsplitSepsを挿入する。
			//tmpSeps.insert(tmpSeps.end(), splitSeps.begin(), splitSeps.end());
			// j番目のstringは間違いないが、sepsはインサートされた時点で順序が変わってj番目ではなくなる。
			seps.insert(seps.begin() + j + numInsertedSeps, splitSeps.begin(), splitSeps.end());
			numInsertedSeps += splitSeps.size();
		}
//		mDebug() << "delimiter" << delim << "tmpVec=" << tmpVec;
		tmpVec.swap(strVec);
		//tmpSeps.swap(seps);
	}
	assert(strVec.size()-1 == seps.size());

//	mDebug() << "before concat quoted element, separated strs  =" << strVec;
//	mDebug() << "before concat quoted element,      separaters =" << seps;


	// ここから引用符を頼りにデータを連結していく。
	std::string leftQmarks;  // 左側引用符のリスト
	for(auto it = qmarkMap.begin(); it != qmarkMap.end(); ++it) {
		leftQmarks += std::string{it->first};
	}

	std::vector<std::string> retVec;
	for(size_t i = 0 ; i < strVec.size();++i) {
		std::string currentStr = strVec.at(i);
		auto leftPos = currentStr.find_first_of(leftQmarks);
		if(leftPos == std::string::npos) {
			// 引用符無しならそのままリターン
			retVec.emplace_back(currentStr);
			continue;
		}

		char leftMark = currentStr.at(leftPos);
		char rightMark = qmarkMap.at(leftMark);

		int nestDepth = 0;
		try {
			// 最初の文字列要素でいきなり入れ子レベルが負になったら不正データなので第四引数はfalseを与える。
			nestDepth = calcNestDepth(leftMark, rightMark, currentStr, false);
		} catch(std::invalid_argument &e) {
            (void)e;
			// 右側引用符が先に出現した場合はエラー
			throw std::invalid_argument(std::string("In \"") + sourceStr
										+ "\" right-side quotation mark or parenthesis appeared before left-side one");
		}

		// 入れ子深さが0ならば引用符は考慮しないでよい＝データ連結せずにそのままにする。
		if(nestDepth == 0) {
			retVec.emplace_back(currentStr);
			continue;
		}

		while(nestDepth != 0) {
			++i;
			//mDebug() << "i=" << i << "nest=" << nestDepth;
			// 入れ子が丁度終わらないまま最後の文字列要素まで来てしまったらエラーにする
			if(i >= strVec.size()) {
				throw std::invalid_argument(std::string("\"") + sourceStr + "\" has too much "
											+ "left side  quotation marks[" + std::string{leftMark} + "].");
			}
			// 次の文字列要素で左側引用符数と右側引用符数を数える。
			nestDepth = nestDepth + calcNestDepth(leftMark, rightMark, strVec.at(i), true);

			// 引用深さが負になるのは右側引用符過剰。なのでエラーにする。
			if(nestDepth < 0) {
				throw std::invalid_argument(std::string("\"") + sourceStr + "\" has too much right side quotation marks.");
			}

			currentStr = currentStr + seps.at(i-1) + strVec.at(i);
		}
		//mDebug() << "Nesting by " << leftMark << "and" << rightMark << " done. string=" << currentStr;
		retVec.emplace_back(currentStr);

		//++i;
	}

	// 空白要素の削除
	if(ignoreEmpty) {
		auto itrEnd = std::remove_if(retVec.begin(), retVec.end(), [](const std::string &str){return str.empty();});
		retVec.erase(itrEnd, retVec.end());
	}

	return retVec;
}

/*!
 * @brief splitString(std::unordered_map, std::string, std::string, bool)の単一種引用符版オーバーロード
 * @sa splitString
 */
std::vector<std::string> utils::splitString(const std::pair<char, char> &qmarkPair, const std::string &delims,
											const std::string &sourceStr, bool ignoreEmpty)
{
	return splitString(std::unordered_map<char, char>{{qmarkPair.first, qmarkPair.second}},
					   delims, sourceStr, ignoreEmpty);
}

/*!
 * @brief splitString(std::unordered_map, std::string, std::string, bool)の単一種左右対称引用符版オーバーロード
 * @sa splitString
 */
std::vector<std::string> utils::splitString(char qmark, const std::string &delims,
											const std::string &sourceStr, bool ignoreEmpty)
{
	return splitString(std::make_pair(qmark, qmark), delims, sourceStr, ignoreEmpty);
}
/*!
 * @brief splitString(std::unordered_map, std::string, std::string, bool)の引用符無し版オーバーロード
 * @sa splitString
 */
std::vector<std::string> utils::splitString(const std::string &delims, const std::string &sourceStr, bool ignoreEmpty)
{
	return splitString(std::unordered_map<char, char>(), delims, sourceStr, ignoreEmpty);
}


/*!
 * クオーテーションを外す関数
 * @param[in] (qmarkMap) 対になる引用符の左側をキーとして右側を値として格納するマップ
 * @param[in] (str) 処理対象文字列
 * @return 最も外側のクオーテーションを外した文字列を返す
 *
 * 対応する引用符が文字列の両端に無い場合は何もしない。
 * dequote(map{'(',')'}, "(a)+(b)")はそのまま引数"(a)+(b)"を返す
 * dequote(map{'(',')'}, "(a+b)")は"a+b"を返す
 *
 */
std::string utils::dequote(const std::unordered_map<char, char> &qmarkMap, std::string str, bool doTrim)
{
	if(qmarkMap.empty()) return str;
	if(doTrim) utils::trim(&str);

	// dequote(map{'(',')'}, "(a)+(b)")の返り値がa)+(bになってはいけない

	// まずクオートしているマークをmarkMapから探す。
	std::unordered_map<char, char>::const_iterator markIt;
	while(markIt = qmarkMap.find(str.front()), markIt != qmarkMap.end()) {
		auto lmark = str.front(), rmark = markIt->second;
		assert(lmark != rmark);
		auto rpos = findMatchedBracket(str, 0, lmark, rmark);
		if(rpos == str.size()-1) {
			str = str.substr(1, str.size()-2);
			if(doTrim) utils::trim(&str);
		} else {
			break;
		}
	}
	return str;

//	while(qmarkMap.find(str.front()) != qmarkMap.end()) {
//		if(str.back() != qmarkMap.at(str.front())) break;  // strのクオーテーション右側が引用符でなければbreak
//		str = str.substr(1, str.size()-2);
//		utils::trim(&str);
//	}
//	return str;
}

/*!
 * @brief dequote(std::unordered_map, std::string, std::string, bool)の単一種左右非対称引用符版オーバーロード
 * @sa dequote
 */
std::string utils::dequote(std::pair<char, char> marks, std::string str, bool doTrim)
{
	return dequote(std::unordered_map<char, char>{marks}, str, doTrim);
}


/*!
 * @brief dequote(std::unordered_map, std::string, std::string, bool)の単一種左右対称引用符版
 * @sa dequote
 */
std::string utils::dequote(char qmark, std::string str, bool doTrim)
{
	if(str.size() < 2) return str;
	if(doTrim) utils::trim(&str);

	// dequoteの左右区別版は左と右が同じだと何もしない。のでだめ
	// return dequote(std::make_pair(qmark, qmark), str, doTrim);

	// 左右非対称マークの場合 (a)+(b) は左右のバランスを考えてデクオートしないのが正しい。["a)+(b"とするのは間違い]
	// 左右対称マークの場合例えば  "a"+"b" はどう返すべきか？
	// 解釈1 「一番外側で式全体をquoteし、かつ+文字をquoteしている」
	// 解釈2 「文字aと文字bをそれぞれquoteしている」
	// どちらが正しいか区別できない。
	// 重要なのは文字列の中身を問わず一貫した解釈ができること。

	// 案1 とにかく一番外側がquoteされていればそこをquoteと見做す → 解釈1
	// 案2 quoteの入れ子は認めない→ 解釈2を

	// 案2の方が素直な解釈なのでこちらを採用する。

	// 今の実装a"+"bでもいい気がするが多項式の処理みたいなのに使うことを考えるとデクオートしないのが正解という気がする。
	// 結論両端がマッチしていない場合はdequoteしない。
	// "a"+"b" → "a"+"b"
	// ""a+b"" → ""a+b""  ← このケースでは空の文字を引用符で括っているとも解釈できるのでdequoteしない
	// ""a+b" → ""a+b"
	// " "a+b" " → " "a+b" " (doTrim=true)の場合

	if(str.front() != qmark || str.back() != qmark) {
		return str;
	} else {
		std::string content = str.substr(1, str.size()-2);
		if(content.find(qmark) != std::string::npos) {
			return str;
		} else {
			if(doTrim) utils::trim(&content);
			return content;
		}
	}
}



void utils::removeRedundantBracket(char leftBracket, char rightBracket, std::string *str, bool doTrim)
{
	if(doTrim) utils::trim(str);

	if(str->empty()) return;

	while(!str->empty() && str->front() == leftBracket && str->back() == rightBracket) {
		if(doTrim) utils::trim(str);
		auto rpos = utils::findMatchedBracket(*str, 0, '(', ')');
		if(rpos == str->size()-1) {
			*str = str->substr(1, str->size()-2);
		} else {
			break;
		}
	}
}


std::string::size_type utils::findMatchedBracket(const std::string &str, std::string::size_type braketPos,
												 char leftBracket, char rightBracket)
{
	if(str.at(braketPos) != leftBracket) {
		std::stringstream ss;
		ss << "string.at(left)=" << str.at(braketPos) << " is not left bracket";
		throw std::invalid_argument(ss.str());
	}
	std::string brackets("  ");
	brackets.at(0) = leftBracket;
	brackets.at(1) = rightBracket;

	int bracketCount = 1;
	auto brPos = braketPos;
	while(brPos = str.find_first_of(brackets, brPos+1), brPos != std::string::npos) {
		if(str.at(brPos) == leftBracket) {
			++bracketCount;
		} else if(str.at(brPos) == rightBracket) {
			--bracketCount;
		} else {
			assert(!"not bracket!");
		}
		if(bracketCount == 0) return brPos;
	}
	return std::string::npos;
}










std::pair<std::string, std::string> utils::separatePath(const std::string &source)
{
#if defined(_WIN32) || defined (_WIN64)
    std::string sep = "\\/";
#else
    std::string sep = "/";
#endif
	typedef std::pair<std::string, std::string> pair_type;
    std::string::size_type sepPos = source.find_last_of(sep);
	// 区切り文字がない場合全体がファイル文字列
    if(sepPos == std::string::npos) {
        return pair_type("", source);
    } else {
        std::string path = source.substr(0, sepPos+1);
        std::string file = source.substr(sepPos+1);
        return std::make_pair(path, file);
    }
}




void utils::trim(std::string *str)
{
	if(str->size() == 0) return;

	std::string::size_type startpos = str->find_first_not_of(" \t");
	std::string::size_type endpos   = str->find_last_not_of(" \t");
	if(startpos == std::string::npos || endpos == std::string::npos) {
		// 空白しかないからクリアする。
		str->clear();
		return;
	} else {
//		std::cout << "start=" << startpos << std::endl;
//		std::cout << "end=" << endpos << std::endl;
		*str = str->substr(startpos, endpos - startpos + 1);
		return;
	}
}
std::string utils::trimmed(const std::string &str)
{
	std::string retstr = str;
	trim(&retstr);
	return retstr;
}





bool utils::isArithmetic(const std::string& str)
{
	try{
		stringTo<double>(str);
	} catch (std::invalid_argument &e) {
        (void) e;
		return false;
	}
	return true;
}

bool utils::isArithmeticVector(const std::vector<std::string> &svec)
{
	for(auto& elem: svec) {
		if(!isArithmetic(elem)) return false;
	}
	return true;
}
bool utils::isArithmeticVector(const std::vector<std::string>::const_iterator beg,
						   const std::vector<std::string>::const_iterator en)
{
	for(auto it = beg; it != en; it++) {
		if(!isArithmetic(*it)) {
			//std::cout << "string=\"" << *it << "\" is not a number." << std::endl;
			return false;
		}
	}
	return true;
}


void utils::toupper(std::string *str)
{
	std::transform(str->begin(), str->end(), str->begin(), ::toupper);
}

void utils::tolower(std::string *str)
{
	std::transform(str->begin(), str->end(), str->begin(), ::tolower);
}

std::string utils::lowerString(const std::string &str)
{
	std::string retStr = str;
	utils::tolower(&retStr);
	return retStr;
}




void utils::uniteSpaces(std::string *str)
{
	std::vector<std::string> tmpVec = splitString(" ", *str, true);
	std::string result;
	for(size_t i = 0; i < tmpVec.size(); ++i) {
		result += tmpVec.at(i);
		if(i != tmpVec.size()-1) {
			result += " ";
		}
	}
	*str = result;
}

std::string utils::spaces(size_t n)
{
	std::stringstream ss;
	for(size_t i = 0; i < n; ++i) {
		ss << " ";
	}
	return ss.str();
}

bool utils::startWith(const std::string &source, const std::string &pattern)
{
	if(source.size() < pattern.size()) return false;

	return source.substr(0, pattern.size()) == pattern;
}



bool utils::isInteger(const std::string &str)
{
	std::stringstream ss;
	ss << str;
	int intvalue;
	ss >> intvalue;
	return !ss.fail();
}


void utils::removeInlineComment(char commChar, std::string *str)
{
	auto pos = str->find_first_of(commChar);
	if(pos != std::string::npos) {
		*str = str->substr(0, pos);
	}
}


std::string utils::paddedString(char pad, int sz, const std::string &str, bool left)
{
	if(sz < 0) return str;
	if(static_cast<int>(str.size()) >= sz) return str;
	if(left) {
		return std::string(sz - str.size(), pad) + str;
	} else {
		return str + std::string(sz - str.size(), pad);
	}
}

/*!
 * 左括弧と右括弧のバランスを調べる
 * @param[in] (left) 左括弧文字
 * @param[in] (right) 右括弧文字
 * @param[in] (str) 検査対象文字列
 * @param[in] (acceptMismatch) もじれつ半ばでの右括弧過剰を許容するかのフラグ
 * @return 左括弧数-右括弧数
 */
int utils::parenthesisBalance(char left, char right, const std::string &str, bool acceptMismatch)
{
	// 合計数だけならalgorithmで計算する
	if(acceptMismatch) {
        return static_cast<int>(std::count(str.cbegin(), str.cend(), left)
                                - std::count(str.cbegin(), str.cend(), right));
	} else {
		int num = 0;
		for(const auto &c: str) {
			if(c == left) {
				++num;
			} else if(c == right) {
				--num;
			}
			if(num < 0) {
				throw std::invalid_argument("Too much right parenthesis. str=" + str);
			}
		}
		return num;
	}
}

int utils::complimentedFactor(const int fac)
{
	return -1*fac;
}

std::string utils::complimentedFactor(const std::string &fac)
{
	if(fac.empty()) throw std::invalid_argument("empty factor for comlimented()");
	std::string retval = fac;
	if(retval.front() == '+') retval = retval.substr(1);
	if(retval.empty()) throw std::invalid_argument("empty(only sign) factor for complimented()");
	if(retval.front() == '-') {
		return retval.substr(1);
	} else {
		return "-" + retval;
	}
}

template<>
int utils::toInt(const int& val) {return val;}

std::string utils::expandRangeString(const std::string &str)
{
	std::smatch sm;
//	std::regex bracedPattern(R"(\{ *(\d+) * - * (/d+) *\})");
	std::regex bracedPattern(R"(\{ *(\d+) *- *(\d+) *\})");
	if(std::regex_search(str, sm, bracedPattern)) {
		assert(sm.size() == 3);
		int startNum = utils::stringTo<int>(sm.str(1));
		int endNum = utils::stringTo<int>(sm.str(2));
		if(endNum <= startNum) {
			throw std::invalid_argument(std::string("2nd argument should be larger than the 1st in braced expression = \"") + str);
		}
		std::vector<std::string> numVec;
		for(int i = startNum; i <= endNum; ++i) {
			numVec.push_back(std::to_string(i));
		}
		return utils::concat(numVec, " ");
	}
	return str;
}


std::string utils::canonicalName(const std::string &orgName)
{
	auto commaPos = orgName.find(".");
	std::string base = orgName, suffix;
	if(commaPos != std::string::npos) {
		base = orgName.substr(0, commaPos);
		suffix = orgName.substr(commaPos +1);
	} else {
		base = orgName;
	}
	char header = 0;
	if(base.size() >= 1 && (base.front() == '+' || base.front() == '*')) {
		header = base.front();
		base = base.substr(1);
	}

	std::string retStr;
	try {
		auto newbase = std::to_string(utils::stringTo<int>(base));
		retStr = (suffix.empty()) ? newbase : newbase + "." + suffix;
	} catch (...) {
		retStr = (suffix.empty()) ? base : base + "." + suffix;
	}
	if(header != 0) retStr = std::string{header} + retStr;
	return retStr;
}
