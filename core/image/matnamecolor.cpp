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
#include "matnamecolor.hpp"

#include <algorithm>
#include <memory>
#include <regex>
#include <sstream>

#include "color.hpp"
#include "core/io/input/dataline.hpp"


#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"


// 文字列vectorからパレットを作成する。
// 文字列は具体的にはphitsのmat name colorセクションの文法を採用する。
// キーは材料名 M1なら"1"
bool img::MaterialColorData::isUserDefinedColor(const img::MaterialColorData &matColData)
{
    if(matColData.matName().empty()) {
        return false;
    } else {
        return matColData.matName().front() != '*';
    }
}

std::map<std::string, img::MaterialColorData> img::MaterialColorData::fromCardsToMap(const std::list<inp::DataLine> &inputData)
{
    if(inputData.empty()) throw std::invalid_argument("Too few data for mat color section");

    std::string colNameLine;
    // 最初の行がタイトル行でなければデフォルトタイトル行を追加。
    std::vector<std::string> firstDataBlocks = utils::splitString(" ", inputData.cbegin()->data, true);
    if(std::regex_search(firstDataBlocks.front(), std::regex(R"(mat|name|size|color)", std::regex::icase))) {
        colNameLine = inputData.cbegin()->data;
    } else {
        colNameLine = "mat name color";
    }
	// 最初の行はテーブルのタイトル(mat) と (name size color)
	utils::tolower(&colNameLine);
	// mat-name-color-size行のデータ
	std::vector<std::string> mncsParams = utils::splitString(" ", colNameLine, true);   

	// mat-name-col-size行は mat name color size non 以外を含まない。
	static std::unordered_set<std::string> validKeywords{"mat", "name", "color", "size", "non"};
	for(size_t i = 0; i < mncsParams.size(); ++i) {
		if(validKeywords.find(mncsParams.at(i)) == validKeywords.cend()) {
            throw std::invalid_argument(inputData.cbegin()->pos()
                                        + " parameters should be mat name col size or none. param ="
                                        + mncsParams.at(i));
		}
	}
    /*
     *  列名指定行のエラーチェック
     * 最小はmatとcolorの2行構成
     */
    auto it = inputData.cbegin();
    if(mncsParams.size() < 2) {
        throw std::invalid_argument(it->pos() + " Invalid mat-name-color-size definition line. data = " + colNameLine);
    } else if(mncsParams.front() != "mat") {
        throw std::invalid_argument(it->pos() + " mat-name-color-size line should be start with \"mat\". data = " + colNameLine);
	}
	// 列名指定行の要素チェック. matは既に確認済みなので name, size, colorの順番と漏れがないことを調べる。
	// "non"で読み飛ばす列を指定できる → name size col以外読み込まないからほっとけばいい。
	// "color", "name"のどちらかのみという場合も合法
	auto nameIt = std::find(mncsParams.cbegin(), mncsParams.cend(), "name");
	auto sizeIt = std::find(mncsParams.cbegin(), mncsParams.cend(), "size");
	auto colorIt = std::find(mncsParams.cbegin(), mncsParams.cend(), "color");


	if(nameIt == mncsParams.cend() && colorIt == mncsParams.cend()) {
		throw std::invalid_argument(std::string("At least, name or color entry is required in mat-size-name-color line. data = ") + colNameLine);
	}
	size_t matIndex = 0;
	bool hasName = nameIt != mncsParams.cend();
	bool hasSize = sizeIt != mncsParams.cend();
	bool hasColor = colorIt != mncsParams.cend();
	size_t nameIndex = !hasName ? 0 :  std::distance(mncsParams.cbegin(), nameIt);
	size_t sizeIndex = !hasSize ? 0 : std::distance(mncsParams.cbegin(), sizeIt);
	size_t colorIndex = !hasColor ? 0 : std::distance(mncsParams.cbegin(), colorIt);

	// ColorPaletteでは材料名と材料インデックスは1対1対応。材料インデックスと色は多対一
	// Phitsでいう材料番号とは材料名であることに注意。
	//          材料番号    表示名       フォントサイズ  色
    std::map<std::string, MaterialColorData> matMap;
    while(++it != inputData.cend()) {
		if(it->data.find_first_not_of(" \t") == std::string::npos) continue;
		// これで括弧でのクオートを有効にして分割。
		auto params = utils::splitString(std::make_pair('{', '}'), " ", it->data, true);
		if(params.size() != mncsParams.size()) {
            throw std::invalid_argument(it->pos() + " Size of mat-name-size-color input and that of data line are different.");
		}

		// クオート外しと n-m展開
		for(auto& param: params) {
			param = utils::expandRangeString(param);  // n-m展開を先にする必要がある。
			param = utils::dequote(std::make_pair('{', '}'), param, true);
		}
		auto matNames = utils::splitString(" ", params.at(matIndex), true);
		for(const auto& mname: matNames) {
			std::string printName = !hasName ? "" : params.at(nameIndex);
			double printSize = !hasSize ? 0  : utils::stringTo<double>(params.at(sizeIndex));
			std::shared_ptr<const Color> colorPtr = !hasColor ? nullptr
															: std::make_shared<const Color>(Color::fromPhitsString(params.at(colorIndex)));
            matMap.emplace(std::make_pair(mname, MaterialColorData(mname, printName, printSize, colorPtr)));
		}
	}
//	for(const auto&dat: matMap) {
//		mDebug() << "matColorName ===" << dat.second.toString();
//	}
	// ここで必要なのはmatName-色-(サイズ-別名データ)
	return matMap;
}




