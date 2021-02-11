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
#include "inputdata.hpp"

#include <array>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <regex>

#include "mcmode.hpp"
#include "dataline.hpp"
#include "core/io/input/common/commoncards.hpp"
#include "core/io/input/original/original_metacard.hpp"
#include "core/io/input/mcnp/mcnp_metacards.hpp"
#include "core/io/input/phits/phits_metacards.hpp"
#include "core/utils/utils.hpp"
#include "core/utils/system_utils.hpp"
#include "core/formula/fortran/fortnode.hpp"





void inp::InputData::replaceConstants(std::list<inp::DataLine> *inputData)
{
	// 定数表現の数値置換
	std::unordered_map<std::string, std::string> constsDefMap;
//	std::unordered_map<std::string, double> constsDefMap;
	std::unordered_map<std::string, std::regex> constsRegMap;
	std::regex constantNamePattern(R"((\D|^)[cC]\d{1,2}(\D|$))");
	for(auto it = inputData->begin(); it != inputData->end(); ++it) {
		std::smatch sm;
		// set:文を探して定数の更新
		if(std::regex_search(it->data, sm, inp::phits::getSetConstantPattern())) {
			auto argSet = std::string(sm[0].second,it->data.cend());
			//mDebug() << "arg=" << argSet;
			auto itr = argSet.cbegin();
			while(std::regex_search(itr, argSet.cend(), sm, inp::phits::getConstantPattern())) {
				itr = sm[0].second;
				std::string name = sm.str(1);
				std::string definition = sm.str(2);
				//mDebug() << "name=" << name << "def=" << definition;
				if(!std::regex_search(name, sm, constantNamePattern)) {
                    throw std::invalid_argument(it->pos() + " Only c[1-99] is acceptable for set: card, input=" + name);
				}else if(std::regex_search(definition, sm, std::regex(std::string("(\\D|^)") + name + "(\\D|$)"))) {
                    throw std::invalid_argument(it->pos() + " Recursive definition, name=" + name + "definition=" + definition);
				}

				// ここでcontentに定義済みの定数が現れた場合は置換する必要がある。
				for(auto &constsPair: constsDefMap) {
					std::regex constPattern(std::string("(\\D|^)(") + constsPair.first + ")(\\D|$)");
					while(std::regex_search(definition, sm, constPattern)) {
//						mDebug() << "existing constant =" << sm.str(2)
//								 << ", found in def of new one=" << name << ", def=" << definition;
						std::string::size_type pos = std::distance(definition.cbegin(), sm[2].first);
						definition.replace(pos, sm.str(2).size(), std::string("(") + constsPair.second + ")");
					}
				}
				//mDebug() << "constant name=" << name  << "def=" << definition;
				// ここで定数定義にまだ他の定数が残っていたら未定義定数の参照か循環参照が起こっているのでエラー
				if(std::regex_search(definition, sm, constantNamePattern)) {
                    throw std::invalid_argument(it->pos() + " Undefined constant value or cirrular reference,"
                                                + "constName =" + name + ", definition =" + definition);
				}
				// 定数置換後に{}でクオートしなくて良いようにdefinitionからは空白を削除する。
				auto it = std::remove_if(definition.begin(), definition.end(), [](char ch){return ch == ' ';});
				if(it != definition.end()) definition.erase(it, definition.end());
				constsDefMap[name] = definition;  // 定数名をキーにして中身の数式文字列を格納
				constsRegMap[name] = std::regex(std::string("(\\D|^)(") + name + ")(\\D|$)");
			}
			// set文自体は残すと後の他セクションでイレギュラー入力扱いされるので削除する。
			it = inputData->erase(it);
			--it;

		}
		// ここから定数c1-c99を発見次第数値化して置換する。
//		if(std::regex_search(it->data, sm, constantNamePattern)
		if(std::regex_search(it->data, constantNamePattern)
		&& !std::regex_search(it->data, inp::phits::getSetConstantPattern())) {
			std::smatch constMatch;
			for(auto &nameRegPair: constsRegMap) {
				while(std::regex_search(it->data, constMatch, nameRegPair.second)) {
					std::string::size_type pos = std::distance(it->data.cbegin(), constMatch[2].first);
					// 値にして置換
					double value = fort::eq(constsDefMap.at(nameRegPair.first));
					it->data.replace(pos, constMatch.str(2).size(), "(" + std::to_string(value) + ")");
//					// 数式文字列置換
//					it->data.replace(pos, sm.str(2).size(), std::string("(") + constsDefMap.at(nameRegPair.first) + ")");
					//mDebug() << "it->data=" << it->data;

				}
			}
		}
	}// set:と定数置換終わり

	// torusを90度回転したら交点が出なくなった。定数の置換がおかしい様子
	// と思ったらtorusを90.00000001度回転した時の4次式の解の誤差が大きくなるせいだった。

	//  c1*c1みたいな場合の展開が{cos(0)}*{cos(0)}みたいになってしまうので(数値)*(数値)とした
	//  括弧がないと -c1 が --1.0みたいになってしまう
}

