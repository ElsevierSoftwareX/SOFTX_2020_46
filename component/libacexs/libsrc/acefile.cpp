#include "acefile.hpp"


#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_map>
#include "utils/string_util.hpp"
#include "utils/utils_vector.hpp"
#include "aceutils.hpp"
#include "mt.hpp"
#include "neutrondosimetryfile.hpp"
#include "neutrontransportfile.hpp"
#include "photoatomicfile.hpp"




namespace {
const int VER1_HEADER_SIZE = 6;
const int NUM_HEADER_LINE = 4;
const int VER1_COMMENT_LINE = 2;

// ZAID, SZAX両方に該当するregex
// match 1:ZA, 2:identifier, 3:class
const std::regex ZAID_PATTERN(R"(([0-9]+)\.([0-9]{2,3})(\w+))");
}

//  NOTE 正規表現でらくらくに書き直せる
bool ace::isZAIDX(const std::string &str)
{
	// ZAID は nnmm.ll 判定基準は
	// 1.ピリオドを1個だけ含むこと
	// 2.ピリオド以前部分は数字で6桁以下2桁以上
	// 3.ピリオド以降部分の最後の文字はASCII

	// 条件1
	std::string::size_type first_period_pos = str.find_first_of(".");
	std::string::size_type last_period_pos = str.find_last_of(".");
	if(first_period_pos != last_period_pos || first_period_pos == std::string::npos) return false;
	// 条件2
	std::string former_part = str.substr(0, first_period_pos);
	if(former_part.size() > 7
			|| former_part.size() < 1
			|| former_part.find_first_not_of("0123456789") != std::string::npos ) return false;

	// 条件3
	std::string last_str = str.substr(str.size() - 1, 1);
	if(last_str.find_first_of("0123456789") != std::string::npos ) return false;

	return true;
}

// SZAX= SSSZZZAAA.dddCC, s:励起状態, z:原子番号, a:質量数, d:ライブラリ識別子, c:ライブラリクラス
// 例：1027058.710nc = 励起状態のCo-58 ENDF/B-VII連続エネルギー中性子ライブラリ
bool ace::isSZAX(const std::string &str)
{
	/*
	 * 判定基準は
	 * 1．ピリオドを1個含むこと
	 * 2．拡張子の最初は1〜3文字の数字
	 * 3．拡張子の最後の2文字はアルファベット
	 * 3．ピリオド以前部分は数字で4桁以上
	 */
	// 条件1
	std::string::size_type first_period_pos = str.find_first_of(".");
	std::string::size_type last_period_pos = str.find_last_of(".");
	if(first_period_pos != last_period_pos || first_period_pos == std::string::npos) return false;

	std::string suffix = str.substr(first_period_pos+1);
	std::string suffixNumberPart = suffix.substr(0, suffix.size() - 2);
	std::string suffixCharPart = suffix.substr(suffix.size() - 2, 2);
	//std::cout << "SZAX suffix=" << suffix << ", number=" << suffixNumberPart << ", char=" << suffixCharPart << std::endl;
	// 条件2
	if(suffixNumberPart.find_first_not_of("0123456789") != std::string::npos
			|| suffixNumberPart.size() > 3 || suffixNumberPart.size() < 1) return false;
	// 条件3
	if(suffixCharPart.find_first_of("0123456789") != std::string::npos ) return false;

	return true;
}




// このコンストラクタはifsからidに該当する核種部分、(idが空なら最初の核種)
// をbuffer_に読み込む
// JXS, NXSはXSSには含まれない。
ace::AceFile::AceFile(std::ifstream &ifs, const std::string id, std::size_t startline)
{
	bool zaidFlag = isZAIDX(id), szaxFlag = isSZAX(id);
    //std::cout << "constructing AceFile for id===" << id << ", isZaid=" << zaidFlag << ", isSZAX=" << szaxFlag << std::endl;
	seek(ifs, id, startline);

    getAceHeader(ifs);
	nxs_ = getNXS(ifs);
	jxs_ = getJXS(ifs);

    std::string tmp;
    std::stringstream ss;
	while(true) {
		getline(ifs, tmp);
		if (ifs.eof()) {
			break;
		} else if(zaidFlag && utils::GetDataBlock(0, tmp) == id) {
			break;
		} else if(szaxFlag && utils::GetDataBlock(1, tmp) == id) {
			break;
		}
		ss << tmp;
	}

	while(ss >> tmp) xss_.push_back(tmp);
}

