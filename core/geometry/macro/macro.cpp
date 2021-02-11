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
#include "macro.hpp"

#include <algorithm>
#include <regex>

#include "core/io/input/cellcard.hpp"
#include "core/io/input/dataline.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"



namespace {

std::string makeExpandedMacroBodyString(const std::string &macroBodyName, int numSurfaces)
{
    std::string op = (macroBodyName.front() == '-') ? " " : ":";
    //展開結果の文字列を作成。
    std::string replacing = std::string(" (");
    for(int i = 1; i <= numSurfaces; ++i) {
        replacing += macroBodyName + "." + std::to_string(i);
        if(i != numSurfaces) replacing += op;
    }
    replacing += ")";
    return replacing;
}

}  // end name

// Surfaceセクションでマクロボディを置換するような文字列を生成する
// S1がRPPの場合 セルセクションで
//  -S1は (-s1.1 -s1.2 -s1.3 -s1.4 -s1.5 -s1.6)
//  +S1は ( s1.1: s1.2: s1.3: s1.4: s1.5: s1.6) と置換する。
// equation 中のsurfNameをnumSurf個のマクロボディに展開する
std::string geom::macro::makeReplacedEquation(const std::string &macroBodyName,
                                  const std::string &replacingString,
                                  const std::string &equation)
{
//    mDebug() << "oldEq=" << equation;
    assert(!macroBodyName.empty());

	// surfNameが"2" なら-2は該当せず。かつ+2には該当するようにする必要がある。
    // この時 "1.2"にも該当してはいけない
    std::regex pattern;
    std::string target;
    if(macroBodyName.front() == '-') {
		target = std::string("(^|[^0-9a-zA-Z.])") + std::string("(") + macroBodyName + ")" + std::string("([^0-9a-zA-Z.]|$)");
    } else {
		target = std::string("(^|[^-0-9a-zA-Z.])") + std::string("\\+*(")+macroBodyName+")" +std::string("([^0-9a-zA-Z.]|$)");
    }
    /*
     *  非マクロボディセルの一部に一致することを避ける必要がある
     * 故に"前後がsurface入力の一部でない"をregexに入れている。
     */

//    mDebug() << "regex string=== " << target << ", replacing=" << replacingString;

    pattern = std::regex(target);
    std::smatch sm;
    auto newEq = equation;
    while(std::regex_search(newEq, sm, pattern)) {
//		for(size_t i = 0; i < sm.size(); ++i) {
//			std::cout << "i===" << i  << "hit ===  \""<< sm.str(i) << "\"" << std::endl;
//		}
        /*
         * マクロボディサーフェイス名に置換後のIDがマッチすることは無いので
         * 該当パターンがなくなるまで置換を繰り返せば良い。
         */
		//mDebug() << "replaced ===" << std::string(sm[2].first, sm[2].second) << " by " << replacingString;
        newEq.replace(sm[2].first, sm[2].second, replacingString);
    }
//    mDebug() << "newEq===" << newEq;
    return newEq;
}

void geom::macro::checkParamLength(const std::vector<double> &params,
								   const std::vector<std::size_t> &validCounts,
								   const std::string &macroName)
{
	if(std::find(validCounts.begin(), validCounts.end(), params.size())
	  == validCounts.end()){
        throw std::invalid_argument(std::string("Acceptable numbers of arguments is {")
									+ utils::concat(validCounts, ", ") + "} for " + macroName
									+ ", given params=\""+ utils::concat(params, " ") + "\"");
	}
}


void geom::macro::replaceSurfaceInput(const std::vector<std::shared_ptr<geom::Surface> > &exSurfaces,
									  const std::unique_ptr<math::Matrix<4>> &matrix,
									  std::list<inp::DataLine>::iterator &it,
									  std::list<inp::DataLine> *surfInputList)
{
	if(matrix) {
		for(auto &surf: exSurfaces) surf->transform(*matrix.get());
	}
	auto filename = it->file;
	auto line = it->line;
	// eraseの戻り値は次の要素、
	// insertの引数は挿入場所の次の要素で、返り値は挿入された要素。
	// transformを掛けながらsurface入力へ追加していく
	it = surfInputList->erase(it);
	for(auto &surf: exSurfaces) {
		it = surfInputList->insert(it, inp::DataLine(filename, line, surf->toInputString()));
		++it;
	}
	--it;
}
/*!
 * \brief geom::macro::replacCelInput セルカード文字列*cellInputの論理式中のマクロボディを書き換える
 * \param macroBodyName マクロボディ名
 * \param numSurfaces　マクロボディが生成する面の数
 * \param cellInput　セルカード入力文字列へのポインタ
 *
 * マクロボディを基本的な面で置換するが、その時の名前は「もとのマクロボディ名」.「通し番号」
 * できまるためnumSurfacesが必要となる。
 *
 */
std::string geom::macro::replacCelInput(const std::string &macroBodyName,
                                        int numSurfaces,
                                        const std::string &cellInputString)
{
	// card.equation中の要素でsurfaceNameに完全一致する要素をreplacingで置き換える。
	auto cellcard = inp::CellCard::fromString(cellInputString);

//    mDebug() << "Replacing macrobody surface === " << macroBodyName;
//    mDebug() << "oldEq ===" << cellcard.equation;

	std::string replacingString = makeExpandedMacroBodyString(macroBodyName, numSurfaces);
	auto newEquation = makeReplacedEquation(macroBodyName, replacingString, cellcard.equation);

//	mDebug() << "newEq ===" << newEquation;
	// itはeraseしなくても直接成分を書き換えれば良い。
	std::string trclString;
	if(!cellcard.trcl.empty()) trclString = " trcl=(" + cellcard.trcl + ")";
	return cellcard.getHeaderString() + " " + newEquation + " " + cellcard.getParamsString() + trclString;
}
