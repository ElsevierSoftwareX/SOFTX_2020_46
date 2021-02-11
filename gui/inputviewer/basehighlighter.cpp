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
#include "basehighlighter.hpp"

namespace {
// TODO コイツはstring_utilsあたりへ移そう
void escapeMetaChars(QString *text)
{
	static const std::vector<QChar> metaChars{'\\', '*', '+', '.', '?', '{', '}', '(', ')', '[', ']', '^', '$', '-', '|', };
	for(const auto &ch: metaChars) {
		QString after("\\");
		after += ch;
		text->replace(ch, after);
	}
}
}



BaseHighlighter::BaseHighlighter(QTextDocument *parent, bool isDark)
    : QSyntaxHighlighter(parent), isDark_(isDark)
{}

void BaseHighlighter::setEmphasisWordInfo(const QString &word, bool isCaseSensitive, bool isWholeWord, bool isRegularExpression)
{
	word_ = word;
	QTextCharFormat foundFormat;
	QColor color = isDark_ ? QColor(Qt::yellow).lighter(60) : QColor(Qt::yellow).lighter(100);
	foundFormat.setBackground(color);
	QString regexStr = isWholeWord ? "\\b(" + word_ + ")\\b" : word_;
	int pos = isWholeWord ? 1 : 0;
	if(!isRegularExpression) escapeMetaChars(&regexStr);
	QRegExp foundReg(regexStr);
	foundReg.setCaseSensitivity(isCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
	foundFormatInfo_ = std::make_tuple("SearchResult", foundFormat, foundReg, pos);
}

namespace {
std::unordered_map<CNAME, QColor> createPalette(bool dark)
{
	const QColor redColor = (dark) ? QColor(Qt::darkRed).lighter(150): QColor(Qt::darkRed).lighter(130);
	const QColor greenColor= (dark) ? QColor(Qt::green).lighter(90) : QColor(Qt::darkGreen);
	const QColor blueColor = (dark) ? QColor(150, 150, 255) : QColor(0, 0, 255);
	const QColor cyanColor = (dark) ? QColor(Qt::cyan) : QColor(Qt::darkCyan);
	const QColor magentaColor =(dark) ? QColor(Qt::magenta).lighter(80) : QColor(Qt::darkMagenta);

	std::unordered_map<CNAME, QColor> retMap {
		{CNAME::DARKRED , redColor},
		{CNAME::GREEN , greenColor},
		{CNAME::BLUE, blueColor},
		{CNAME::CYAN , cyanColor},
		{CNAME::MAGENTA , magentaColor},
	};
	return retMap;
}
}  // end anonymous namespace


const std::unordered_map<CNAME, QColor> &BaseHighlighter::getPalette(bool isDark)
{
	static const std::unordered_map<CNAME, QColor> normalPalette = createPalette(false);
	static const std::unordered_map<CNAME, QColor> darkPalette = createPalette(true);
	return isDark ? darkPalette : normalPalette;
}

std::map<std::string, QTextCharFormat> BaseHighlighter::createTextCharFormatMap(bool dark)
{
	auto pal = BaseHighlighter::getPalette(dark);

	// コメント
	QTextCharFormat commentFormat;
	commentFormat.setForeground(pal.at(CNAME::DARKRED));
	commentFormat.setFontItalic(true);
	// 拡張入力
	QTextCharFormat extinpFormat;
	extinpFormat.setFontWeight(QFont::Bold);
	extinpFormat.setForeground(Qt::darkGray);
	// セクションタイトル
	QTextCharFormat sectionFormat;
	sectionFormat.setFontWeight(QFont::Bold);
	sectionFormat.setForeground(Qt::red);
	// メタカード
	QTextCharFormat metacardFormat;
	metacardFormat.setForeground(pal.at(CNAME::BLUE));
	// 中括弧、数式
	QTextCharFormat braceFormat;
	braceFormat.setForeground(pal.at(CNAME::CYAN));
	QTextCharFormat literalFormat = braceFormat;
	// その他パラメータ
	QTextCharFormat paramFormat;
	paramFormat.setForeground(pal.at(CNAME::MAGENTA));
	// 組み込み関数
	QTextCharFormat funcFormat;
	funcFormat.setForeground(pal.at(CNAME::GREEN));
	//MCNP・phits共通
	// surfaceニーモニック
	QTextCharFormat surfaceFormat;
	surfaceFormat.setForeground(pal.at(CNAME::BLUE));
	QTextCharFormat trFormat = paramFormat;
	QTextCharFormat matFormat = paramFormat;
	// like-but, #
	QTextCharFormat keyForm;
	keyForm.setFontWeight(QFont::Bold);

	// mcnp固有
	// TR以外のデータカード mode, sdef
	QTextCharFormat dataCardFormat = paramFormat;
	QTextCharFormat designatorFormat = surfaceFormat;



	return std::map<std::string, QTextCharFormat>{
		{"comment", commentFormat},
		{"compliment", keyForm},
		{"extinp", extinpFormat},
		{"section", sectionFormat},
		{"meta", metacardFormat},
		{"brace", braceFormat},
		{"parameter", paramFormat},
		{"literal", literalFormat},
		{"builtinFunc", funcFormat},
		{"surface", surfaceFormat},
		{"transform", trFormat},
		{"material", matFormat},
		{"cellKeyword", keyForm},
		{"dataCard", dataCardFormat},
		{"designator", designatorFormat}
	};
}

std::map<std::string, QRegExp> BaseHighlighter::createRegexpMap()
{
	QRegExp surfaceReg(" (x|y|z|p|px|py|pz|s|so|sx|xy|sz|c/x|c/y|c/z|cx|cy|cz|ca|"
					   "k/x|k/y|k/z|kx|ky|kz|sq|gq|tx|ty|tz|ta|box|rpp|sph|rcc|"
					   "rhp|hex|rec|trc|ell|wed|arb|r0|r1|xyz)(\\s|$)", Qt::CaseInsensitive);
	QRegExp matReg = QRegExp(R"((^ {,4}m[0-9]+|mat\[[0-9]+\])|nlib|plib|nplib|elib|hlib)", Qt::CaseInsensitive);
	QRegExp keyReg(R"( (like|but) )", Qt::CaseInsensitive);
	QRegExp compReg(R"((#)[a-zA-Z0-9( ]+)");

	return std::map<std::string, QRegExp> {
		{"surface", surfaceReg},
		{"transform", QRegExp(R"(^ {,4}\*{,1}tr[0-9]+)", Qt::CaseInsensitive)},
		{"material", matReg},
		{"cellKeyword", keyReg},
		{"compliment", compReg},
	};
}

// 行中のテキストに対して再帰的にフォーマット適用を実行したいので関数としてhighlightBlockから外へだした。
void BaseHighlighter::applyFormat(int pos, const FormatTuple &formatTuple, const QString &text)
{
	if(pos >= text.size()) return;
	//const QString *name = &std::get<0>(formatTuple);
	const QTextCharFormat *form = &std::get<1>(formatTuple);
	const QRegExp *reg = &std::get<2>(formatTuple);
	int emphPos = std::get<3>(formatTuple);

	// ここで検索！	// posはあくまでヒットした部分の先頭でしかない
	pos = reg->indexIn(text, pos);

	if(pos != -1) {
		auto len = reg->capturedTexts().at(emphPos).length();
		int start = pos;   // フォーマット適用位置の先頭はposとは限らない(subexpressionの時)
		if(emphPos != 0) {
			start = text.indexOf(reg->cap(emphPos), pos);
			pos = start + reg->cap(emphPos).size() + 1;
		} else {
			pos += reg->matchedLength();
		}
		this->setFormat(start, len, *form);
		applyFormat(pos, formatTuple, text);
	}
}
