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
#include "filetabitem.hpp"
#include "ui_filetabitem.h"

#include <fstream>

#include <QFile>
#include <QRegExp>
#include <QSignalBlocker>
#include <QSyntaxHighlighter>
#include <QTextStream>
#include <QTimer>

#include "../globals.hpp"
#include "../loghighlighter.hpp"
#include "../../core/io/input/mcmode.hpp"
#include "../../core/utils/system_utils.hpp"
#include "../../core/utils/message.hpp"

namespace {
const int WAITING_TIME_MS = 250;

// 第四引数がtrueならtextを正規表現文字列として検索
bool findTextOrRegex(CustomPlainTextEdit *plainTextEdit,
					 const QString &text,
					 QTextDocument::FindFlags flags,
					 bool isRegex)
{
	if(isRegex) {
		return plainTextEdit->find(QRegExp(text), flags);
	} else {
		return plainTextEdit->find(text, flags);
	}
}
}  // end anonymous namespace

FileTabItem::FileTabItem(QWidget *parent, const QString &filename, McMode mode) :
	QWidget(parent), ui(new Ui::FileTabItem), absFilePath_(filename), isHighlighted_(false)
{
	ui->setupUi(this);
	ui->verticalLayout->setSpacing(2);
	waitTimer_ = new QTimer(this);
	// ここでCustomPlainTextEdit生成時にハイライターを選択できるようにMcModeを与える。guessで不明ならphitsにする。
	if(mode == McMode::AUTO) mode = inp::guessMcModeFromSuffix(filename.toStdString());
	if(mode == McMode::AUTO) mode = McMode::PHITS;
	plainTextEdit_ = new CustomPlainTextEdit(this, mode);
	plainTextEdit_->setReadOnly(false);
    plainTextEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->verticalLayout->insertWidget(0, plainTextEdit_);

	connect(plainTextEdit_, SIGNAL(requestOpenFile(QString)), this, SIGNAL(requestOpenFile(QString)));  // 右クリックから開くシグナル
    connect(plainTextEdit_, SIGNAL(modificationChanged(bool)), this, SIGNAL(fileTabModificationChanged(bool)));

	// 検索関係ウィジェット
	hideSearchWidgets(true);  // 初期状態では検索widget非表示
	ui->pushButtonCloseSearch->setIcon(QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    ui->pushButtonDown->setIcon(QApplication::style()->standardIcon(QStyle::SP_TitleBarUnshadeButton));
    ui->pushButtonUp->setIcon(QApplication::style()->standardIcon(QStyle::SP_TitleBarShadeButton));
	// 検索widgetを閉じる時は内容をクリア
	connect(ui->pushButtonCloseSearch, &QPushButton::pressed,
			this, [=](){
				this->hideSearchWidgets(true);
				ui->lineEditSearch->clear();
				ui->lineEditReplace->clear();
				updateHighlighter();
			});
	/*
	 *  QLineEdit::textEditedをそのままFileTabItem::receiveSearchingTextにconnectすると
	 * 一文字入力ごとに検索が実行され、サイズの大きいファイルで重くなりすぎるので、
	 * 間にタイマーによるウェイトを入れ、待機中に再度textEditが来ると待機時間延長にする。
	 */
	//connect(ui->lineEditSearch, &QLineEdit::textEdited, this, &FileTabItem::receiveSearchingText);
	connect(ui->lineEditSearch, &QLineEdit::textEdited, this, &FileTabItem::relayTextEditedWithDelay);
	connect(waitTimer_, &QTimer::timeout, this, [=](){receiveSearchingText(this->searchStr_);});

	// ボタンを押した瞬間はfocusはボタンへ写っているので検索lineEditへfocusを手動で移してから検索する。
	connect(ui->pushButtonDown, &QPushButton::pressed, this, [=](){ui->lineEditSearch->setFocus(); findWord(true);});
	connect(ui->pushButtonUp, &QPushButton::pressed, this, [=](){ui->lineEditSearch->setFocus(); findWord(false);});
	connect(ui->checkBoxWordUnit, &QCheckBox::stateChanged, this, &FileTabItem::updateHighlighter);
	connect(ui->checkBoxCaseDpendent, &QCheckBox::stateChanged, this, &FileTabItem::updateHighlighter);

    reload();
}

FileTabItem::~FileTabItem() {
	delete ui;
}


QString FileTabItem::canonicalFilePath() const {return absFilePath_;}


void FileTabItem::keyPressEvent(QKeyEvent *event)
{
    /*
     * キーイベントは基本的にメンバ変数のplainTextEdit_から送られてくる。
     * 故に、QPlainTextEditがデフォルトで処理するCtrl-zのundoなどはここで
     * 処理すると2回実施することになるので、ここでは何もしない。
     * QPlainTextEditが実装済みのショートカット
     * ・Ctrl-z          undo
     * ・Ctrl-Shift-z    redo
     * ・Ctrl-c          copy
     * ・Ctrl-x          cut
     * ・Ctrl-v          paste
     * などファイル編集に関わるものはだいたい全部実装されている。
     */
    auto key = event->key();
    if(event->modifiers() & Qt::ControlModifier) {
        if (key == Qt::Key_S) {
            this->save();
        } else 	if(key == Qt::Key_R) {
            this->reload();
        } else if(key == Qt::Key_F) {
			this->handleCtrlFkey();
        } else if(key == Qt::Key_Slash) {
            this->commentRegion();
        }
	} else if(key == Qt::Key_Enter || key == Qt::Key_Return){  // Enterはnumpad右下のenter, 大きなEnterはReturn。
		// サーチlineEditフォーカス中にEnterおしたらfindNext,
		if(ui->lineEditSearch->hasFocus()) {
			this->findWord(true);
		}
	}

	QWidget::keyPressEvent(event);
}

void FileTabItem::advanceCursor(int num)
{
	if(num == 0) return;
	QTextCursor cursor = plainTextEdit_->textCursor();
	if(num > 0) {
		cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, num);
	} else {
		cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, -num);
	}
	plainTextEdit_->setTextCursor(cursor);
}

