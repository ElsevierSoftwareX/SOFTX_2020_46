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
#ifndef BASEHIGHLIGHTER_HPP
#define BASEHIGHLIGHTER_HPP

#include <tuple>
#include <unordered_map>
#include <vector>

#include <QRegExp>
#include <QString>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextDocument>

/*
 * ハイライターの動作は
 * ・QSyntaxHighlighter::highlightBlockの実装
 *
 */

// C++11 ではstd::hashを定義しないとenumをunordered_mapのキーにできない。
// C++14ではOK. g++6からはOK
#if defined(__GNUC__) && __GNUC__ < 6 && !defined(DEFINE_ENUMHASH) && !defined(__clang__)
#define DEFINE_ENUMHASH
namespace std {
  template <class T>
  struct hash {
    static_assert(is_enum<T>::value, "This hash only works for enumeration types");
    size_t operator()(T x) const noexcept {
      using type = typename underlying_type<T>::type;
      return hash<type>{}(static_cast<type>(x));
    }
  };
}
#endif
// ハイライターでは以下の5色を使う。darkの場合lighterな色を使い、通常時はdark*を使う。
// foreground, backgroundはcssが定義するのでこのenumには含めない。
enum class CNAME: int {DARKRED, GREEN, BLUE, CYAN, MAGENTA};

class BaseHighlighter : public QSyntaxHighlighter
{
public:
// フォーマット名、フォーマット、適用正規表現、部分正規表現の場合ハイライトする部分の番号
	using FormatTuple = std::tuple<QString, QTextCharFormat, QRegExp, int>;
	using FormatSetType = std::vector<FormatTuple>;

	// ハイライターはparentにQTextDocumentをセットする。
	BaseHighlighter(QTextDocument *parent = 0, bool isDark = false);
	virtual ~BaseHighlighter(){;}
	// (検索結果でヒットした時に)強調する単語をセットする。
	void setEmphasisWordInfo(const QString &word, bool isCaseSensitive, bool isWholeWord, bool isRegularExpression);
	// 個別のMCNP/PHITSハイライターで実装する。QSyntaxHighliterのvirtual関数
	virtual void highlightBlock(const QString &text) override = 0;
	// モード(dark or not)に応じたカラーパレットを返す。

	static const std::unordered_map<CNAME, QColor> &getPalette(bool isDark);
	// QTextCharFormatはphits/mcnpで共通なのでこのマップを生成する。
	static std::map<std::string, QTextCharFormat> createTextCharFormatMap(bool dark);
	static std::map<std::string, QRegExp> createRegexpMap();
protected:
	// 構文強調用のフォーマットタプルを保持する。
	FormatSetType formatMap_;
	bool isDark_;
	QString word_;                  // 強調する単語
	FormatTuple foundFormatInfo_;  // 検索で発見した単語に適用するフォーマットのタプル

	// フォーマットの適用。highlightBlock内で再帰実行させるためにhighlightBlock内から独立させた。
	// 位置、適用するフォーマットのタプル、 対象テキスト
	void applyFormat(int pos, const FormatTuple &formatTuple, const QString &text);
};






#endif // BASEHIGHLIGHTER_HPP