void ace::AceFile::dump() {
	std::cout << "Dump xs data in " << ID_ << std::endl;
	this->DumpNXS(std::cout);
	this->DumpJXS(std::cout);
	for( MTmap_iterator it=XSmap_.begin(); it!=XSmap_.end(); it++){
		(it->second).dump() ;
	}
}

// reactionの断面積を返す。
const ace::CrossSection &ace::AceFile::getCrossSection(ace::Reaction reaction) const
{
	if(XSmap_.find(reaction) == XSmap_.end()) {
		throw std::out_of_range(std::string("No cross section data for reaction MT=")
								+ std::to_string(ace::mt::toNumber(reaction)));
	} else {
		return XSmap_.at(reaction);
	}

}

void ace::AceFile::getAceHeader(std::ifstream &is)
{
	std::string param;
    is >> param;
//    getline(is, param);
//    std::cout << "param line===" << param;
//    std::stringstream ss;
//    ss << param;
//    ss >> param;


    //std::cout << "Header first param=" << param << std::endl;
	double aceVersion = 0.0;
	// param, 即ちACEファイル最初のパラメータが
	// ZAIDならversion1 ヘッダなので残りのヘッダ行は残り5行
	// 数値で2.0ならversion2ヘッダなのでヘッダ行数は可変
	if(isZAIDX(param)) {
		aceVersion = 1.0;
		ID_ = param;
		//std::cout << "Version 1.0 ACE format detected."  << std::endl;;
	} else if(param.find_first_not_of("0123456789.,+-Ee") == std::string::npos) {
		aceVersion = std::stod(param);
		if(aceVersion >= 2.0) {
			is >> param;
			if(isSZAX(param)) {
				ID_ = param;
				//std::cout << "Version " << aceVersion << " ACE format detected."  << std::endl;;
			} else {
                std::cerr << "Invalid header parameter == " << param;
				std::exit(EXIT_FAILURE);
			}
		}
	} else {
		std::cerr << "Invalid header parameter = " << param;
		std::exit(EXIT_FAILURE);
	}

	int headerSize = 0;
	int num_comment_line = 0;
	std::string dummy;
	switch (static_cast<int>(aceVersion)) {
	case 1:
		num_comment_line = VER1_COMMENT_LINE;
		break;
	case 2:
		// ACE ver2 ではヘッダサイズは可変で7ブロック目にコメント行数が書かれている。
		// version判定のために既に2個読み込んだからあと5個データを取得するとそこが行数
		for(int i = 0; i < 4; i++) {
			is >> dummy;
		}
		is >> num_comment_line;
		// コメント行数の後に改行が入るのでgetlineしておく
		getline(is, dummy);
		break;
	default:
		std::cerr << "Sorry, Ace version = " << aceVersion << "is not implmented yet." << std::endl;
		std::exit(EXIT_FAILURE);
	}

	headerSize = num_comment_line + NUM_HEADER_LINE;
	std::string buff;
	for(int i = 0; i < headerSize; i++) {
		getline(is, buff);
		//std::cout << buff << std::endl;
	}
}