QTextDocument::FindFlags FileTabItem::getFindFlags() const
{
	QTextDocument::FindFlags flags;
    if(ui->checkBoxCaseDpendent->isChecked()) flags = flags | QTextDocument::FindCaseSensitively;
    if(ui->checkBoxWordUnit->isChecked()) flags = flags |QTextDocument::FindWholeWords;
	return flags;
}

void FileTabItem::updateHighlighter()
{
	plainTextEdit_->updateHighlighter(ui->lineEditSearch->text(),
									  ui->checkBoxCaseDpendent->isChecked(),
									  ui->checkBoxWordUnit->isChecked(),
									  ui->checkBoxRegularExpression->isChecked());
	isHighlighted_ = true;
}

void FileTabItem::cancelHighighter()
{
	if(!isHighlighted_) return;
	plainTextEdit_->updateHighlighter("",
									  ui->checkBoxCaseDpendent->isChecked(),
									  ui->checkBoxWordUnit->isChecked(),
									  ui->checkBoxRegularExpression->isChecked());
	isHighlighted_ = false;
}

// wait中(=timerアクティブ中)に再度呼ばれた場合、待機時間を延長し、strは上書きする
void FileTabItem::relayTextEditedWithDelay(QString str)
{
	searchStr_ = str;
	// timer動作中なら更新
	if(waitTimer_->isActive()) {
		waitTimer_->stop();
	}
	waitTimer_->setSingleShot(true);  // singleShotにしないと延々とsignalが出てしまう。
	waitTimer_->start(WAITING_TIME_MS);
}

void FileTabItem::handleCtrlFkey()
{
	// サーチlineEditフォーカス中にCtrl-Fで次を検索。フォーカスが外れていたら中身をクリアしてフォーカスを移す
	if(!ui->lineEditSearch->hasFocus()) {
		ui->lineEditSearch->clear();
		// 今単語を選択していればコレをlineEditにセットする。
		auto text = plainTextEdit_->textCursor().selectedText();
		if(!text.isEmpty()) {
			ui->lineEditSearch->setText(text);
			updateHighlighter();
		}
	}
	this->findWord(true);
}

void FileTabItem::setFont(const QFont &font)
{
	QWidget::setFont(font);
	plainTextEdit_->setFont(font);
}


