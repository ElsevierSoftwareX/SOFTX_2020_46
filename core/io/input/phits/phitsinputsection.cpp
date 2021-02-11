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
#include "phitsinputsection.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <stdexcept>
#include <sstream>

#include "core/io/input/common/commoncards.hpp"
#include "core/io/input/inputdata.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/original/original_metacard.hpp"
#include "core/utils/utils.hpp"

namespace {
//bool isSpace(char c) {return std::isspace(static_cast<unsigned char>(c));}

bool isCaseDependentParam(const std::string &arg)
{
	// fileが入る名前のパラメータはファイルしていなのでcase dependent
	// title, txtも文字列なのでcd
	static const std::regex caseDependentParamRegex(R"(file|title|txt)", std::regex_constants::icase);
	return std::regex_search(arg, caseDependentParamRegex);
}
}

inp::phits::PhitsInputSection::PhitsInputSection(const std::string &name,
												 const std::list<DataLine> inputData,
												 bool verbose, bool warnPhitsCompat)
    :sectionName_(name)
{
	(void) verbose;
	(void)warnPhitsCompat;

//	mDebug() << "PhitsInputSectionコンストラクタ";
//	for(auto id: inputData) {
//		mDebug() <<"data=" << id.data;
//	}


	/*
	 * MCNP互換セクションはPhitsInputSectionで処理しないように変更された。
	 * cell, surface, material, transformをこのクラスを使って処理することはもう無い。
	 *
	 *
	 */

    /*
     * PhitsInputSectionがやること
	 * ・入力と独自拡張を分離
	 * ・上を行ったあと空白の行は削除
	 *
	 * 以下の処理はInputDataへ移行したのでここではしない
	 * ・コメント削除
	 * ・行連結
	 * ・セミコロンの改行への置換
	 * ・IMJR表現の展開
	 * しかし、InputDataの時点では拡張入力はコメントと同等なので拡張入力の
	 * 上記処理はここに残っている。NOTE よくない。
	 *
     */

    // 入力と独自拡張の分離
    std::smatch sm;
	for(auto &dataLine: inputData) {
		if(std::regex_search(dataLine.data, sm, inp::org::getOriginalCommandPattern())) {
            assert(sm.size() == 2);
			extension_.emplace_back(DataLine(dataLine.file, dataLine.line, sm.str(1)));
        } else {
			input_.emplace_back(dataLine);
        }
    }

	// 空白行除去
	auto inputEnd = std::remove_if(input_.begin(), input_.end(),
								 [](inp::DataLine dl){
									return dl.data.find_first_not_of(" \t") == std::string::npos;
								 });
	input_.erase(inputEnd, input_.end());

	/*
	 *  NOTE ここでメッシュ定義文文もうまく処理できないだろうか？
	 *  map[r-type] = "1; ne=1   0 1 2 3 4"; みたいに分全体を保存してしまうとかで。
	 */


	// Phits専用ルーチンになったので1パラメータ1行でOk
	for(auto &dl: input_) {
		// イコールの前後に空白があるとパラメータ判定がマッチしないので詰める
        dl.data = std::regex_replace(dl.data, std::regex(" *= *"), "=");

		 /*
		  *  parameterセクションのパラメータはアルファベットのみ。
		  * 但しパラメータ値は
		  * ・ファイル名 → 日本語含む任意文字(含む_+-.")、セパレータ:\/
		  * ・粒子指定、→ +-.アルファベット
		  * ・数値と数式 → 0-9a-zA-Z(){}+-/%.
		  *
		  *
          * 日本語対応要チェック。但し日本語ロケールでstd::regex使うとmingwでは落ちるので要注意
		  * Linux+g++ではロケール設定無しで\wが日本語文字にヒットしてくれる。
          * Windows+msvc/mingwでもちゃんと動く。たまたま?
		  */
		 // sm(1)はパラメータ名 sm(2)は要素番号、sm(3)は値
		static std::regex arrayParamRegex(R"((\w+)\s*\(\s*([0-9])+\s*\)\s*=\s*([-+/%.:\\ \w)(\"]+[^=]*))");
		static std::regex paramRegex(R"((\w+)\s*=\s*([-+/%.:\\ \w)(\"]+[^=]*))");

		if(std::regex_search(dl.data, sm, arrayParamRegex)) {
			// file( 7 )のようなアレイ型パラメータの処理
			std::string name = sm.str(1);
			utils::tolower(&name);
			size_t num = utils::stringTo<size_t>(sm.str(2));
			std::string value = utils::dequote('"', sm.str(3));
			if(!isCaseDependentParam(name)) utils::tolower(&value);
			//mDebug() << "parameter, name===" << name << ", num=" << num << "value=" << value;
			inputArrayParams_[name].emplace(num, value);
		} else if(std::regex_search(dl.data, sm , paramRegex)) {
			// name = value の普通のパラメータ処理
			std::string name = sm.str(1);
			utils::tolower(&name);
			std::string value = utils::dequote('"', sm.str(2));
			if(!isCaseDependentParam(name)) utils::tolower(&value);
			//mDebug() << "parameter, name ===" << name << "value===" << value;
			inputParams_[name] = value;
		} else {
			// parameterを含まない行はとりあえず小文字化する。
			utils::tolower(&dl.data);
		}
//			std::string::const_iterator startIt;
//			do{
////				for(size_t i = 0; i < sm.size(); ++i) {
////					mDebug() << "i=" << i << "str=" << sm.str(i);
////				}
//				// パラメータの正準な名前は小文字空白なし
//				std::string paramName = sm.str(1);
//				paramName = utils::lowerString(std::regex_replace(paramName, std::regex(" +"), std::string()));
//				std::string paramValue = utils::dequote('"', sm.str(2));
//				// パラメータ名に"file"が含まれているパラメータはcasedpendentなので小文字化しない
//				mDebug() << "paramName=" << paramName << ", value=" << paramValue;
//				if(paramName.find("file") == std::string::npos) utils::tolower(&paramValue);
//				// file( 7 ) みたいなものもOKにするため空白を削除する。そもそもparamNameに")"しか入っていない
//				paramName.erase(std::remove_if(paramName.begin(), paramName.end(), isSpace), paramName.end());
//				inputParams_[paramName] = paramValue;
//				tmpStr += utils::lowerString(sm.prefix()) + paramName + "=" + paramValue;
//				startIt = sm[2].second;
//			} while(std::regex_search(startIt, dl.data.cend(), sm, inp::phits::parameterPattern()));
//			// 検索に該当しなくなったら最期に残った分を追加
//			dl.data = tmpStr + std::string(sm.suffix());
//		} else {
//			// parameterを含まない行はとりあえず小文字化する。
//			utils::tolower(&dl.data);
//		}
	}





	// 拡張入力は行連結と改行処理、行頭c空白コメント処理ができていないので追加作業する。
	InputData::removeComment(McMode::MCNP, &extension_);  // 拡張入力での#削除はしないので常にmcnpモードを適用
	InputData::concatLine(&extension_, true, false);
	InputData::replaceSemicolon(&extension_, false);
//	InputData::expandIjmr(McMode::PHITS, &extension_, false);
	// 今の所拡張入力ではパラメータを定義していない。が一応
	for(const inp::DataLine &dl: extension_) {
		if(std::regex_search(dl.data, sm , inp::phits::parameterPattern())) {
			// パラメータの正準な名前は小文字空白なし
			std::string canonicalParamName = sm.str(1);
			canonicalParamName = std::regex_replace(canonicalParamName, std::regex(" +"), std::string());
			utils::tolower(&canonicalParamName);
			std::string canonicalParamValue = sm.str(2);
			// パラメータ名に"file"が含まれているパラメータはcasedpendentなので小文字化しない
			if(canonicalParamName.find("file") == std::string::npos) utils::tolower(&canonicalParamValue);
			extensionParams_[canonicalParamName] = canonicalParamValue;
		}
	}

	// 空白行除去
	auto extensionEnd = std::remove_if(extension_.begin(), extension_.end(),
								 [](inp::DataLine dl){
									return dl.data.find_first_not_of(" \t") == std::string::npos;
								 });
	extension_.erase(extensionEnd, extension_.end());
	// 小文字化
	for(auto &inp: extension_) {
		utils::tolower(&(inp.data));
	}
}

std::string inp::phits::PhitsInputSection::toString() const
{
	std::stringstream ss;
	ss << "Section name = " << sectionName_ << std::endl;
	ss << "Input =" << std::endl;
	for(auto &x: input_) {
		ss << x.pos() << " " << x.data << std::endl;
	}
	ss << "Extension =" << std::endl;
	for(auto &x: extension_) {
		ss << x.pos() << " " << x.data << std::endl;
	}
	return ss.str();
}

std::string inp::phits::PhitsInputSection::getInputParameter(const std::string &paramName) const
{
	if(inputParams_.find(paramName) == inputParams_.end()) {
		return "";
	} else {
		return inputParams_.at(paramName);
	}
}

std::string inp::phits::PhitsInputSection::getInputArrayParameter(const std::string &paramName, int num) const
{
	if(inputArrayParams_.find(paramName) == inputArrayParams_.end()) {
		return "";
	}
	const std::map<int, std::string> &pmap = inputArrayParams_.at(paramName);
	if(pmap.find(num) == pmap.end()) {
		return "";
	}
	return pmap.at(num);
}



std::string inp::phits::PhitsInputSection::expandBrace(const std::string &str)
{
	return utils::expandRangeString(str);
}



