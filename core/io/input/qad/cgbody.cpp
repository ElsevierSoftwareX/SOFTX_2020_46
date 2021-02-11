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
#include "cgbody.hpp"

#include <regex>
#include "../../utils/message.hpp"
#include "../../utils/string_utils.hpp"

namespace {

constexpr size_t CGB_DATA_WIDTH = 10;

}  // end anonymous namespace

int inp::qad::CGBody::seqID_ = 0;
std::unordered_set<int> inp::qad::CGBody::usedIDs_;

bool inp::qad::CGBody::isEnd() const
{
	return std::regex_search(itype_, std::regex("end", std::regex_constants::icase));
}

std::string inp::qad::CGBody::toInputString() const
{
	std::stringstream ss;
	ss << ialp_ << "  " << itype_ << "  ";

	static std::unordered_map<std::string, size_t> argNumMap{
		{"box", 12}, {"rpp", 6}, {"sph", 4}, {"rcc", 7}, {"rec", 12},
		{"ell", 7}, {"trc", 8}, {"wed", 12}, {"arb", 30}
	};
	size_t numParams = argNumMap.at(itype_);
	if(numParams > fpd_.size()) {
		std::stringstream ss;
		ss << "Number of parameters for itype = " << itype_ << " should be "
			<< numParams << ". actual = " <<  fpd_.size();
		throw std::invalid_argument(ss.str());
	}
	for(size_t i = 0; i < numParams; ++i) {
		ss << fpd_.at(i);
		if(i != fpd_.size()-1) ss << "  ";
	}
	return ss.str();
}

int inp::qad::CGBody::getUniqueID()
{
	while(!(usedIDs_.insert(++seqID_)).second) {;}
	return seqID_;
}

void inp::qad::CGBody::initUniqueID()
{
	seqID_ = 0;
	usedIDs_.clear();
}

bool inp::qad::CGBody::isContData(const std::string &str)
{
	if(str.size() < 6) {
		return false;
	} else {
		return str.substr(0, 6).find_first_not_of(" ") == std::string::npos;
	}
}

// 連結処理済み文字列からCGBodyを作成。
inp::qad::CGBody inp::qad::CGBody::fromString(const std::string &str)
{
//	std::cout << "CGBody::fromString, str===\"" << str << "\"" << std::endl;
	std::string buff = str;
	utils::tolower(&buff);
	// 本当に困難にフォーマット守ってるのだろうか？なんとなく自由形式しているような気がする。
	// とりあえず固定フォーマット読み取りを実装する。
	// ジオメトリカード2 のフォーマットは(2X, A3, 1X, I4, 6E10.3)

	// ここでは継続行連結済みとするので70文字より短い場合はエラーと言いたいが
	// streamが末尾の空白を削っている可能性があるので 61文字くらいの場合は存在し得る
	// 実際にはENDカードが最短で5文字の場合がある
	if(buff.size() < 5) {
		throw std::invalid_argument("Too short CGB card. input =\"" + str + "\"");
	}
	// 2X部分
	if(buff.substr(0, 2) != "  ") {
		throw std::invalid_argument("CGB card should start with 2 spaces \"  \"."
									" actual =\"" + buff.substr(0, 2) + "\"");
	}
	buff = buff.substr(2);  // 2X ぶんのカット

	// A3取得
	std::string itypeStr = buff.substr(0, 3);
	// itypeに許容されるのは box, rpp, sph, rcc, rec, ell, trc, wed, arb, end
	// 連結処理済みなのでblankは許容されない。
	if(!std::regex_search(itypeStr, std::regex("box|rpp|sph|rcc|rec|ell|trc|wed|arb|end", std::regex_constants::icase))) {
		throw std::invalid_argument("CGB card mnemonic should be box/rpp/sph/rcc/rec/ell/trc/wed/arb/end."
									" actual=" + itypeStr);
	}
	buff = buff.substr(3);  // A3カット
	// ENDカードならパラメータ等は存在しないのでここでリターンする
	if(std::regex_search(itypeStr, std::regex("end", std::regex_constants::icase))) {
		return CGBody(itypeStr, 0, std::vector<double>());
	}


	// 1X部分
	if(buff.front() != ' ') {
		throw std::invalid_argument("Mnemonic of CG and number should be separated with a space. input =" + str);
	}
	buff = buff.substr(1);  // 1Xカット

	// I4部分
	std::string ialpStr = buff.substr(0, 4);
	int ialpNum = 0;
	if(ialpStr.find_first_not_of(" ") == std::string::npos) {
		ialpNum = CGBody::getUniqueID();
	} else {
		try {
			ialpNum = utils::stringTo<int>(ialpStr);
		} catch (std::exception &e) {
            (void) e;
			throw std::invalid_argument("IALP input should be a number. input = " + ialpStr);
		}
	}
	buff = buff.substr(4);

	// 6E10.3部分
	//  ところで継続行のフォーマットはどうなるのだろうか？ E10.3が任意の数続くでよいのか
	// → マニュアルの表3.2に11文字目から開始でE10.3相当が必要数続くことが示されている。

	// とりあえず真面目に固定フォーマットで読み取る。
	std::string fpdStr;
	double fpdValue;
	std::vector<double> fpdVec;
	while(!buff.empty()) {
		size_t length = (std::min)(CGB_DATA_WIDTH, buff.size());
		fpdStr = buff.substr(0, length);
		buff = buff.substr(length);
		//mDebug() << "fpdStr ===" << fpdStr;
		try {
			if(fpdStr.find_first_not_of(" ") == std::string::npos) {
				fpdValue = 0;
			} else {
				fpdValue = utils::stringTo<double>(fpdStr);
			}
		} catch (std::exception &e) {
            (void) e;
			throw std::invalid_argument("FPD parameter should be a number, param = \"" + fpdStr + "\", whole input = " + str);
		}
		fpdVec.emplace_back(fpdValue);
	}
	if(fpdVec.size() < 4) {
		throw std::invalid_argument("Number of CG Body parameters (FPD) shoudl be >= 4. params = " + buff);
	}

	return CGBody(itypeStr, ialpNum, fpdVec);
}

