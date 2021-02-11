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

#include <functional>
#include <regex>
#include <sstream>
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "cgconversionmap.hpp"

//// CGbodyのmnemonic(string)とパラメータ(vector<double>)を受け取って、
//// MCNPのサーフェイスカードで読めるカード入力文字列に変換する関数の型
//using convfunc_type = std::function<std::string(int, const std::string&, const std::vector<double>&)>;
//// convfunc_type にパラメータ数チェック用の引数(第一引数)を追加した型。
//// この第一引数にbindで数値をあててconvfunc_typeのマップへ代入する。
//using convproto_type = std::function<std::string(size_t, int, const std::string&, const std::vector<double>&)>;
namespace {

constexpr size_t CGB_DATA_WIDTH = 10;





}  // end anonymous namespace





int inp::CGBody::seqID_ = 0;
std::unordered_set<int> inp::CGBody::usedIDs_;

bool inp::CGBody::isEnd() const
{
	return std::regex_search(mnemonic_, std::regex("end", std::regex_constants::icase));
}

std::string inp::CGBody::toInputString() const
{
	return getConvFuncMap().at(mnemonic_)(id_, mnemonic_, parameters_);
}

bool inp::CGBody::isEndString(const std::string &str)
{
	auto arg = utils::trimmed(str);
	utils::tolower(&arg);
	return arg == "end";
}

int inp::CGBody::getUniqueID()
{
	while(!(usedIDs_.insert(++seqID_)).second) {;}
	return seqID_;
}

void inp::CGBody::initUniqueID()
{
	seqID_ = 0;
	usedIDs_.clear();
}

bool inp::CGBody::isContData(const std::string &str)
{
	if(str.size() < 6) {
		return false;
	} else {
		return str.substr(0, 6).find_first_not_of(" ") == std::string::npos;
	}
}

// 連結処理済み文字列からCGBodyを作成。
inp::CGBody inp::CGBody::fromQadFixedString(const std::string &str)
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
	static std::regex mnemonicReg("box|rpp|sph|rcc|rec|ell|trc|wed|arb|end|bpp|wpp|gel|qua|tor|px|py|pz|ps|p|c|cx|cy|cz",
								  std::regex_constants::icase);
	if(!std::regex_search(itypeStr, mnemonicReg)) {
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

#include "core/utils/system_utils.hpp"
#include "core/utils/message.hpp"

std::vector<inp::CGBody> inp::CGBody::getCgBodiesFromQadFixed(std::stringstream &ss, int &lineNumber)
{
	CGBody::initUniqueID();
	std::vector<inp::CGBody> cgbodies;
	std::string currentLine, nextLine;
	getline(ss, currentLine);
	utils::sanitizeCR(&currentLine);
	++lineNumber;

	while(!CGBody::isEndString(currentLine)) {
		while(getline(ss, nextLine), ++lineNumber, utils::sanitizeCR(&nextLine), CGBody::isContData(nextLine)) {
			/*
			 *  currentLineは連結する前にケツをtrimする。
			 *  入力のフォーマットがただしければそのような必要はないが、スペースが行末に余分に入っている
			 *  場合くらいは対処しないと使いづらすぎる。
			 *  行連結が起こるのは currentLineの文字数が 10+60nの時のみなのであまり部分を削除する。
			 */
			if(currentLine.size() > 70) {
				size_t numExtraChars = (currentLine.size()-10)%60;
//				std::string extraString = currentLine.substr(currentLine.size()-numExtraChars);
				if(numExtraChars != 0) {
//					// 余分な文字列に空白以外の文字があったら警告する。
//					if(extraString.find_first_not_of(" ") != std::string::npos) {
//						mWarning(inputFile_ + ":" + std::to_string(toRealLineNumber(lineNumber)))
//								<< "Extra string was omitted. extra data ="
//								<< "\"" + extraString + "\"";
//					}
					//mDebug() << "before ===" << "\"" + currentLine + "\"";
					currentLine = currentLine.substr(0, currentLine.size()-numExtraChars);
					//mDebug() << "after  ===" <<  "\"" +currentLine + "\"";
				}
			}

			currentLine += nextLine.substr(10);
			if(ss.eof()) throw std::invalid_argument("Unexpected EOF while reading CGB card.");
		}
		// ここの時点でnextLineには継続行ではない次の行が入る。
		try {
			auto tmpCgb = CGBody::fromQadFixedString(currentLine);
			assert(!tmpCgb.isEnd());  // currentLineが"END"の状態でここへ来ることは無いはず。
			cgbodies.emplace_back(std::move(tmpCgb));
		} catch (std::exception &e) {
			throw std::invalid_argument(std::string("CGB error, what = ") + e.what());
		}
		currentLine.swap(nextLine);
	}
	return cgbodies;
}



