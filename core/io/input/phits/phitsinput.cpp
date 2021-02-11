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
#include "phitsinput.hpp"

#include <map>
#include <regex>
#include "phitsinputsection.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/inputdata.hpp"
#include "core/io/input/common/commoncards.hpp"
#include "core/option/config.hpp"
#include "core/utils/system_utils.hpp"

namespace {
// PHITSではセクションによってメタカードの処理がことなるので、セクションごとMCNP互換非互換のメタカード処理を使い分ける。
// MCNP非互換(行頭空白行連結が無効)なセクションのメタカード処理
void procMetaCardInPhitsSection(std::list<inp::DataLine> *inputData, bool warnCompat)
{
	//  phitsでも後置行連結メタカードは処理する。
	inp::InputData::concatLine(inputData, false, warnCompat);
	inp::InputData::replaceSemicolon(inputData, warnCompat);

	// FIXME ijmrの一括変換は不可。cell名、surface名などにiを使う場合があり得る
	inp::InputData::expandIjmr(inputData, warnCompat);

	inp::InputData::replaceConstants(inputData);
}
// MCNP互換(行頭空白行連結が有効)なセクションのメタカード処理
void procMetaCardInMcnpCompatSection(std::list<inp::DataLine> *inputData, bool warnCompat)
{
	inp::InputData::concatLine(inputData, true, warnCompat);
	inp::InputData::replaceSemicolon(inputData, warnCompat);
	// FIXME ijmrの一括変換は不可。cell名、surface名などにiを使う場合があり得る。
	inp::InputData::expandIjmr(inputData, warnCompat);
	inp::InputData::replaceConstants(inputData);
}

// phitsでは空行をエラー扱いにしないので、空行を見つけたらエラーではなく削除する。
void removeEmptyLines(std::list<inp::DataLine> *datalines)
{
    for(auto it = datalines->begin(); it != datalines->end();) {
        if(it->data.find_first_not_of(" \t") == std::string::npos) {
            it = datalines->erase(it);
        } else {
            ++it;
        }
    }
}

void removeAfterEndSectionCard(std::list<inp::DataLine> *dataLines)
{
	for(auto it = dataLines->begin(); it != dataLines->end();) {
		if(std::regex_search(it->data, inp::phits::getforceEndSectionCard())) {
			*dataLines = std::list<inp::DataLine>(dataLines->begin(), it);
			return;
		} else {
			++it;
		}
	}
}

// sectセクションが複数存在しないかチェックする。
void checkRedefinedSection(inp::phits::Section sect,
						   const std::multimap<inp::phits::Section, std::list<inp::DataLine>> &secMap)
{
	if(secMap.count(sect) <= 1) return;
    // セクションが複数個ある場合、箇所を指摘して例外を投げるる。
	std::vector<inp::DataLine> firstLines;  // それぞれの重複セクションの最初の行
	auto itrPair = secMap.equal_range(sect);
	auto it = itrPair.first;
	while(it != itrPair.second) {
		firstLines.emplace_back(it->second.front());
		++it;
	}
//	firstLines.emplace_back(itrPair.second->second.front());
	assert(!firstLines.empty());
	std::vector<std::string> posVec;
	for(const auto &dl: firstLines) {
		posVec.emplace_back(dl.pos());
	}
    std::stringstream ess;
    ess << firstLines.back().pos() + " Redefinition of sections found. section ="
           + inp::phits::toSectionString(sect) + ", location =" << posVec;
    throw std::invalid_argument(ess.str());
}

}  // end anonymous namespace




std::vector<phys::ParticleType> inp::phits::PhitsInput::particleTypes() const
{
	std::map<phys::ParticleType, std::string> particleTypesMap;

	auto srcListVec = this->sourceSections();

	std::regex projPattern("^ *[pP][rR][oO][jJ] *= *([-+0-9a-zA-Z ]+)");
	std::smatch projMatch;
	for(auto srcList: srcListVec) {
		for(auto &dl: srcList) {
			//mDebug() << "searching proj string from data=" << dl;
			if(std::regex_search(dl.data, projMatch, projPattern)) {
				auto projStrs = utils::splitString(" ", projMatch.str(1), true);
				//mDebug() << "proj found =" << projMatch.str(1);
				for(auto &projstr: projStrs) {
					if(projstr.empty()) {
						mWarning() << "particle type =" << projstr << "is not supported.";
					} else {
						particleTypesMap[phys::strToParticleType(projstr)] = projstr;
					}
				}
			}
		}
	}
	std::vector<phys::ParticleType> particleTypes;
	for(auto &ptypePair: particleTypesMap) {
		particleTypes.emplace_back(ptypePair.first);
	}
	return particleTypes;
}

std::list<inp::DataLine> inp::phits::PhitsInput::colorCards() const
{
	return getPhitsSectionCards(inp::phits::Section::MATNAMECOLOR);
}


