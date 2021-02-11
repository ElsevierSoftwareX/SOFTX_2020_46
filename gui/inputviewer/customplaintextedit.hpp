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
#ifndef CUSTOMPLAINTEXTEDIT_HPP
#define CUSTOMPLAINTEXTEDIT_HPP

#include <functional>
#include <unordered_set>
#include <vector>
#include <QIcon>
#include <QContextMenuEvent>
#include <QPixmap>
#include <QPlainTextEdit>

#include "../../core/io/input/mcmode.hpp"

class CustomPlainTextEdit;
class BaseHighlighter;

class LineNumberArea : public QWidget
{
public:
	LineNumberArea(CustomPlainTextEdit *editor);
	QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	CustomPlainTextEdit *editor_;
};



// スクロール中でもwheel eventをignoreしないカスタムPlainTextEdit
class CustomPlainTextEdit : public QPlainTextEdit
{
	Q_OBJECT
public:
    explicit CustomPlainTextEdit(QWidget *parent = nullptr, McMode mode = McMode::AUTO);

	void lineNumberAreaPaintEvent(QPaintEvent *event);
	int lineNumberAreaWidth();
    // highlighterがwordを強調表示するようにアップデートする
    void updateHighlighter(const QString &word, bool isCaseSensitive, bool isWordUnit, bool isRegularExpression);
    void setFatalLine(int line);  // lineにfatalアイコンを追加する。
	void clearFatalLine();
    void commentRegion();

    const std::unordered_set<int> &fatalLines() {return fatalLines_;}

signals:
	void requestOpenFile(QString);

protected:
	void wheelEvent(QWheelEvent *event)override;
	void resizeEvent(QResizeEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
    void changeEvent(QEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

private slots:
	void updateLineNumberAreaWidth(int newBlockCount);
    // 現在行の強調は検索結果との一致強調と相性が良くないので使わない
//	void highlightCurrentLine();
	void updateLineNumberArea(const QRect &rect, int dy);


private:
	QWidget *lineNumberArea;
	McMode mode_;
	BaseHighlighter *highlighter_;
    bool isDark_;
    QPixmap fatalPix_;
    std::function<bool(const QString&)> isPrefixComment_;

    std::unordered_set<int> fatalLines_;
    void rescalePixmaps() {fatalPix_ = fatalPix_.scaledToHeight(0.7*QWidget::fontMetrics().height());}
    void commentInOutLines(const std::vector<int> &lines, QTextCursor &cursor);
    void replaceLine(int lineNumber, const QString& text);
};

#endif // CUSTOMPLAINTEXTEDIT_HPP