// https://stackoverflow.com/questions/15723778/ifstreamtellg-differs-on-msvc2012-and-gcc-mingw-when-file-has-trailing-newli
// https://stackoverflow.com/questions/9817806/why-does-my-program-produce-different-results-on-windows-and-linux-about-file-r
void ace::AceFile::seek(std::ifstream &ifs, const std::string &id, size_t startline)
{
	bool zaidFlag = isZAIDX(id), szaxFlag = isSZAX(id);
	//	std::cout << "zaid flag =" << zaidFlag << ", szax flag = " << szaxFlag << std::endl;;
	//ID(ZAIDXかSZAX) が指定されている場合目的の核種の位置を探す
	if((!zaidFlag && !szaxFlag) || id.empty()) return;


	// xsdir等で開始行が分かっている場合そこまで飛ばす
    //std::cout << "startline===" << startline << std::endl;
	if(startline != 0) {
		std::string dummy;
		for(size_t i = 0; i < startline-1; ++i) {
			getline(ifs, dummy);
			if (ifs.eof()) {
				std::cerr << "Unexpected EOF while seeking startline, nuclide ID = " << id << std::endl;
				std::exit(EXIT_FAILURE);
			}
		}
	}

	std::ifstream::pos_type pos;
	std::string buff;
	while(true) {
		pos = ifs.tellg();  // ifsがasciiだとコンパイラによってposの位置が変わってしまう
		getline(ifs, buff);
        //std::cout << "buff===" << buff << std::endl;
		if (ifs.eof()) {
            std::stringstream ss;
            // aceファイル読み取り中のエラー表明にはファイル名があった方がよさそう。
            ss <<  "Unexpected EOF while seeking nuclide data, ZAIDX = " << id;
            throw std::invalid_argument(ss.str());
		}
		// ZAID指定の場合最初のパラメータがZAIDの所がデータの始まりとなる
		if(zaidFlag && utils::GetDataBlock(0, buff) == id) {
			std::cout << "ZAIDX = " << id << " found. pos===" << pos << std::endl;
            ifs.seekg(pos);
			break;
			// SZAX指定の version szax sourceとなっている行が先頭
		} else if(szaxFlag && utils::GetDataBlock(1, buff) == id) {
			std::cout << "SZAX = " << id << " found." << std::endl;
			ifs.seekg(pos);
			break;
		}

	}
}


bool ace::AceFile::checkEndOfData(std::ifstream &ifs, const bool& outputResidualData)
{
	// ファイルの最後に到達しているかチェック。
	// monolithicなファイルの場合ここでeofにならないので次のデータがzaidかどうかで判断する。
	std::string eofbuff;
	ifs >> eofbuff; // ファイル最後尾で読み取ろうとするとeofbuffはemptyでifsにeofがセットされる。
	// eofbuffの最初のデータがZAIDなら正常終了
	if(isZAIDX(utils::GetDataBlock(0, eofbuff))) {
		std::cout << "End target nuclide data." << std::endl;
		std::cout << "(Next data block = " << utils::GetDataBlock(0, eofbuff) << ")" << std::endl;
		return true;
	} else if(!ifs.eof()) {
		if(outputResidualData) {
			while(true) {
				std::cerr << "Ace file includes additional data =" << eofbuff << std::endl;
				ifs >> eofbuff;
				if(ifs.eof()) break;
			}
		}
		return false;
	} else {
		return true;
	}
}



std::vector<long> ace::AceFile::getNXS(std::istream &is)
{
	return ace::getData<long int>(NXS_SIZE, is);
}

std::vector<long> ace::AceFile::getJXS(std::istream &is)
{
	return ace::getData<long int>(JXS_SIZE, is);
}


std::unique_ptr<ace::AceFile> ace::AceFile::createAceFile(const std::string &filename,
														  const std::string &zaidx,
														  std::size_t startline)
{
    // asciiモードの場合、mingwでのifs.posが正しい値を返さない。
    std::ifstream ifs(utils::toEncodedString(filename).c_str(), std::ios::binary);
	if(ifs.fail()) {
		throw std::invalid_argument(std::string("No such a file = ") + filename);
	}
	//std::smatch sm;
	std::string classStr = getClassStr(zaidx);
	if(classStr.empty()) throw std::invalid_argument(std::string("No classe found in zaidx = ") + zaidx);

	ace::NTY nty = classStrToNty(classStr);
	std::unique_ptr<ace::AceFile> aceFile;
	switch(nty) {
	case ace::NTY::CONTINUOUS_NEUTRON:
		aceFile.reset(new ace::NeutronTransportFile(ifs, zaidx, startline));
		break;
	case ace::NTY::DOSIMETRY:
		aceFile.reset(new ace::NeutronDosimetryFile(ifs, zaidx, startline));
		break;
	case ace::NTY::CONTIUNOUS_PHOTOATOMIC:
		aceFile.reset(new ace::PhotoatomicAceFile(ifs, zaidx, startline));
		break;
	case ace::NTY::PHOTONUCLEAR:
		std::cerr << "ProgramError: photonuclear file is not implemented yet." << std::endl;
		abort();
	default:
		break;
		std::cerr << "ProgramError:not implemented acefile.";
		abort();
	}
	return aceFile;
}

