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
#ifndef FILETABITEM_HPP
#define FILETABITEM_HPP

#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QString>
#include <QSyntaxHighlighter>
#include <QTimer>
#include <QTextDocument>
#include <QWidget>

#include "customplaintextedit.hpp"
#include "../../core/io/input/mcmode.hpp"

namespace Ui {
class FileTabItem;
}


// QPlainTextEditはCtrl+ホイールで拡大されない
// read-onlyにするか自分で実装するか、らしい
// https://bugreports.qt.io/browse/QTBUG-30784
// なのでCtrl押している時はQPlainTextEdit::wheelEventを呼ばず
// 独自singalをemitするクラスCustomPlainTextEditを作った。

class FileTabItem : public QWidget
{
	Q_OBJECT

public:
    FileTabItem(QWidget *parent = nullptr, const QString &filename = "", McMode mode = McMode::AUTO);
    ~FileTabItem() override;
	QTextDocument *document();
	QString canonicalFilePath() const;

	void copy() {plainTextEdit_->copy();}
	void cut() {plainTextEdit_->cut();}
	void paste() {plainTextEdit_->paste();}
	void findAct() {hideSearchWidgets(false);}
    void undo() {plainTextEdit_->undo();}
	void redo() {plainTextEdit_->redo();}
    void commentRegion() {plainTextEdit_->commentRegion();}

	void save();
	void clear();
	void reload();
    void setFatalInLineEdit(int line);
	void handleCtrlFkey();  // Ctrl-Fキーに対応した処理。parentから呼ばれることもあるのでpublicにおく

	void setFont(const QFont &font);

protected:
	void keyPressEvent(QKeyEvent *event) override;
    //void changeEvent(QEvent *event) override;

signals:
    void fileSaved(QString absFilePath);
    void fileTabModificationChanged(bool);
    void requestOpenFile(QString);

private:
	Ui::FileTabItem *ui;
	CustomPlainTextEdit *plainTextEdit_;
	QString absFilePath_;
	bool isHighlighted_;

	// QLineEdit::textEditedが連発されないようにウェイトをいれて検索を実行する。
	QTimer *waitTimer_;
	QString searchStr_;

	void advanceCursor(int num);
	QTextDocument::FindFlags getFindFlags() const;


private slots:
    void hideSearchWidgets(bool flagHide);
	// &QLineEdit::textEditedから検索したい文字列を受け取るスロット。入力に応じてインクリメンタル検索する
	bool receiveSearchingText(QString text);
	// Ctrl-Fや検索ボタンを押した時に呼ぶ。lineditに入っているtextで次の位置を検索する。
	void findWord(bool isForward);
	bool replaceFoundWord();
	void replaceFoundWordAndSearch();
	void replaceAllFoundWords();
	void updateHighlighter();
	void cancelHighighter();
	void relayTextEditedWithDelay(QString str);


};

#endif // FILETABITEM_HPP
