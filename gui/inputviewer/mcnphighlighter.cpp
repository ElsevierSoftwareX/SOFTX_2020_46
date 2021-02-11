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
#include "mcnphighlighter.hpp"

#include "../../core/utils/message.hpp"

namespace {

const int OFF_SECTION = 1;
constexpr char COMMENT_LINE_NAME[] ="commentLine";
constexpr char META_NAME[] = "meta";

BaseHighlighter::FormatSetType createFormatMap(bool dark)
{
	static const std::map<std::string, QTextCharFormat> defaultFormatMap = BaseHighlighter::createTextCharFormatMap(false);
	static const std::map<std::string, QTextCharFormat> darkFormatMap = BaseHighlighter::createTextCharFormatMap(true);
	const std::map<std::string, QTextCharFormat> &fmap = dark ? darkFormatMap : defaultFormatMap;

	static const std::map<std::string, QRegExp> regMap = BaseHighlighter::createRegexpMap();

	// mcnp固有
	// TR以外のデータカード mode, sdef
	QRegExp metacardReg(R"(^\s*read *file)", Qt::CaseInsensitive);
	// FIXMEキーワードの前後をどう認識するかは要チェック SC1 direction とfmeshのrecを混同しないようにすべし。
	QRegExp datReg = QRegExp("mode|sdef|f[0-9]+[xyz]*|de[0-9]+|df[0-9]+|[^0-9]e[0-9]+|em[0-9]+|imp"
							 "|prdmp|nps|print|fmesh|rec|rzt|cyl|factor|out|vec|axs"
							 "|[eijk]ints|[eijk]mesh|erg|pos|nonu|totnu|void|sc[0-9]+|sp[0-9]+|si[0-9]+"
							 "|ds[0-9]+", Qt::CaseInsensitive);
	QRegExp designReg = QRegExp(R"(:[^-0-9)(# ](,[^-0-9)(#]){0,10})", Qt::CaseInsensitive); // particle designator
	QRegExp particleReg = QRegExp(R"(\b(n|p|e)\b)", Qt::CaseInsensitive);   // particle name
	//


	// NOTE 複数の部分パターンがマッチする場合(例 " x=0" ならリテラルかつparam)はあとに格納しているほうが優先される。
	// mapは 名前、フォーマット、正規表現、部分一致ならその部分の番号 のマップ

	// 多少正規表現を変えて対応しようが、いずれ適用順序問題は出る。正規表現を長くするよりは良い。
	// あとに出たフォーマットが上書きされる
	const BaseHighlighter::FormatSetType retvec
	{
		std::make_tuple("cellKeyword", fmap.at("cellKeyword"), regMap.at("cellKeyword"), 0),
		std::make_tuple("material",    fmap.at("material"),    regMap.at("material"), 0),
		std::make_tuple("tr",          fmap.at("transform"),   regMap.at("transform"), 0),
		std::make_tuple("surface",     fmap.at("surface"),     regMap.at("surface"),  0),
		std::make_tuple("compliment",     fmap.at("compliment"),     regMap.at("compliment"),  1),
		std::make_tuple(META_NAME,    fmap.at("meta"),        metacardReg, 0),
		std::make_tuple("param",       fmap.at("parameter"),   QRegExp(R"(([-\w\(\)]*)(:\w){0,1} *=)"), 1),
		std::make_tuple("literal",     fmap.at("literal"),     QRegExp(R"(\".*\")"), 0),
		std::make_tuple("data",        fmap.at("dataCard"),    datReg, 0),
		std::make_tuple("designator",  fmap.at("designator"), designReg, 0),
		std::make_tuple("particle",    fmap.at("designator"), particleReg, 1),  // particleのフォーマットはdesignatorと同じ。


		std::make_tuple("extinp",  fmap.at("extinp"),   QRegExp(R"(^[cC] \*.*$)"), 0),
		std::make_tuple("inlcom",  fmap.at("comment"),  QRegExp(R"(\$.*$)"), 0),
        std::make_tuple(COMMENT_LINE_NAME,  fmap.at("comment"),  QRegExp(R"(^ {0,4}[cC]( [^*].*$| +.*$|$))"), 0),
//		std::make_tuple(COMMENT_LINE_NAME,  fmap.at("comment"),  QRegExp(R"(^ {0,4}[cC]( [^*].*$| +.*$|$))"), 0),

	};
	return retvec;
}

// タプルはフォーマット、正規表現、行全体修飾するならtrueフラグ、captureの何番目部分か、の4つ
const BaseHighlighter::FormatSetType &chooseFormatMap(bool dark)
{
	static const McnpHighlighter::FormatSetType defaultMap = createFormatMap(false);
	static const McnpHighlighter::FormatSetType darkMap = createFormatMap(true);
	return dark ? darkMap : defaultMap;
}

}  // end anonymous namespace



McnpHighlighter::McnpHighlighter(QTextDocument *parent, bool isDark)
	: BaseHighlighter(parent, isDark)
{
	formatMap_ = chooseFormatMap(isDark_);
}

void McnpHighlighter::highlightBlock(const QString &text)
{
	if(text.isEmpty()) return;

	// MCNPに複数行コメントはないから適時フォーマットを適用していくだけ。
	for(auto &formatTuple: formatMap_) applyFormat(0, formatTuple, text);

	// 検索ヒット強調
	if(!word_.isEmpty()) {
		applyFormat(0, foundFormatInfo_, text);
	}
}


