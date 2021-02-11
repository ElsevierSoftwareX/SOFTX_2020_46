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
#include "cellparameter.hpp"

#include <unordered_set>
#include <unordered_map>

#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "ijmr.hpp"

/*
 *  問題は 番号とparticle designatorがあるのとないのと。 xxxN:MのNとM
 *  番号を持つものは番号を省略可能なので要注意.パラメータ類は以下の5種に分類する。
 * ・両方ないもの  : u, lat, nonu pwt vol rho mat
 * ・両方あるもの  : dxc wwn
 * ・designatorのみ: ext fcl imp
 * ・番号のみ      : pd tmp
 * ・可変長： fill trcl
 *
 * パラメータ引数は
 * trcl: = 可変長 (trcl=2  trcl=(cos(0) 2r 9j))
 * fill: 可変長
 * tmp: 可変 (tmp1=100, tmp= 200 4r)
 * wwn: 可変 (wwn1:N=0.5, wwn:N 0.5 2m)
 * u:  u= 1個 (u=1)
 * lat 1個(123のどれか)
 * dxc: 1個 (dxc1:n=1) みたいな感じ。
 * ext: 1個 (ext:n=1)
 * fcl: 1個 (fcl:n=1)
 * imp: 1個 (imp:n=1)
 * nonu 0個
 * pd:  1個 (pd1=1)
 * pwt: 1個 (pwt=1)
 * vol: 1個 (vol=10)
 * rho: 1個 (row=0.5)
 * mat: 1個 (mat=hydrogen)
 *
 * ※ trcl,fillは ijmr展開の対象となる。
 *
 *
 */
const int FIXED = 0;
const int DESIGNATOR = 1;
const int NUMBER = 2;
const int ASTERISK = 3;

#include <bitset>
// paramter名をキーにして、「先頭アスタリスク可、番号あり、Designatorあり、固定長かどうか」 のbitset
const std::unordered_map<std::string, std::bitset<4>> &parameterMap()
{
    static const std::unordered_map<std::string, std::bitset<4>> params
    {
        {"trcl", std::bitset<4>("1000")},
        {"fill", std::bitset<4>("1000")},
		{"tmp",  std::bitset<4>("0100")},  // 1つのセルに時間別の複数の温度を与える可変長入力あり
		{"wwn",  std::bitset<4>("0110")},  // 1つのセルにエネルギー別に複数のww下限を与える可変長入力あり
        {"u",    std::bitset<4>("0001")},
        {"lat",  std::bitset<4>("0001")},
        {"dxc",  std::bitset<4>("0111")},
        {"ext",  std::bitset<4>("0011")},
        {"fcl",  std::bitset<4>("0011")},
        {"imp",  std::bitset<4>("0011")},
        {"nonu", std::bitset<4>("0001")},
        {"pd",   std::bitset<4>("0101")},
        {"pwt",  std::bitset<4>("0001")},
        {"vol",  std::bitset<4>("0001")},
        {"rho",  std::bitset<4>("0001")},
		{"mat",  std::bitset<4>("0001")},
		// phits専用パラメータ
		{"tfile", std::bitset<4>("0001")},
		{"tsfac", std::bitset<4>("0001")}
    };
    return params;
}


//　↑のパラメータを組み合わせて番号なしdesignatorなしパラメータ(可変長、固定長両方)のパラメータセットを返す
std::unordered_set<std::string> getNotNumberedNorDesignatedParams()
{
    std::unordered_set<std::string> params;
    // p.second はbitsetで0が番号あり、1がdesignatorあり、2が固定長
    for(const auto &p: parameterMap()) {
        if(!p.second[NUMBER] && !p.second[DESIGNATOR]) {
            params.emplace(p.first);
        }
    }
	return params;
}
std::unordered_set<std::string> getFixedParams()
{
	std::unordered_set<std::string> params;
	for(const auto &p: parameterMap()) {
		if(p.second[FIXED]) params.emplace(p.first);
	}
	return params;

}