ace::CrossSection::CrossSection():mt(ace::Reaction::NOT_DEFINED), release_n(0), Qval(0.0), e_offset(0), angular_flag(0) {;}



// ACEフォーマット ver1 | ver2のclass 検索用正規表現を返す。
std::string ace::getClassRegexStr(ace::NTY nty)
{
	static const std::map<ace::NTY, std::string> regexStrTable{
		{ace::NTY::CONTINUOUS_NEUTRON, "(c|nc)"},
		{ace::NTY::MULTIGROUP_NEUTRON, "(m|nm)"},
		{ace::NTY::DISCRETE_NEUTRON, "(d|nd)"},
		{ace::NTY::DOSIMETRY, "(y|ny)"},
		{ace::NTY::THERMAL, "(t|nt)"},
		{ace::NTY::CONTIUNOUS_PHOTOATOMIC, "(p|pp)"},
		{ace::NTY::PHOTONUCLEAR, "(u|pu)"}
	};
	return regexStrTable.at(nty);
}

ace::NTY ace::classStrToNty(const std::string &str)
{
	static std::unordered_map<std::string, NTY> strNtyMap {
		{"c", NTY::CONTINUOUS_NEUTRON},
		{"m", NTY::MULTIGROUP_NEUTRON},
		{"d", NTY::DISCRETE_NEUTRON},
		{"y", NTY::DOSIMETRY},
		{"p", NTY::CONTIUNOUS_PHOTOATOMIC},
		{"u", NTY::PHOTONUCLEAR},
		{"nc", NTY::CONTINUOUS_NEUTRON},
		{"nm", NTY::MULTIGROUP_NEUTRON},
		{"nd", NTY::DISCRETE_NEUTRON},
		{"ny", NTY::DOSIMETRY},
		{"pp", NTY::CONTIUNOUS_PHOTOATOMIC},
		{"pu", NTY::PHOTONUCLEAR}
	};
	if(strNtyMap.find(str) == strNtyMap.end()) {
		throw std::out_of_range(std::string("\"") + str + "\" is not valid ace class string");
	}
	return strNtyMap.at(str);
}

std::string ace::getClassStr(const std::string &zaidxStr)
{
	std::smatch sm;
	if(std::regex_search(zaidxStr, sm, ZAID_PATTERN)) {
		return sm.str(3);
	} else {
		return "";
	}
}

std::string ace::getZaStr(const std::string &zaidxStr)
{
	std::smatch sm;
	if(std::regex_search(zaidxStr, sm, ZAID_PATTERN)) {
		return sm.str(1);
	} else {
		return "";
	}
}

std::string ace::ntyToString(ace::NTY nty)
{
	static const std::map<ace::NTY, std::string> strTable{
		{ace::NTY::CONTINUOUS_NEUTRON, "continous-neutron"},
		{ace::NTY::MULTIGROUP_NEUTRON, "multigroup-neutron"},
		{ace::NTY::DISCRETE_NEUTRON, "discrete-neutron"},
		{ace::NTY::DOSIMETRY, "dosimetry-neutron"},
		{ace::NTY::THERMAL, "thermal-neutron"},
		{ace::NTY::CONTIUNOUS_PHOTOATOMIC, "photo-atomic"},
		{ace::NTY::PHOTONUCLEAR, "photo-nuclear"}
	};
	return strTable.at(nty);
}

ace::NTY ace::getNtyFromZaidx(const std::string &zaidx)
{
	return ace::classStrToNty(getClassStr(zaidx));
}