void FileTabItem::hideSearchWidgets(bool flagHide)
{
    if(flagHide) {
        ui->lineEditReplace->clear();
        ui->lineEditSearch->clear();
		cancelHighighter();
    }
    ui->frame->setHidden(flagHide);
    ui->checkBoxCaseDpendent->setHidden(flagHide);
    ui->checkBoxWordUnit->setHidden(flagHide);
    ui->pushButtonDown->setHidden(flagHide);
    ui->pushButtonUp->setHidden(flagHide);
    ui->pushButtonCloseSearch->setHidden(flagHide);
    ui->lineEditSearch->setHidden(flagHide);

    ui->lineEditReplace->setHidden(flagHide);
    ui->pushButtonReplace->setHidden(flagHide);
    ui->pushButtonReplaceAndSearch->setHidden(flagHide);
    ui->pushButtonReplaceAll->setHidden(flagHide);
}




/*
 * スクロールバー表示中Ctrl-wheelが効かない問題はそのそもeventがignoreされているようで
 * eventFilterでもキャッチできない。よってここでeventfilterを使っても無駄。
 */



void FileTabItem::clear() {	plainTextEdit_->clear();}

void FileTabItem::reload()
{
    // ここにシグナルブロッカーを入れるとtextChanged()以外のシグナルもブロックされて表示が崩れる。
	this->clear();
	QFile inpFile(absFilePath_);
	if (!inpFile.exists()) mFatal("No such a file =", absFilePath_);
	inpFile.open(QFile::ReadOnly | QFile::Text);
	QTextStream ts(&inpFile);
    ts.setCodec("UTF-8");  // これがないとwinではsjisで読み取ってしまう
	plainTextEdit_->appendPlainText(ts.readAll());
	QTextCursor cursor(plainTextEdit_->document());
	cursor.movePosition(QTextCursor::Start);
	plainTextEdit_->setTextCursor(cursor);
	plainTextEdit_->document()->clearUndoRedoStacks();
    plainTextEdit_->document()->setModified(false);
    plainTextEdit_->setFocus();
}

void FileTabItem::setFatalInLineEdit(int line)
{
    this->plainTextEdit_->setFatalLine(line);
}


void FileTabItem::save()
{
	QFile inpFile(absFilePath_);
	if(!inpFile.open(QFile::WriteOnly | QFile::Text)) {
		mFatal("File =", absFilePath_, "is not writable. File save failed.");
	}
	QTextStream ts(&inpFile);
	ts.setCodec("UTF-8");
	ts << plainTextEdit_->toPlainText();

    plainTextEdit_->document()->setModified(false);
	plainTextEdit_->clearFatalLine();
    emit fileSaved(absFilePath_);
}


// LineEdit更新によるインクリメンタル検索
bool FileTabItem::receiveSearchingText(QString text)
{
	// 空なら検索せず、ハイライト解除のみ
	if(text.isEmpty()) {
		cancelHighighter();
		return false;
	}

	// 正規表現にチェックがついていなければメタ文字はエスケープする
//	if(!ui->checkBoxRegularExpression->isChecked()) escapeMetaChars(&text);


	static int prevSearchNum = 0;
	// QPlainTextEdit::find()は検索文字列の後ろにカーソルがくるので、
	// インクリメンタルサーチするにはカーソルを戻す必要がある。
	// 前回の検索文字数だけ戻すわけだが、それは今の検索文字列数-1でだいたいOK...と思ったがデクリメンタルなサーチもあり得る
	advanceCursor(-prevSearchNum);
	bool result = findTextOrRegex(plainTextEdit_, text, getFindFlags(), ui->checkBoxRegularExpression->isChecked());
	if(!result) {
		advanceCursor(prevSearchNum); // 失敗時は検索前に戻したカーソルを進める。
	}
	prevSearchNum = text.size();
	// 検索の成功不成功に関わらず単語ハイライトは実施する。
	updateHighlighter();
//    bool isCaseSensitive = ui->checkBoxCaseDpendent->isChecked();
//    bool isWordUnit = ui->checkBoxWordUnit->isChecked();
//    plainTextEdit_->updateHighlighter(ui->lineEditSearch->text(), isCaseSensitive, isWordUnit);

	return result;
}



/*
 * 検索する場合↑のlineEditから自動検索するとカーソルは該当部分の後ろに来る。（QPlainText::findの仕様）
 * だからCtrl-F(あるいはボタン)で前方検索する時はQplainText::findを実行した後
 * 追加で検索できるようにする必要が有る。
 *
 */