std::unordered_set<std::string> getAsteriskParams() {
    std::unordered_set<std::string> params;
    // p.second はbitsetで0が番号あり、1がdesignatorあり、2が固定長
    for(const auto &p: parameterMap()) {
//        mDebug() << p.first << p.second << p.second[ASTERISK]
//                    << p.second[NUMBER] << p.second[DESIGNATOR] << p.second[FIXED];
        if(p.second[ASTERISK]) params.emplace(p.first);
    }
    return params;
}
std::unordered_set<std::string> getNumberedAndDesignatedParams()
{
    std::unordered_set<std::string> params;
    // p.second はbitsetで0が番号あり、1がdesignatorあり、2が固定長
    for(const auto &p: parameterMap()) {
        if(p.second[NUMBER] && p.second[DESIGNATOR]) {
            params.emplace(p.first);
        }
    }
    return params;
}
// 番号ありパラメータのセットを返す
std::unordered_set<std::string> getNumberedNotDesignatedParams()
{
    std::unordered_set<std::string> params;
    for(const auto &p: parameterMap()) {
        if(p.second[NUMBER] && !p.second[DESIGNATOR]) {
//            mDebug() <<"pname===" << p.first << p.second[0] << p.second[1] << p.second[2] << p.second;
            params.emplace(p.first);
        }
    }
    return params;
}
// Designatorあり番号なしパラメータを返す
std::unordered_set<std::string> getDesignatedParams()
{
    std::unordered_set<std::string> params;
    for(const auto &p: parameterMap()) {
        if(p.second[DESIGNATOR]) {
            params.emplace(p.first);
        }
    }
    return params;
}


std::pair<bool, int> inp::isCellFixedParam(const std::string &str)
{
	static const std::unordered_set<std::string> params = getFixedParams();
	auto it = params.find(str);
	if(it != params.end()) {
		int num = (str == "nonu") ? 0 : 1;  // 引数0はnonuのみ。あとは1個の引数を取る。
		return std::make_pair(true, num);
	} else {
		return std::make_pair(false, -1);
	}
}

bool inp::isCellParam(std::string str)
{
	utils::trim(&str);
	utils::tolower(&str);
    // 数字のみの入力はセルパラメータになりえないのでfalseを返す
	if(str.find_first_not_of("+-0123456789)(") == std::string::npos) return false;

	// とりあえず番号なし、designatorなしはセットと直接比較
	static const std::unordered_set<std::string> simpleParams = getNotNumberedNorDesignatedParams();
	auto it = simpleParams.find(str);

	if(it != simpleParams.end()) return true;
	if(str.size() == 1) return false; // 1文字セルパラメータはuだけなのでu非該当で1文字ならセルパラメータではない

    // 先頭にアスタリスクがある場合はasteriskParamと比較.
    // 幸い*fill, *trclは番号もdesignatorもないことが確定しているので
    // asteriskParamsに該当しなければfalseを返して良い
    if(str.front() == '*') {
        static const std::unordered_set<std::string> asteriskParams = getAsteriskParams();
        it = asteriskParams.find(str.substr(1));
        if(it != asteriskParams.end()) {
            return true;
        } else {
            return false;
        }
    }

	// ここからは strをpname, number, designatorに分ける。

	auto colonpos = str.find_first_of(":");
	if(colonpos == std::string::npos) {
		// コロンが無いのであとは番号のみありのパラメータと比較。
        auto numpos = str.find_first_of("0123456789");
        static const std::unordered_set<std::string> numberedParams = getNumberedNotDesignatedParams();
		it = numberedParams.find(str.substr(0, numpos));
		if(it != numberedParams.end()) return true;
	} else 	if(colonpos != std::string::npos) {
		// コロンがあったらdesignatorをカットして designatorのみパラメータと比較。
		std::string pname = str.substr(0, colonpos);
		static const std::unordered_set<std::string> designatedParams = getDesignatedParams();
		it = designatedParams.find(pname);
		if(it != designatedParams.end()) return true;
		// あとは数字部分をカットして 番号・designator両持ちパラメータと比較する。
        static const std::unordered_set<std::string> numberedDesignatedParams = getNumberedAndDesignatedParams();
        auto numpos = pname.find_first_of("0123456789");
		it = numberedDesignatedParams.find(pname.substr(0, numpos));
		if(it != numberedDesignatedParams.end())return true;
	}
	return false;

}