std::string img::MaterialColorData::toString() const
{
	std::stringstream ss;
    ss << "Material = " << matName_ << ": alias = " << aliasName_ << ", size = " << printSize_
	   << ", color = " << (!color_ ? "Notset" : color_->toRgbString());
	return ss.str();
}

#define VARNAME(ARG) #ARG
picojson::value img::MaterialColorData::jsonValue() const
{
	/*
	 * メンバ変数は
	 * 	string matName_;
	 * 	string printName_;
	 * 	double printSize_;
	 * 	shared_ptr<const img::Color> color_;
	 *
	 * MatNameColorのjson値は
	 *	[
	 *		"matname":"1",
     *		"aliasname":"Fe",
	 *		"printsize":1.5,
	 *		"color":[255, 0, 255, 1]
	 *	]
	 * とかそんな感じ。
	 */
    //mDebug() << "jonvalue, matName===" << matName_ << ", alias===" << aliasName_;
	picojson::object matNameColorObj;
	matNameColorObj.insert(std::make_pair(VARNAME(matName_), matName_));
    matNameColorObj.insert(std::make_pair(VARNAME(aliasName_), aliasName_));
	matNameColorObj.insert(std::make_pair(VARNAME(printSize_), printSize_));
	matNameColorObj.insert(std::make_pair(VARNAME(color_), color_->jsonValue()));
	return picojson::value(matNameColorObj);
}

img::MaterialColorData img::MaterialColorData::fromJsonString(const std::string &jsonStr)
{
	picojson::value val;
	const std::string err = picojson::parse(val, jsonStr);
	if (!err.empty()) {
        throw std::invalid_argument(std::string("Parsing json file for cunfig failed. err = ") + err);
	}

	picojson::object& obj = val.get<picojson::object>();
	return fromJsonObject(obj);
}

img::MaterialColorData img::MaterialColorData::fromJsonObject(picojson::object obj)
{
	auto matName = obj[VARNAME(matName_)].get<std::string>();
    auto printName = obj[VARNAME(aliasName_)].get<std::string>();
	auto printSize = obj[VARNAME(printSize_)].get<double>();
	auto colorJsonStr = obj[VARNAME(color_)].get<picojson::object>();
	auto color = std::make_shared<const img::Color>(img::Color::fromJsonObject(colorJsonStr));

    return MaterialColorData(matName, printName, printSize, color);
}

#undef VARNAME