// 前方あるいは後方検索
void FileTabItem::findWord(bool isForward)
{
	// lineEditが非表示状態なら表示する。
    if(ui->lineEditSearch->isHidden()) hideSearchWidgets(false);
	// lineEditにフォーカスが行っていない場合フォーカスを移すだけ。
    if(!ui->lineEditSearch->hasFocus()) {
        ui->lineEditSearch->setFocus();
        return;
    }

	// 空欄なら検索せずハイライト解除のみ
	if(ui->lineEditSearch->text().isEmpty()) {
		cancelHighighter();
		return;
	}

	QString text = ui->lineEditSearch->text();
	auto oldCursor = plainTextEdit_->textCursor();

	// 検索オプションフラグの設定
	QTextDocument::FindFlags flags = getFindFlags();
    if(!isForward) flags = flags | QTextDocument::FindBackward;



	// NOTE 現在行頭(pos=0)付近にカーソルが有る場合にbackward検索をするとなぜか１個飛ばしになるQtバグ
	// これはQtのバグ。 https://bugreports.qt.io/browse/QTBUG-48035
	// 発現する条件はアンカーが行頭にあること。
	// 当面解決しなさそうなので後方検索で2個前にヒットしたら1個戻すという対処を取る。
	// この回避策はバグフィックスされたら逆にバグになるので要注意
#ifdef WORKAROUND_QTBUG48035
	auto cursor = plainTextEdit_->textCursor();
	int anchorPositionInblock = cursor.anchor() - cursor.block().position();
	bool bugCorrectionRequired = (anchorPositionInblock == 0) && !isForward;

	// バックワード検索バグ+二行目の組み合わせが発生した場合の補正(1行戻して前方検索)はここで対応。
	if(bugCorrectionRequired && plainTextEdit_->textCursor().blockNumber() == 1) {
		auto tmpcursor = plainTextEdit_->textCursor();
		tmpcursor.movePosition(QTextCursor::Start);
		plainTextEdit_->setTextCursor(tmpcursor);
		findWord(true);
		return;
	}
#endif


	if(findTextOrRegex(plainTextEdit_, text, flags, ui->checkBoxRegularExpression->isChecked())) {
#ifdef WORKAROUND_QTBUG48035
		if(bugCorrectionRequired) findWord(true);  // 行頭バグを避けるために一個後に検索を移す
#endif
	} else {
		// 後（前）にtextが見つからなければカーソルをtextEdit先頭（末尾）に戻し再度検索。該当なしなら元の場所に戻す
        auto edge = isForward ? QTextCursor::Start : QTextCursor::End;
		auto edgeCursor = plainTextEdit_->textCursor();
		edgeCursor.movePosition(edge, QTextCursor::MoveAnchor,1);
		plainTextEdit_->setTextCursor(edgeCursor);
		if(!findTextOrRegex(plainTextEdit_, text, flags, ui->checkBoxRegularExpression->isChecked())) {
            plainTextEdit_->setTextCursor(oldCursor);
        }
    }
}

// 検索した文字列を置換
bool FileTabItem::replaceFoundWord()
{
	if(ui->lineEditSearch->text().isEmpty()) return false;  // 空欄なら検索しない

	// 現在選択中のテキストがsearch文字列と一致しなければ何もしない。
	//if(plainTextEdit_->textCursor().selectedText() != ui->lineEditSearch->text()) return false;
	// と思ったが正規表現の置換を有効化するにはこれではだめなので無効化する。

	// 一致していれば置換
	plainTextEdit_->textCursor().removeSelectedText();
	plainTextEdit_->textCursor().insertText(ui->lineEditReplace->text());
	return true;
}

void FileTabItem::replaceFoundWordAndSearch()
{
	// replaceFoundWordはemptyチェックをしている
	replaceFoundWord();
	this->receiveSearchingText(ui->lineEditSearch->text());
}

void FileTabItem::replaceAllFoundWords()
{
	auto currentCursor = plainTextEdit_->textCursor();
	while(this->receiveSearchingText(ui->lineEditSearch->text())) {
		replaceFoundWord();
	}
	// 先頭に戻ってもう一度置換を実行
	auto tmpcursor = currentCursor;
	tmpcursor.movePosition(QTextCursor::Start);
	plainTextEdit_->setTextCursor(tmpcursor);
	while(this->receiveSearchingText(ui->lineEditSearch->text())) {
		replaceFoundWord();
	}
	// 元の位置へ戻す
	plainTextEdit_->setTextCursor(currentCursor);
}



