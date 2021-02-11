#include "xsdir.hpp"


#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "utils/string_util.hpp"

ace::XsDir::XsDir(const std::string &xsdirFilename,
				  const std::vector<ace::NTY> &targetNtyVec,
				  bool readXsInfo)
	:filename_(xsdirFilename)
{
//	std::cout << "Enter xsdir constructor, filename====" << xsdirFilename << ", size of ntyvec=" << targetNtyVec.size() << std::endl;
//	for(auto nty: targetNtyVec) {
//		std::cout << "nty===" << static_cast<int>(nty) << std::endl;
//	}

	// xsdirはせいぜい600kBくらいなので、多少読み込み方法を変えてもあまり差はでない。
	if(filename_.empty()) return;
	std::ifstream ifs(utils::toEncodedString(filename_).c_str());
	if(ifs.fail()) {
        throw std::invalid_argument(std::string("No such a file == ") + filename_);
	}

	std::string buff;
	getline(ifs, buff);
	// XSDIRは大文字/小文字を区別する。ので小文字化しない
    // 1行目はDATAPATHの場合がある。そうでなければ環境変数を参照する
	std::smatch sm;
	if(std::regex_search(buff, sm, std::regex(R"(^ *datapath *=*)", std::regex_constants::icase))) {
		datapath_ = std::string(sm[0].second, buff.cend());
		getline(ifs, buff);
    } else if(std::getenv("DATAPATH")){
        // ここは内部表現utf8に統一する。
        datapath_ = utils::toUTF8(std::string(std::getenv("DATAPATH")));
	}


	// ファイル名の中間の空白は残すが両端の空白は削除する。
    auto left = datapath_.find_first_not_of(" \t\v");
    if(left != std::string::npos) {
        datapath_ = datapath_.substr(left);
        auto right = datapath_.find_last_not_of(" \t\v");
        if(right != std::string::npos) datapath_ =datapath_.substr(0, right+1);
    }
//    // NOTE ↓のtrimはUTF8ではwinではマルチバイト非対応
//	datapath_.erase(datapath_.begin(),
//					std::find_if(datapath_.begin(), datapath_.end(),
//								 [](int c){ return !std::isspace(c);})
//					);
//	datapath_.erase(std::find_if(datapath_.rbegin(), datapath_.rend(),
//					[](int c){ return !std::isspace(c);}).base(),
//					datapath_.end());

	// ここは必ず"atomic weight ratios"
	if(!std::regex_search(buff, sm, std::regex(R"(^ *atomic weight ratios *$)", std::regex_constants::icase))) {
		throw std::invalid_argument(std::string("xsdir should start with \"atomic weight ratios\", actual=\"") + buff + "\".");
	}

	// ######### ここからAWR読み取り
	std::string zaidStr;
	double awr;
    int c=0;
	while(true) {
        ++c;
		ifs >> zaidStr;
		//  awrテーブルが終わったら 日付か"directory"が読み込まれる。
        auto pos = zaidStr.find_first_not_of("0123456789.");
        if(pos != std::string::npos) {
            break;
		}
		ifs >> awr;
		awrMap_.emplace(zaidStr, awr);
	}

	// 特に日付を入れる規程は見当たらなかったのでdirectoryが出るまで進める
	std::regex directory(R"(^ *directory *$)", std::regex_constants::icase);
	do {
		if(std::regex_search(buff, sm, directory)) break;
	} while(getline(ifs, buff));
	// directory発見前にEOFでwhileを抜けている可能性があるのでeofチェック
	if(ifs.eof()) {
		throw std::invalid_argument("Unexpected EOF before \"directory\"");
	}

	// 第三引数がfalseならAWRだけ読んでaceファイル情報は読まずにリターンする
	if(!readXsInfo) return;


	std::vector<std::regex> ntyRegexVec;
	for(auto &nty: targetNtyVec) {

        std::string regStr = std::string("\\.") + "\\d{2,3}" + ace::getClassRegexStr(nty);
        ntyRegexVec.emplace_back(regStr);

	}


	// ######### ここからファイル情報読み取り
	//                                 id    awr    file  route   type  addr   tablen  reclen  nent  tmp
	std::regex  longDirPattern(R"(^ *(\S+) +(\S+) +(\S+) +(\S+) +(\d+) +(\d+) +(\d+) +(\d+) +(\d+) (\S+))");
	std::regex shortDirPattern(R"(^ *(\S+) +(\S+) +(\S+) +(\S+) +(\d+) +(\d+) +(\d+))");
	while(getline(ifs, buff)) {
		//std::transform(buff.begin(), buff.end(), buff.begin(), ::tolower);
		XsInfo xsinfo;
		if(std::regex_search(buff, sm, longDirPattern)) {
//			for(size_t i = 0; i < sm.size(); ++i) {
//				std::cout << "i=" << i << "sm=" << sm.str(i) << std::endl;
//			}
			xsinfo.tableID = sm.str(1);
			xsinfo.awr = std::stod(sm.str(2));
			xsinfo.filename = sm.str(3);
			xsinfo.accessRoute = sm.str(4);
			xsinfo.filetype = std::stoi(sm.str(5));
			xsinfo.address = std::stoi(sm.str(6));
			xsinfo.tableLength = std::stoi(sm.str(7));
			xsinfo.recordLength = std::stoi(sm.str(8));
			xsinfo.entriesPerRecord = std::stoi(sm.str(9));
			xsinfo.temperature = std::stod(sm.str(9));
			if(buff.find("ptable") != std::string::npos) xsinfo.hasPtable = true;
		} else if (std::regex_search(buff, sm, shortDirPattern)) {
			xsinfo.tableID = sm.str(1);
			xsinfo.awr = std::stod(sm.str(2));
			xsinfo.filename = sm.str(3);
			xsinfo.accessRoute = sm.str(4);
			xsinfo.filetype = std::stoi(sm.str(5));
			xsinfo.address = std::stoi(sm.str(6));
			xsinfo.tableLength = std::stoi(sm.str(7));
			xsinfo.recordLength = 0;
			xsinfo.entriesPerRecord = 0;
			xsinfo.temperature = 0;
			xsinfo.hasPtable = false;
        } else if (buff.find_first_not_of(" \t") == std::string::npos) {
            continue;
        } else {
			throw std::invalid_argument(std::string("In file ") + filename_
                                        + " \"" + buff + "\" is not a valid table info.");
		}
		// ここで対象ntyでなければ保存しない

		if(targetNtyVec.empty()) {
			registerXsInfo(xsinfo, &xsInfoMap_);
		} else {
			for(auto &ntyRegex: ntyRegexVec) {
				if(std::regex_search(xsinfo.tableID, ntyRegex)) {
					registerXsInfo(xsinfo, &xsInfoMap_);
					break;
				}
			}
		}
	}
}