// cellParamStrの先頭をTRCL引数として、正準なtrcl文字列化してtrStringに代入する。
// この時当該部分はcellParamStrから削除される。
void inp::getCellTrStr(bool hasAsterisk, std::string *cellParamStr, std::string *trString)
{
	assert(cellParamStr);
	assert(trString);
//	mDebug() << "Enter getCellTrStr has asterisk?===" << hasAsterisk << ", current cellStr===" << *cellParamStr << "trString===" << *trString;
	std::string prefix = hasAsterisk ? "*" : "";
	utils::trim(cellParamStr);
	if(cellParamStr->empty()) return;
	/*
	 *  cellParamStrは TR番号と括弧内直書きの複合かつ間のスペースあったりなかったりの
	 * ・ケース1：TR番号                 "2"
	 * ・ケース2：直書き                 "({cos(0)} 0 0)"
	 * ・ケース3：TR+直書きスペースなし  "2(abs(-1) 0 0)" という場合がある。
	 * ・ケース4：TR+直書きスペースあり  "2 ({exp(-1)} 0 0)"
	 * ・ケース5：直書きカンマ区切り複数 "(0 0 0, 10 0 0)  これはTRSFのための拡張内部表現
	 * とりあえず1要素取得して'('を含むかどうかと、残った文字列の先頭で判断する。
	 */
	std::string firstArg = inp::cutFirstElement(" ", cellParamStr);
	utils::trim(&firstArg);
	auto lbracket = firstArg.find_first_of("(");
	if(lbracket == std::string::npos) {
		// ケース1かケース4、
		firstArg = prefix + firstArg;
		if(!cellParamStr->empty() && cellParamStr->front() == '(') {
			// ケース4 TR＋スペースなし直書き
			*trString = trString->empty() ? firstArg : *trString + "," + firstArg;
			auto rbracket = utils::findMatchedBracket(*cellParamStr, 0, '(', ')');
			if(rbracket == std::string::npos) {
				throw std::invalid_argument("Parenthesis not matched in TRCL parameter. string=" + *cellParamStr);
			}
			*trString = *trString + "," + prefix + utils::trimmed(cellParamStr->substr(1, rbracket-1));
		} else {
			// ケース1 TR
			*trString = trString->empty() ? firstArg : *trString + "," + firstArg;
		}
	} else {
		// ケース2(5)かケース3
		*cellParamStr = firstArg + " " +  *cellParamStr; // cutFirstElementは切り出した部分を削除しているので取得した不完全データをcellStrに戻す。
		if(cellParamStr->front() == '(') {
			// ケース2(5) 直書き。ケース2はケース5の1成分版として包含される。
			auto rbracket = utils::findMatchedBracket(*cellParamStr, 0, '(', ')');
			if(rbracket == std::string::npos) {
				throw std::invalid_argument("Parenthesis not matched in complex TRCL parameter. string=" + *cellParamStr);
			}
			std::string tmpTrSrc = utils::trimmed(cellParamStr->substr(1, rbracket-1));
			assert(cellParamStr->size() > rbracket);
			*cellParamStr = cellParamStr->substr(rbracket + 1); // 読みこんだ部分をcellStrからカット

			auto trStrVec = utils::splitString(",", tmpTrSrc, true);
			for(auto trStrElem: trStrVec) {
				utils::trim(&trStrElem);
				*trString = trString->empty() ? prefix + trStrElem : *trString + "," + prefix + trStrElem;
			}
		} else {
			// ケース3 TR＋スペースあり直書き
			lbracket = cellParamStr->find_first_of("(");
			assert(lbracket != std::string::npos);
			auto tmpTr = prefix + utils::trimmed(cellParamStr->substr(0, lbracket));
			*trString = trString->empty() ? tmpTr : *trString + "," + tmpTr;
			*cellParamStr = cellParamStr->substr(lbracket);
			auto rbracket = utils::findMatchedBracket(*cellParamStr, 0, '(', ')');
			if(rbracket == std::string::npos) {
				throw std::invalid_argument("Parenthesis not matched in complex-TRCL parameter. string=" + *cellParamStr);
			}
			tmpTr = prefix + utils::trimmed(cellParamStr->substr(1, rbracket-1));
			assert(cellParamStr->size() > rbracket);
			*cellParamStr = cellParamStr->substr(rbracket + 1); // 読みこんだ部分をcellStrからカット
			*trString = *trString + "," + tmpTr;
		}
	}
}

