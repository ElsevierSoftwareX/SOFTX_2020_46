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
#ifndef PHITSHIGHLIGHTER_HPP
#define PHITSHIGHLIGHTER_HPP

#include <QRegExp>
#include <QString>
#include <QTextDocument>

#include "basehighlighter.hpp"

/*
 * PHITSとMCNPでハイライターはの違いは
 * ・非共通キーワード
 * ・複数行コメントアウトがMCNPにはない
 * ・行内コメント継続行記号の違い
 *
 */

class PhitsHighlighter : public BaseHighlighter
{
public:
//  // 型のusingも基底へ
//	// フォーマット名、フォーマット、適用正規表現、部分正規表現の場合ハイライトする部分の番号
//	using FormatTuple = std::tuple<QString, QTextCharFormat, QRegExp, int>;
//	using FormatSetType = std::vector<FormatTuple>;

	PhitsHighlighter(QTextDocument *parent = 0, bool isDark = false);

	// QSyntaxHighlighrer::highlightBlockの実装
	virtual void highlightBlock(const QString &text) override final;

private:

//	// 検索結果の強調に関するメンバ変数は基底クラスへ。
//	bool isDark_;
//	QString word_;                  // 強調する単語
//	FormatTuple foundFormatInfo_;  // 検索で発見した単語に適用するフォーマットのタプル

//	// 構文強調用のフォーマットタプルを保持する。
//	const FormatSetType formatMap_;

	// phitsでは複数行コメントがある。
	FormatTuple commentTuple_;   // 複数行コメントへの適用フォーマット
	FormatTuple metacardTuple_;  // 複数行コメント内にmetaカード行がある場合に適用するフォーマット

	// TODO 正規表現はconstexprリテラルでどこかにまとめたい
    // offsection, section表現ははハイライト判定時に毎回使うのでメンバで保持する
//	const QRegExp offSecReg_ = QRegExp(R"(^ *\[[-0-9a-zA-Z ]+\] *off\s*$)", Qt::CaseInsensitive);
    const QRegExp offSecReg_ = QRegExp(R"(^ *\[[-0-9a-zA-Z ]+\] *off\s*$|^ {0,4}qp:)", Qt::CaseInsensitive);
    const QRegExp offFileReg_ = QRegExp(R"(^ *\[\s*e\s*n\s*d\s*\]\s*$|^ {0,4}q:)", Qt::CaseInsensitive);
	const QRegExp secReg_ = QRegExp(R"(^ *\[[-0-9a-zA-Z ]+\] *(\$.*|$))", Qt::CaseInsensitive);
};

#endif // PHITSHIGHLIGHTER_HPP