void ace::XsDir::registerXsInfo(const ace::XsInfo &xsinfo,
                                std::unordered_map<std::string, std::vector<ace::XsInfo> > *infoMap)
{
	std::string zaid = xsinfo.tableID.substr(0, xsinfo.tableID.find_first_of("."));
	if(infoMap->find(zaid) == infoMap->end()) {
		(*infoMap)[zaid] = std::vector<XsInfo>{xsinfo};
	} else {
		infoMap->at(zaid).emplace_back(xsinfo);
	}
}

ace::XsInfo ace::XsDir::getNuclideInfo(const std::string &id, NTY nty) const
{
	try{
		std::string::size_type dotPos = id.find_first_of(".");
		if(dotPos == std::string::npos && !xsInfoMap_.at(id).empty()) {
			std::string regexStr =std::string("\\b") + id + "\\." + "\\d{2,3}" + ace::getClassRegexStr(nty);
			std::regex idPattern(regexStr);
		// idに拡張子が無い場合zaidのみなのでmapからzaidをキーにして最初にntyの適合するものを返す
			for(auto &xs: xsInfoMap_.at(id)) {
				if(std::regex_search(xs.tableID, idPattern)) return xs;
			}
		} else {
			std::string zaid = id.substr(0, dotPos);
			for(auto &xs: xsInfoMap_.at(zaid)) {
				if(xs.tableID == id) return xs;
			}
		}
	} catch (std::exception &e) {
        (void) e;
	}
	throw std::out_of_range("Material id = " + id + " for " + ntyToString(nty)  + " not found in xsdir.");
}


std::string ace::XsInfo::toString() const {
	std::stringstream ss;
	ss << "id=" << tableID << ", awr=" << awr << ", file=" << filename << ", route=" << accessRoute
		<< ", type=" << filetype << ", address=" << address << ", tablength=" << tableLength
		<< ", reclength=" << recordLength << ", entries per rec=" << entriesPerRecord
		<< ", temperature=" << temperature << std::boolalpha << ", has ptable=" << hasPtable;
	return ss.str();
}