/*
 * MCNPでのxsdirと断面積ファイルのの探索順序は下記の通り
 * 	1.XSDIR = cross-section directory file name on the MCNP execution line,
 * 	2.DATAPATH = datapath in the INP file message block,
 * 	3.the current directory,
 * 	4.the DATAPATH entry on the first line of the XSDIR file,
 * 	5.the UNIX environmental variable setenv DATAPATH datapath,
 * 	6.the individual data table line in the XSDIR file (see below under Access Route), or
 * 	7.the directory specified at MCNP compile time in the BLOCK DATA subroutine.
 * 	なのでxsdirの非明示時の自動探索優先は1．カレントディレクトリ 2．環境変数。
 */
std::string inp::InputData::getAlternativeXsdirFileName(bool quiet)
{
	std::string xsdirFileName;
	// 明示的指定がなければ./xsdirを探す。 実はwindowsはパスセパレータに/を用いてOKなので./xsdirで通じるはず。
	if(utils::exists("./xsdir")) {
		xsdirFileName = "./xsdir";
    } else if(std::getenv("DATAPATH")) {
		// 次は環境変数DATAPATH.環境変数はWindowsの場合SJISなのでUTF8に変換する
        xsdirFileName = utils::systemEncodingToUtf8(std::string(std::getenv("DATAPATH"))) + PATH_SEP + "xsdir";
        if(!quiet) mDebug() << "XSDIR was not set in the input file. $DATAPATH was used. xsdir=" << xsdirFileName;

	} else {
        if(!quiet) mWarning() << "xsdir file was not specified nor found in current directory or environmental variable."
				   << " Cross section files won't be read.";
	}
	return xsdirFileName;
}

void inp::InputData::toLowerDataLines(std::list<inp::DataLine> *lines)
{
	for(auto &dl: *lines) {
		utils::tolower(&dl.data);
	}
}

inp::InputData::InputData(const conf::Config &config)
{
    config_ = config;
//	mode_ = config.mode;
//	warnPhitsCompat_ = config.warnPhitsIncompatible;
//	xsdirFileName_ = config.xsdir;
}

bool inp::InputData::confirmXsdir() const
{
	std::string xsdirFile = xsdirFilePath();
    if(xsdirFile.empty()) {
        if(!config_.quiet) mWarning() << "XSDIR file is not set (empty).";
		return false;
	} else if(!utils::exists(xsdirFile)) {
        if(!config_.quiet) mWarning() << "XSDIR file =" << xsdirFile << "not exists.";
		return false;
	} else {
        if(config_.verbose) mDebug() << "XSDIR file exists. file =" << xsdirFile;
#ifdef ENABLE_GUI
		emit fileOpenSucceeded(std::make_pair(inputFile_, xsdirFile));
#endif
		return true;
	}
}