std::vector<std::list<inp::DataLine> > inp::phits::PhitsInput::sourceSections() const
{
	std::vector<std::list<DataLine>> inputLinesVec = this->getPhitsSections(inp::phits::Section::SOURCE);

	for(auto &inputLines: inputLinesVec) {
        procMetaCardInPhitsSection(&inputLines, config_.warnPhitsIncompatible);
	}
	return inputLinesVec;
}


// sectに該当するセクション部分のDalaLineリストを返す。該当セクションが複数ある場合最初のものを返す。
std::list<inp::DataLine> inp::phits::PhitsInput::getPhitsSectionCards(const inp::phits::Section &sect) const
{
	auto it = sectionsMap_.find(sect);
	return (it == sectionsMap_.end()) ? std::list<inp::DataLine>() : it->second;
}

// TODO input関係のテストを書き直し
std::vector<std::list<inp::DataLine> > inp::phits::PhitsInput::getPhitsSections(inp::phits::Section sect) const
{
	std::vector<std::list<inp::DataLine>> retVec;
	auto itrPair = sectionsMap_.equal_range(sect);
	for(auto it = itrPair.first; it != itrPair.second; ++it) {
		retVec.emplace_back(it->second);
	}
	return retVec;
}




std::vector<std::list<inp::DataLine> > inp::phits::PhitsInput::tallySections() const
{
	std::vector<std::list<DataLine>> inputLinesVec = this->getPhitsSections(inp::phits::Section::T_POINT);
	for(auto &inputLines: inputLinesVec) {
        procMetaCardInPhitsSection(&inputLines, config_.warnPhitsIncompatible);
	}
	return inputLinesVec;
}


#include "core/io/input/common/commoncards.hpp"

