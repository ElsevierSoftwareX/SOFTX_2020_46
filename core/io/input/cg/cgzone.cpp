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
#include "cgzone.hpp"

#include <algorithm>
#include <regex>
#include <stdexcept>

#include "core/utils/string_utils.hpp"
#include "core/utils/system_utils.hpp"

int inp::CGZone::seqID_ = 0;
std::unordered_set<int> inp::CGZone::usedIDs_;

bool inp::CGZone::isContData(const std::string &str)
{
	if(str.size() < 5) {
		return false;
	} else {
		return str.substr(2, 3).find_first_not_of(" ") == std::string::npos;
	}
}

bool inp::CGZone::isEndString(const std::string &str)
{
	std::string arg = utils::trimmed(str);
	utils::tolower(&arg);
	return (arg == "end" || arg == "999");
}

void inp::CGZone::initUniqueID()
{
	seqID_ = 0;
	usedIDs_.clear();
}

int inp::CGZone::getUniqueID()
{
	while(!(usedIDs_.insert(++seqID_)).second) {;}
	return seqID_;
}

bool inp::CGZone::isEnd() const
{
	return std::regex_search(ialp_, std::regex("end", std::regex_constants::icase));
}

std::string inp::CGZone::toString() const
{
	std::stringstream ss;
	ss << ialp_ << " ";
	for(size_t i = 0; i < jtyPairs_.size(); ++i) {
		ss << (jtyPairs_.at(i).first ? "OR" : "  ") << jtyPairs_.at(i).second;
	}
	return ss.str();
}

std::string inp::CGZone::equation() const
{
	std::stringstream ss;
	// 要は最初のOR以外をOR演算子に置き換えればいいのではないか？
	bool noOROpsBefore = true;
	for(size_t i = 0; i < jtyPairs_.size(); ++i) {
		if(jtyPairs_.at(i).first) {
			// OR演算子が出現した場合
			if(!noOROpsBefore) {
				ss << ":";
			} else {
				// 最初にORが出てきた場合
				noOROpsBefore = false;
			}
		} else {
			// AND演算の場合
			ss << " ";
		}
		ss << -1*jtyPairs_.at(i).second;
	}
	return ss.str();
}


namespace {
// 継続行連結済み文字列データからCGZoneを作成する。
// 書式は (2X,A3,I5,9(A2,I5))
inp::CGZone fromQadFixedString(const std::string &str)
{
	std::string buff = str;
	if(buff.size() < 5) {
		throw std::invalid_argument("Too short CGC card. input = " + str);
	} else if (buff.substr(0, 2) != "  ") {
		throw std::invalid_argument("CGC card should start with 2 spaces. input = " + str);
	}
	buff = buff.substr(2);

	// IALP
	std::string ialpStr = buff.substr(0, 3);
	if(std::regex_search(ialpStr, std::regex("end", std::regex_constants::icase))) {
		return inp::CGZone(ialpStr, 0, std::vector<std::pair<bool, int>>());
	}
	buff = buff.substr(3);

	// NAZ
	if(buff.size() < 5) throw std::invalid_argument("Too short CGC card. NAZ entry is absent. input = " + str);
	std::string nazStr = buff.substr(0, 5);
	int nazValue;
	if(nazStr.find_first_not_of(" ") == std::string::npos) {
		nazValue = 0;
	} else {
		nazValue = utils::stringTo<int>(nazStr);
	}
	buff = buff.substr(5);

	// IIBLASとJTY
	// とりあえず固定フォーマット実装
	constexpr size_t IIBLAS_WIDTH = 2;
	constexpr size_t JTY_WIDTH = 5;
	std::string iiblasStr, jtyStr;
	std::vector<std::pair<bool, int>> iiBlasJtyPairs;
	while(!buff.empty()) {
		// 空白しか含まれなくなった場合もbreakする。
		if(buff.find_first_not_of(" ") == std::string::npos) break;

		// iiblas取得
		if(buff.size() < IIBLAS_WIDTH) {
			throw std::invalid_argument("Too shord CGC card IIBLAS is required. input = " + str);
		}
		iiblasStr = buff.substr(0, IIBLAS_WIDTH);
		buff = buff.substr(IIBLAS_WIDTH);
		if(iiblasStr != "  " && !std::regex_search(iiblasStr, std::regex("or", std::regex_constants::icase))) {
			throw std::invalid_argument("IIBLAS in CGC card should be spaces or \"OR\". actual = " + iiblasStr);
		}

		// jty 取得
		size_t jtyLen = (std::min)(JTY_WIDTH, buff.size());
		if(jtyLen == 0) throw std::invalid_argument("Empty JTY entry.");
		jtyStr = buff.substr(0, jtyLen);
		buff = buff.substr(jtyLen);
		iiBlasJtyPairs.emplace_back(iiblasStr == "  " ? false: true, utils::stringTo<int>(jtyStr));
	}
	if(iiBlasJtyPairs.empty()) {
		throw std::invalid_argument("Too short CGC card. IIBLAS and JTY are empty. input = " + str);
	}
	return inp::CGZone(ialpStr, nazValue, iiBlasJtyPairs);
}
// 継続行連結済み文字列データからCGZoneを作成する。
// 書式よくわからないが (2X,A3,I5,9(A2,I5))としてA3が材料名
inp::CGZone fromMarsFixedString(const std::string &str)
{
	// 書式は同じとする。ただしialpが材料名となる。
	return fromQadFixedString(str);
}

enum class CGFORMAT {QAD, MARS};
std::vector<inp::CGZone> getCgZones(std::stringstream &ss, int &lineNumber, CGFORMAT format)
{
	inp::CGZone::initUniqueID();
	std::vector<inp::CGZone> cgzones;
	std::string currentLine, nextLine;
	getline(ss, currentLine);
	while(!inp::CGZone::isEndString(currentLine)) {
		//int currentLineNumber = lineNumber;
		while(getline(ss, nextLine), ++lineNumber, utils::sanitizeCR(&nextLine), inp::CGZone::isContData(nextLine)) {
			std::string::size_type pos = currentLine.find_last_not_of(" \t");
			currentLine = currentLine.substr(0, pos+1);
			currentLine += nextLine.substr(10);
			if(ss.eof()) throw std::invalid_argument("Unexpected EOF while reading CGC card.");
		}
		// FIXME MARSの書式が良くわからない2X3Aかと思ったが 1X4Aかもしれない。
		try {
			inp::CGZone tmpCgc;
			if(format == CGFORMAT::QAD) {
				tmpCgc = fromQadFixedString(currentLine);
			} else if (format == CGFORMAT::MARS) {
				tmpCgc = fromMarsFixedString(currentLine);
			} else {
				throw std::invalid_argument("ProgramError: invalid CG Format");
			}
			assert(!tmpCgc.isEnd());
			cgzones.emplace_back(std::move(tmpCgc));
		} catch (std::exception &e) {
			throw std::invalid_argument(std::string("CGC error, ") + e.what());
		}
		currentLine.swap(nextLine);
	}
	return cgzones;
}

}  // end anonymous namespace


std::vector<inp::CGZone> inp::CGZone::getCgZonesFromQadFixed(std::stringstream &ss, int &lineNumber)
{
	return getCgZones(ss, lineNumber, CGFORMAT::QAD);
}

std::vector<inp::CGZone> inp::CGZone::getCgZonesFromMarsFixed(std::stringstream &ss, int &lineNumber)
{
	return getCgZones(ss, lineNumber, CGFORMAT::MARS);
}