// signalを出せるようにコンストラクタから分離した
void inp::InputData::init(const std::string &inputFileName)
{
	inputFile_ = inputFileName;
    if(config_.mode == McMode::AUTO) config_.mode =  inp::guessMcModeFromFile(inputFile_);

	// phits.in のセクションなしfile=に対応する必要ない。phits.inの並列実行時入力ファイル固定に対応する必要はない。
	// 入力ファイルを直接指定すれば良いだけ。
	path_ = utils::separatePath(inputFile_).first;
	dataLines_ = readFile("", inputFile_);    // こここでインクルード処理完了
	// FIXME インライン拡張入力を削除しないように修正したい。

    // replace tabchar by eight spaces.
    for(auto &line: dataLines_) {
        line.data = std::regex_replace(line.data, std::regex("\t"), "        ");
    }

	// コメントアウトは実は共通ではない。mcnpではtitle部分がコメント業の場合それをtitleとして取得する。
	// ゆえにカードブロック処理はコメント削除に先んじることになる。
	/*
	 * メタカードの処理は以下の順
	 *
	 * 1．ファイルインクルード(inflがコメントアウトされていないことを確認しつつ)
	 *   ↑この時#以外の後置コメントも削除
	 * 2．前置コメント処理
	 * -------- ここからphits/mcnpの差が生じる
	 * 3．set: (setはファイル全体で適用されるのでセクション分け前に適用する。)
	 * -------- ここからセクション依存
	 * 4．行連結
	 * 5．改行挿入
	 * 6．拡張入力内のメタカード展開
	 *
	 */

}



void inp::InputData::dump(std::ostream &os) const
{
	for(auto& x: dataLines_) {
		os << x << std::endl;
	}
}


void inp::InputData::dumpData(std::ostream &os) const
{
	for(auto& x: dataLines_) {
		os << x.data << std::endl;
	}
}



void inp::InputData::dump(std::ostream &os, const std::list<inp::DataLine> &dataList)
{
	for(auto& x: dataList) {
		os << x << std::endl;
	}
}


void inp::InputData::dumpData(std::ostream &os, const std::list<inp::DataLine> &dataList)
{
	for(auto& x: dataList) {
		os << x.data << std::endl;
	}
}



