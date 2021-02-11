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
#include "phitshighlighter.hpp"

#include <QColor>
#include <QRegExp>
#include <QTextCharFormat>
#include "../core/utils/message.hpp"

namespace {
const int OFF_SECTION = 1;  // "[section] off" や　qp: での複数行コメントを示す値
const int OFF_FILE = 2;     // "[end]" やq: でのファイル末尾までコメントを示す値
constexpr char COMMENT_LINE_NAME[] ="commentLine";
constexpr char META_NAME[] = "meta";


BaseHighlighter::FormatSetType createFormatMap(bool dark)
{
	static const std::map<std::string, QTextCharFormat> defaultFormatMap = BaseHighlighter::createTextCharFormatMap(false);
	static const std::map<std::string, QTextCharFormat> darkFormatMap = BaseHighlighter::createTextCharFormatMap(true);
	const std::map<std::string, QTextCharFormat> &fmap = dark ? darkFormatMap : defaultFormatMap;

	static const std::map<std::string, QRegExp> regMap = BaseHighlighter::createRegexpMap();

	QRegExp metacardReg(R"(^\s*(infl:|set:))", Qt::CaseInsensitive);

	QRegExp funcReg("\\b(c[0-9]{1,2}|float|int|abs|exp|log|log10|max|min|mod|nint|"
                    "sign|sqrt|acos|asin|atan|atan2|cos|sin|tan|cosh|sinh|tanh|pi)\\b", Qt::CaseInsensitive);

	// NOTE 複数の部分パターンがマッチする場合(例 " x=0" ならリテラルかつparam)はあとに格納しているほうが優先される。
	// mapは 名前、フォーマット、正規表現、部分一致ならその部分の番号 のマップ

	// 多少正規表現を変えて対応しようが、いずれ適用順序問題は出る。正規表現を長くするよりは良い。
	// あとに出たフォーマットが上書きされる
	const PhitsHighlighter::FormatSetType retvec
	{
		std::make_tuple("keyword",   fmap.at("cellKeyword"), regMap.at("cellKeyword"), 0),
		std::make_tuple("material",  fmap.at("material"),    regMap.at("material"), 0),
		std::make_tuple("transform", fmap.at("transform"),   regMap.at("transform"), 0),
		std::make_tuple("surface",   fmap.at("surface"),     regMap.at("surface"),  0),
		std::make_tuple("compliment",     fmap.at("compliment"),     regMap.at("compliment"),  1),
		std::make_tuple("function",  fmap.at("builtinFunc"), funcReg, 1),
		std::make_tuple("section",   fmap.at("section"),     QRegExp(R"(^ *\[[-0-9a-zA-Z ]+\] *(\$.*|$))"), 0),
		std::make_tuple(META_NAME,  fmap.at("meta"),         metacardReg, 0),
		std::make_tuple("brace",     fmap.at("brace"),        QRegExp(R"(^ *\S+([\{\[].*[\}\]]))"), 1),
		std::make_tuple("param",     fmap.at("parameter"),    QRegExp(R"(([-\w\(\)]*) *=)"), 1),
		std::make_tuple("literal",   fmap.at("literal"),      QRegExp(R"(\".*\")"), 0),

		std::make_tuple("extinp",  fmap.at("extinp"),   QRegExp(R"(^[cC] \*.*$)"), 0),
		std::make_tuple("inlcom",  fmap.at("comment"),  QRegExp(R"([\$!%].*$)"), 0),
//		std::make_tuple(COMMENT_LINE_NAME,  commentFormat,  QRegExp(R"(^ {0,4}[cC]( [^*]+| +).*$)"), 0),
		std::make_tuple(COMMENT_LINE_NAME,  fmap.at("comment"),  QRegExp(R"(^ {0,4}[cC]( [^*].*$| +.*$|$))"), 0),
	};
	return retvec;
}

// タプルはフォーマット、正規表現、行全体修飾するならtrueフラグ、captureの何番目部分か、の4つ
const BaseHighlighter::FormatSetType &chooseFormatMap(bool dark)
{
	static const PhitsHighlighter::FormatSetType defaultMap = createFormatMap(false);
	static const PhitsHighlighter::FormatSetType darkMap = createFormatMap(true);
	return dark ? darkMap : defaultMap;
}
}  // end anonymous namespace





PhitsHighlighter::PhitsHighlighter(QTextDocument *parent, bool isDark)
	: BaseHighlighter(parent, isDark)
{
	formatMap_ = chooseFormatMap(isDark_);
	for(auto &tup: formatMap_) {
		if(std::get<0>(tup) == COMMENT_LINE_NAME) {
			commentTuple_ = tup;
		} else if (std::get<0>(tup) == META_NAME) {
			metacardTuple_ = tup;
		}
	}
	assert(!std::get<0>(commentTuple_).isEmpty());
	assert(!std::get<0>(metacardTuple_).isEmpty());
}

// 複数行コメントはQSyntaxHighlightrerリファレンスを見ること
void PhitsHighlighter::highlightBlock(const QString &text)
{
    //if(text.isEmpty()) return;

	/*
	 * PHITSでは複数行コメントがある。
	 * 通常行（複数行コメントではない）の条件は以下のどちらか
     * ・前の行が通常行で       かつ  offセクション開始行ではない
     * ・前の行がOFF_SECTIONで  かつ 新しいセクションが始まる
	 */

	auto previousState = previousBlockState();
    if (text.isEmpty()) {
            //mDebug() << "EMPTY  ";
            setCurrentBlockState(previousBlockState()); // 空白行は前の行の状態を引き継ぐ
    } else if(previousState == OFF_FILE || offFileReg_.indexIn(text, 0) != -1) {
        setCurrentBlockState(OFF_FILE);
        this->setFormat(0, text.size(), std::get<1>(commentTuple_));
    } else if((previousState != OFF_SECTION && offSecReg_.indexIn(text, 0) == -1)
            || (previousState == OFF_SECTION && secReg_.indexIn(text, 0) != -1)) {
		// 通常行
		setCurrentBlockState(0);
		for(auto &formatTuple: formatMap_) {
			applyFormat(0, formatTuple, text);
		}
        //mDebug() << "NORMAL "<< text;
    } else {
		// offセクション開始or継続
		setCurrentBlockState(OFF_SECTION);

        // offセクションならメタカードも当然無効化
//		// offセクションでもmetacardは生きているのでハイライトを適用する。
//		if(std::get<2>(metacardTuple_).indexIn(text, 0) != -1) {
//			for(auto &formatTuple: formatMap_) {
//				applyFormat(0, formatTuple, text);
//			}
//		} else {
//			this->setFormat(0, text.size(), std::get<1>(commentTuple_));
//		}
        this->setFormat(0, text.size(), std::get<1>(commentTuple_));
        //mDebug() << "OFFSEC "<< text;
	}

	// 検索ヒット強調
    if(!word_.isEmpty() && !text.isEmpty()) {
		applyFormat(0, foundFormatInfo_, text);
	}
}