void inp::phits::PhitsInput::init(const std::string &inputFileName)
{
	namespace ip = inp::phits;
	/*
	 * この関数では基底の4リストlist<DataLine> cellCards_, surfaceCards_, materialCards_, transformCards_;
	 * を初期化する。
	 */
	inp::InputData::init(inputFileName);  // metacardの処理とdataLines_への読み込み

	for(auto it = dataLines_.begin(); it != dataLines()->end(); ++it) {
		comm::removeMcnpPostComment(&(it->data));    // mcnp式後置コメントの除去
		comm::removePhitsPostCommentNotSharp(&(it->data));  // phits式後置コメントの除去
	}


    InputData::removeComment(config_.mode, &dataLines_);  // 行頭コメントアウトはphits/mcnpで同じなのでここ実施
	// 全セクションで共通する定数置換、改行挿入はここで実施
	inp::InputData::replaceConstants(&dataLines_);
    inp::InputData::replaceSemicolon(&dataLines_, config_.warnPhitsIncompatible);
	// ここでq: [end]を処理する。発見したら以後を削除
    for(auto it = dataLines_.begin(); it != dataLines_.end(); ++it) {
        if(std::regex_search(it->data, inp::phits::getForceEndCard())) {
            if(config_.verbose) mDebug() << "input ended with [end]!!! at " << it->pos();
            dataLines_ = (decltype(dataLines_)(dataLines_.begin(), it));
            break;
        }
    }

	// ここでsectionsMap_に振り分け。
	std::list<inp::DataLine> tmpList;
	std::smatch sectionMatch;
	inp::phits::Section oldSection = inp::phits::Section::NOTSET;
	bool isValid = false;
	for(auto &dl: dataLines_) {
		if(std::regex_search(dl.data,ip::getDisabledSectionPattern())) {
			isValid = false;
		} else if(std::regex_search(dl.data, sectionMatch, ip::getSectionPattern())) {
			// offされていない[section]発見時
			isValid = true;
			// セクションに所属していない入力はあってはならない。（メタカードは処理済みで存在しないとして）
//            mDebug() <<"section===" << inp::phits::toSectionString(oldSection) << ",tmpList==" << "\n" << tmpList;
            if(tmpList.empty() && oldSection != ip::Section::NOTSET) {
                mWarning(dl.pos()) << "Empty section!!! Ignored.";
                continue;
            }
//            assert(!tmpList.empty() || oldSection == ip::Section::NOTSET);

			// sectionsMap_に追加前に空白行は削除しておく。
			removeEmptyLines(&tmpList);
			// ここで:qp, [end] 処理
			removeAfterEndSectionCard(&tmpList);

			sectionsMap_.emplace(oldSection, std::move(tmpList));
			std::string sectionTitle = sectionMatch.str(1);
			inp::phits::canonicalizeSectionName(&sectionTitle); // 空白削除と小文字化で正準なタイトル文字列に変換。
			oldSection = inp::phits::toEnumSection(sectionTitle);
		} else {
			if(isValid) tmpList.emplace_back(dl);
		}
	}
	// tmpListが残存していたら追加する。空セクションがあればtmpListが空の場合も有り得る
	if(!tmpList.empty()) {
		removeEmptyLines(&tmpList); // 空白行は削除
		removeAfterEndSectionCard(&tmpList); // :qp, [end] 処理
		sectionsMap_.emplace(oldSection, std::move(tmpList));
	}

//	for(auto p: sectionsMap_) {
//		mDebug() << "section =====" << toSectionString(p.first);
//		for(auto dl: p.second) {
//			mDebug() << dl.data;
//		}
//	}





	// セルセクションが複数あった場合どうなるの？ → PHITSではエラーになるのでそのように。
	checkRedefinedSection(ip::Section::CELL, sectionsMap_);
	auto it = sectionsMap_.find(ip::Section::CELL);
	if(it != sectionsMap_.end()) {
		cellCards_ = it->second;

		//InputData::toLowerDataLines(&cellCards_);
		// ここで小文字化してしまうとtfile=TetraFile.eleみたいなのが処理できなくなる。
		// tfile=の引数は小文字から除外する
		std::smatch sm;
		for(auto &dl: cellCards_) {
			if(std::regex_search(dl.data, sm, std::regex(R"(tfile *= *([-+0-9a-zA-Z_.()\[\]\\/]+))", std::regex_constants::icase))) {
				std::string tfileArg = std::string(sm[1].first, sm[1].second);
				std::string frontPart = std::string(dl.data.cbegin(), sm[1].first);
				std::string backPart = std::string(sm[1].second, dl.data.cend());
				utils::tolower(&frontPart);
				utils::tolower(&backPart);
				dl.data = frontPart + tfileArg + backPart;
			} else {
				utils::tolower(&dl.data);
			}
		}
        procMetaCardInMcnpCompatSection(&cellCards_, config_.warnPhitsIncompatible);
	}






	checkRedefinedSection(ip::Section::SURFACE, sectionsMap_);
	it = sectionsMap_.find(ip::Section::SURFACE);
	if(it != sectionsMap_.end()) {
		surfaceCards_ = it->second;
		InputData::toLowerDataLines(&surfaceCards_);
        procMetaCardInMcnpCompatSection(&surfaceCards_, config_.warnPhitsIncompatible);
	}
	checkRedefinedSection(ip::Section::MATERIAL, sectionsMap_);
	it = sectionsMap_.find(ip::Section::MATERIAL);
	if(it != sectionsMap_.end()) {
		materialCards_ = it->second;
		InputData::toLowerDataLines(&materialCards_);
        procMetaCardInMcnpCompatSection(&materialCards_, config_.warnPhitsIncompatible);
	}
	checkRedefinedSection(ip::Section::TRANSFORM, sectionsMap_);
	it = sectionsMap_.find(ip::Section::TRANSFORM);
	if(it != sectionsMap_.end()) {
		transformCards_ = it->second;
		InputData::toLowerDataLines(&transformCards_);
        procMetaCardInMcnpCompatSection(&transformCards_, config_.warnPhitsIncompatible);
	}

	checkRedefinedSection(ip::Section::PARAMETERS, sectionsMap_);
	std::list<inp::DataLine> parameterLines;
	it = sectionsMap_.find(ip::Section::PARAMETERS);
	if(it != sectionsMap_.end()){
		parameterLines = it->second;
        procMetaCardInPhitsSection(&parameterLines, config_.warnPhitsIncompatible);
	}

	ip::PhitsInputSection parameterSection(ip::toSectionString(ip::Section::PARAMETERS),
                                           parameterLines, true, config_.warnPhitsIncompatible);


    if(config_.xsdir.empty()) {
        config_.xsdir = parameterSection.getInputArrayParameter("file", 7);
        if(config_.xsdir.empty()) config_.xsdir = inp::InputData::getAlternativeXsdirFileName(config_.quiet);
	}
    if(!config_.xsdir.empty()) {
        if(utils::isRelativePath(config_.xsdir) && !path_.empty()) config_.xsdir = path_ + PATH_SEP + config_.xsdir ;
	}



	/*
	 *  phitsのmaterialセクションは
	 * MAT[1]
	 * 1001 1
	 * みたいな行頭継続パターン無しでの継続がOKになっている。
	 * なのでここでその処理をする。
	 * 要は次のmaterialカードが出現するまで行を連結する。
	 *
	 */

    for(auto it = materialCards_.begin(); it != materialCards_.end();) {
        // PEND とりあえずMTカードの扱いは保留。MAT[n]に所属する？しない？
        // とりあえずmtは削除する。するとdedxfileがnlib同様のパラメータ扱いとなって、既存の文法との整合性が良くなる。

        if(std::regex_search(it->data, std::regex(R"(^ {0,4}mt)", std::regex::icase))) {
            it = materialCards_.erase(it);
            continue;
        }

		if(inp::comm::GetMaterialIdStr(it->data).empty()) {
			if(it == materialCards_.begin()) {
                throw std::invalid_argument(it->pos() + " the front of material cardblock is not a valid material card.");
			} else {
				auto str = it->data;
				--it;
				it->data += " " + str;
				++it;
				it = materialCards_.erase(it);
				--it;
			}
		}
		++it;
	}
}