// inputFileのstartLine	からendLineまでデータを読み込み、リストにして返す
std::list<inp::DataLine> inp::InputData::readFile(const std::string &parentFileName,
												  std::string inputFileName, bool echo,
												  std::size_t startLine, std::size_t endLine,
												  bool warnPhitsCompat)
{
    inputFileName = utils::dequote('\"', inputFileName);
	/*
	 * inputFileNameは相対パス、絶対パスどちらの場合もあり得る
	 * 入力ファイルの場合相対/絶対どちらでも差は出ない
	 * 問題はincludeする場合。この時パスの起点を
	 * ・カレントディレクトリとするか
	 * ・includeしている入力ファイルのディレクトリ
	 * のどちらに設定するか。
	 * → 明らかに入力ファイルのディレクトリとするのが正しい。
	 *
	 * ゆえにreadFileを再帰的に呼ぶ場合(＝includeを使用している場合)、
	 * 曖昧さを除くため(パスが存在する場合)パスを追加して呼ぶ
	 * またreadFileのinputFileNameはUTF8とすること。
	 *
	 */
	// inputFileNameにパスが含まれる場合、includeするファイルにもパスを付与する必要がある。
    const std::pair<std::string, std::string> pathAndFile = utils::separatePath(inputFileName);
	const std::string path = pathAndFile.first;//, fileName = pathAndFile.second;

    // windows環境ではファイル名はsjisでなければならない
	std::ifstream ifs(utils::utf8ToSystemEncoding(inputFileName).c_str());
	if(ifs.fail()) {
		std::string message = "No such a input file \"" + inputFileName + "\"";
		throw std::runtime_error(message);
	} else {
#ifdef ENABLE_GUI
		//mDebug() << "In InputData::readFile: path, parent, inpfile ==="<< path << "," << parentFileName << "," << inputFileName;
		emit fileOpenSucceeded(std::make_pair(parentFileName, inputFileName));
#else
		(void) parentFileName;
#endif

	}

	std::list<DataLine> retlist;
	std::size_t lineNumber = 0;
	std::string buff;
	std::smatch sm;

	while(getline(ifs, buff)) {
		++lineNumber;
		utils::sanitizeCR(&buff);
		if(buff.size() > 80 && warnPhitsCompat) {
			mWarning(inputFileName + ":" + std::to_string(lineNumber)) << "More than 80 characters in a line is phits-imcompatible";
		}
		// lineNumber < startLineなら開始行へ到達するまで早送り
		if(lineNumber < startLine) {
			while(getline(ifs, buff)) {
				++lineNumber;
				if(lineNumber == startLine) {
					std::cout << "Start reading file =" << inputFileName
							  << ":" << lineNumber << std::endl;
					break;
				}
				// 早送り中にEOFに来たらエラー
				if(ifs.eof()) {
					std::stringstream ss;
					ss<< "Error: Unexpected EOF while reading file = " << inputFileName
							  << ". start line=" << startLine << ", EOF line=" << lineNumber;
                    throw std::invalid_argument(ss.str());
				}
			}
		}
		// endLineを超えていたらbreakする。
		if(lineNumber > endLine) {
			break;
		}
		//if(ifs.eof()) break;

		// ファイル名は大文字小文字区別ありなのでここで小文字化してはいけない

		// #以外の後置コメントパターンは即時処理して良い
		// MCNPでは「行頭継続＋後置コメント」のみはblank-line-delimiterと区別されるので
		// ここで後置コメントを削除すると区別がつかなくなるという問題がある。



		// MCNP READカード処理.
		// READカードパターンは行頭4文字以内から"read"なのでカード自体はコメントの影響を受けない。
		// 但し引数はコメントアウトされたりして影響を請けるので考慮する必要がある。
		if(std::regex_search(buff, sm, mcnp::getReadCardPattern())) {
			// READカード発見したらその行はコメント化して保存する
			retlist.emplace_back(DataLine(inputFileName, lineNumber, std::string("c Include by READ card = ") + buff, echo));

			// 前置コメントパターンはREADカードパターンと競合するのでREADの引数は含まれないことが保証されている。
			auto readCardResult = mcnp::procReadCard(sm.suffix());
			std::string includedFileName = readCardResult.first;
			bool echoFlag = readCardResult.second;

			if(includedFileName.empty()) {
                throw std::invalid_argument(inputFileName + ":" + std::to_string(lineNumber)+ " Read card requires a FILE parameter.");
			}
			// include対象が相対パスか絶対パスか判断してパスを追加
			if(utils::isRelativePath(includedFileName)) includedFileName = path + includedFileName;

			// 再帰呼出し。echo=falseならNOECHOで読み込む
			try {
				auto includedVec = readFile(inputFileName, includedFileName, echoFlag && echo);
				retlist.insert(retlist.end(), includedVec.begin(), includedVec.end());
			} catch (std::runtime_error &re) {
				std::stringstream ss;
				ss << inputFileName<< ":" << lineNumber << " " << "While reading included file, " << re.what();
                // 最初にエラーが発生した時点で例外を投げてそれ以上の読み込みは止める。どうせ入力内容に矛盾が生じてエラーの嵐になるから
				throw std::runtime_error(ss.str());
			}

		// PHITS includeカード処理
//		} else if (std::regex_search(buff, sm, phits::getIncludeCardPattern())) {
		} else if (std::regex_search(buff, sm, std::regex(R"(^ *[iI][nN][fF][lL]:)"))) {
			std::string includedFileName;
			std::pair<std::size_t, std::size_t> range;
			try {
				auto inflResult = phits::procInflCard(buff);
				includedFileName = inflResult.first;
				range = inflResult.second;
			} catch (std::invalid_argument &ia) {
                (void) ia;
				throw std::runtime_error(inputFileName + ":" + std::to_string(lineNumber) + " Invalid infl: card, str=" + buff);
			}

			// 今のcard patternではemptyになるはずは無いが一応チェック
			if(includedFileName.empty()) {
				std::cerr << "Warning! Argument of infl card is empty, ignored." << std::endl;
			} else {
				// include対象が相対パスか絶対パスか判断してパスを追加
				if(utils::isRelativePath(includedFileName)) includedFileName = path + includedFileName;
				//mDebug() << "include file=" << includedFileName << ", max range=" << range.first << ", " << range.second;
                try{
					// infl:にはnoechoオプションは無い
					auto includedVec = readFile(inputFileName, includedFileName, false, range.first, range.second);
                    retlist.insert(retlist.end(), includedVec.begin(), includedVec.end());
				} catch(std::runtime_error &re) {
					std::stringstream ss;
                    ss << inputFileName<< ":" << lineNumber << " " << "While reading an included file, " << re.what();
                    throw std::invalid_argument(ss.str());
                }
			}
		} else {
			//mDebug() << "data=" << DataLine(inputFileName, lineNumber, buff, echo);
			retlist.emplace_back(DataLine(inputFileName, lineNumber, buff, echo));
		}
	}  // getline終わり
	if(!parentFileName.empty()) {
		retlist.emplace_back(DataLine(inputFileName, lineNumber, std::string("c End file =") + inputFileName, echo));
	}
	return retlist;
}


void inp::InputData::removeComment(McMode mode, std::list<DataLine> *inputData)
{
    std::smatch sm;
	// 行頭c空白 コメント行の削除
	inputData->remove_if([&](const DataLine& dl){return std::regex_search(dl.data, sm, comm::getPreCommentPattern());});

	if(mode == McMode::MCNP || mode == McMode::QAD || mode == McMode::MARS) {
		return;
	} else if(mode == McMode::PHITS) {
		auto cellSectionName = phits::toSectionString(phits::Section::CELL);
		auto surfSectionName = phits::toSectionString(phits::Section::SURFACE);
		// 行中#コメントの削除。currentSectionNameが"surface", "cell"以外なら#コメントを削除。
		// "!%$"はファイル読み取り時に削除済みなので対象は#のみ。
		std::string currentSectionName;
		for(auto & dl: *inputData) {
			if(std::regex_search(dl.data, sm, phits::getSectionPattern())) {
				currentSectionName = sm.str(1);
				phits::canonicalizeSectionName(&currentSectionName);
			}
			if(currentSectionName == cellSectionName || currentSectionName == surfSectionName) {
				continue;
			} else {
				if(std::regex_search(dl.data, sm, std::regex(R"(#)"))) {
					dl.data = sm.prefix();
				}
			}
		}
	} else {
        throw std::invalid_argument("McMode is not set");
	}
}

// 後置連結->前置連結 の順に行連結を行う。拡張入力は連結する方としてもされる方としても扱わない。
// 第二引数は前置継続パターンを処理するかのフラグ
void inp::InputData::concatLine(std::list<DataLine> *inputData, bool enablePre, bool warnPhitsCompat)
{
    std::smatch sm;
	// 後置継続パターンの処理. セクション名が"cell", "surface"以外なら#コメント
    for(auto it = inputData->begin(); it != inputData->end(); ++it) {
		// 間違った入力データ構造になるので、この段階では拡張入力の行継続は展開しない。
		if(std::regex_search(it->data, sm, inp::org::getOriginalCommandPattern())) continue;

		std::string continuedLine = "";
		if(std::regex_search(it->data, sm, comm::getMcnpPostContinuePattern())) {
			if(warnPhitsCompat) mWarning(it->pos()) << "End-of-line continue pattern \"&\" is not phits-compatible.";
			continuedLine = sm.prefix();
		} else if(std::regex_search(it->data, sm, comm::getPhitsPostContinuePattern())) {
			continuedLine = sm.prefix();
		}

		if(!continuedLine.empty()) {
			// 後置継続パターンが出たら次の行のデータに前方連結し、現在行は削除
			std::list<inp::DataLine>::iterator next_it = it;
			// 拡張入力行には連結しないので通常入力行まで進める。
			while(++next_it, std::regex_search(next_it->data, sm, inp::org::getOriginalCommandPattern())) {
				if(next_it == inputData->end()) {
                    throw std::invalid_argument(it->pos() + "Continue pattern in the last line is invalid.");
				}
			}

			next_it->data = continuedLine + " " + next_it->data;
			it = inputData->erase(it);
			--it;
			continuedLine.clear();
		}
	}



	if(!enablePre) return;  //前置連結メタカード処理をしない場合はここでリターン
	// 前置継続パターンの処理。拡張入力行は前置パターンと重複しないので拡張入力のことは考慮しなくて良い。
    for(std::list<inp::DataLine>::iterator it = inputData->begin(); it != inputData->end(); ++it) {
		//mDebug() << "Checking pre-continue pattern. data=" << it->data << ", cont===" << ();
        if(std::regex_search(it->data, sm, comm::getPreContinuePattern())) {
			// 旧：先頭に5個以上空白があっても空白しかない行はblank-line-delimiterとして扱うので連結処理しない
			// 新：mcnp入力では「前置継続＋後置コメント」のみの行をdelimiter扱いしない。
			//    コレに対応するために先にdelimiterで入力を分割した後で継続行連結をする。
			//    ゆえにここでdelimiter扱いしてはならない。
			//if(it->data.find_first_not_of(" \t") == std::string::npos) continue;

			// 先頭行でcontカードが出たらおかしい。
            if(it == inputData->begin()) throw std::invalid_argument(it->pos() + " Continue pattern is invalid in the beginning");

            // 前置行継続パターンが出たら前の行にデータ連結し、連結元は削除
            std::list<inp::DataLine>::iterator prev_it = it;
            --prev_it;
            prev_it->data = prev_it->data + " " + std::string(sm.suffix());
            it = inputData->erase(it);
            --it;
		}
    }
}

void inp::InputData::expandIjmr(std::list<DataLine> *inputData, bool warnPhitsCompat)
{
	std::smatch sm;
//	const std::string separators = (mode == McMode::MCNP)? " =": " ";  // phitsでは"="は除去しない。
	const std::string separators = " ";  // phitsでは"="は除去しない。

	//std::cout << "seps=(" << separators << ")" << std::endl;
    for(auto& dl: *inputData) {
        try {
			// セクション文字列ではijmr展開しない
			if(!std::regex_search(dl.data, sm, inp::phits::getSectionPattern())) {
				dl.data = inp::DataLine::expandIJMR(dl, separators, warnPhitsCompat);
				utils::trim(&(dl.data));
			}
        } catch(std::invalid_argument &e) {
            throw std::invalid_argument(dl.pos() + " While expanding imr-expression, " + e.what());
        }
	}
}



void inp::InputData::replaceSemicolon(std::list<DataLine> *inputData, bool warnPhitsCompat)
{
	std::smatch sm;
    for(std::list<DataLine>::iterator it = inputData->begin(); it != inputData->end(); ++it) {
		if(std::regex_search(it->data, sm, inp::org::getOriginalCommandPattern())) continue;

        if(it->data.find(";") != std::string::npos) {
			auto dataVec = utils::splitString(";", it->data, true);
			if(!dataVec.empty() && warnPhitsCompat) {
				// ne=Nの後の;はphits非互換なので警告する。
				std::smatch sm;
				if(std::regex_search(dataVec.at(0), sm, std::regex(R"( *n[aetrxyz] * =)"))) {
					mWarning(it->pos()) << "\";\" after number of group is not phits-compatible.";
				}
			}
            const std::string file = it->file;
            const std::size_t line = it->line;
            // ";"を含む現在行*itを削除
            it = inputData->erase(it);
            // eraseの返り値は次の要素なので巻き戻さなくて調度良い。
            for(auto &dat: dataVec) {
                // insertの返り値は挿入した要素, 挿入位置はposとpos-1の間
                it = inputData->insert(it, DataLine(file, line, dat));
                ++it;
            }
            --it; // ループ最初で加算されるので1つ巻き戻しておく必要がある。
        }
    }
}
