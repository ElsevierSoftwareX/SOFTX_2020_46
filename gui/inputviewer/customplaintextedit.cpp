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
#include "customplaintextedit.hpp"

#include <regex>
#include <string>

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QMenu>
#include <QPainter>
#include <QSignalBlocker>
#include <QTextBlock>
#include "../globals.hpp"
#include "basehighlighter.hpp"
#include "phitshighlighter.hpp"
#include "mcnphighlighter.hpp"
#include "../../core/utils/message.hpp"
#include "../../core/io/input/common/commoncards.hpp"
#include "../../core/io/input/mcnp/mcnp_metacards.hpp"
#include "../../core/io/input/phits/phits_metacards.hpp"

LineNumberArea::LineNumberArea(CustomPlainTextEdit *editor)
	: QWidget(editor) {editor_ = editor;}

QSize LineNumberArea::sizeHint() const {return QSize(editor_->lineNumberAreaWidth(), 0);}

void LineNumberArea::paintEvent(QPaintEvent *event) {editor_->lineNumberAreaPaintEvent(event);}


CustomPlainTextEdit::CustomPlainTextEdit(QWidget *parent, McMode mode)
    : QPlainTextEdit(parent), fatalPix_(QApplication::style()->standardPixmap(QStyle::SP_MessageBoxCritical))
{
    rescalePixmaps();
	lineNumberArea = new LineNumberArea(this);

	connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
	connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));

	updateLineNumberAreaWidth(0);
    //highlightCurrentLine();
    isDark_ = global::isDarkTheme;
	mode_ = mode;
	// ここでPHITSかMCNPかハイライターを選択する。
	if(mode_ == McMode::MCNP) {
		highlighter_ = new McnpHighlighter(this->document(), isDark_);
	} else {
		highlighter_ = new PhitsHighlighter(this->document(), isDark_);
	}
    // コメント行判定はモード別に切り替えられたほうが便利だが、現時点ではその必要はなさそう。"c "だから。
    isPrefixComment_ = [](const QString &text) {
        return std::regex_search(text.toStdString(), inp::comm::getPreCommentPattern());
    };

}

void CustomPlainTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
	QPainter painter(lineNumberArea);
	// 色変更はここ
	QColor brushColor = isDark_ ? QColor(Qt::lightGray).lighter(60) : QColor(Qt::lightGray).lighter(125);
	painter.fillRect(event->rect(), QBrush(brushColor));

	QTextBlock block = firstVisibleBlock();
	int blockNumber = block.blockNumber();
	int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
	int bottom = top + (int) blockBoundingRect(block).height();


	while (block.isValid() && top <= event->rect().bottom()) {
		if (block.isVisible() && bottom >= event->rect().top()) {
			QString number = QString::number(blockNumber + 1);
			painter.setPen(Qt::black);
//			painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
//                             Qt::AlignRight, number);
            painter.drawText(0, top, lineNumberArea->width()-fatalPix_.width(), fontMetrics().height(),
                             Qt::AlignRight, number);

        }

        // fatalLines_に含まれている行へ到達したらアイコンを出す
        if(fatalLines_.find(blockNumber+1) != fatalLines_.end()) {
			// Criticalマークの描画
			painter.drawPixmap(lineNumberArea->width()-0.9*fatalPix_.width(), top+0.1*fatalPix_.height(),
							   fatalPix_.width(), fatalPix_.height(), fatalPix_);
        }

		block = block.next();
		top = bottom;
		bottom = top + (int) blockBoundingRect(block).height();
		++blockNumber;
	}
}


int CustomPlainTextEdit::lineNumberAreaWidth()
{
	int digits = 1;
	int max = qMax(1, blockCount());
	while (max >= 10) {
		max /= 10;
		++digits;
	}
    // 行表示スペースの幅はここ。将来的にはエラー箇所表示なんかにも使いたい
//    int space = 5 + QWidget::fontMetrics().width("9") * (digits) + fatalPix_.width();
	int space = 5 + QWidget::fontMetrics().horizontalAdvance("9") * (digits) + fatalPix_.width();

    return space;
}

void CustomPlainTextEdit::updateHighlighter(const QString &word, bool isCaseSensitive, bool isWordUnit, bool isRegularExpression)
{
	highlighter_->setEmphasisWordInfo(word, isCaseSensitive, isWordUnit, isRegularExpression);
    highlighter_->rehighlight();
}

void CustomPlainTextEdit::setFatalLine(int line)
{
	// Fatal行へのマークの追加と移動。
	fatalLines_.emplace(line);
    // 行の強調。ファイル保存したら表示は消す
	QTextEdit::ExtraSelection selection;
	QColor lineColor = isDark_ ? QColor(Qt::yellow).lighter(40) : QColor(Qt::yellow).lighter(160);
	selection.format.setBackground(lineColor);
	selection.format.setProperty(QTextFormat::FullWidthSelection, true);
	selection.cursor = textCursor();
	selection.cursor.clearSelection();
	selection.cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line-1);
	setExtraSelections(QList<QTextEdit::ExtraSelection> {selection});
	// 移動
	QTextCursor cursor(this->document()->findBlockByLineNumber(line-1)); // 行番号は0スタートなので-1
	this->setTextCursor(cursor);
}

void CustomPlainTextEdit::clearFatalLine()
{
	fatalLines_.clear();
	// ハイライトを消す。要するになにもしないselectionで上書きしているだけ。
	QTextEdit::ExtraSelection selection;
	selection.format.setProperty(QTextFormat::FullWidthSelection, true);
	selection.cursor = textCursor();
	selection.cursor.clearSelection();
	setExtraSelections(QList<QTextEdit::ExtraSelection> {selection});

	QTextCursor tc = textCursor();

    this->repaint();
}




void CustomPlainTextEdit::commentInOutLines(const std::vector<int> &lines, QTextCursor &cursor)
{
    /*
     * 選択範囲の行全てがコメント → コメントイン
     * それ以外の場合→ コメントアウト
     */
    QTextDocument* documentPtr = this->document();
    bool isCommentOnly = true;
    for(const auto& lineNumber: lines) {
        if(!isPrefixComment_(documentPtr->findBlockByLineNumber(lineNumber).text())) {
            isCommentOnly = false;
            break;
        }
    }

    //auto cursor = this->textCursor();
    int anchorPos = documentPtr->findBlockByLineNumber(lines.front()).position();
//    auto backupPos = cursor.position();
//    mDebug() << "backupPos===" << backupPos;
    //cursor.setKeepPositionOnInsert(true); // いまのルーチンではinsert後何もせず先頭に戻るので意味なし。
    for(const auto& lineNumber: lines) {
        QTextBlock textBlock = documentPtr->findBlockByLineNumber(lineNumber);
        QString text = textBlock.text();
        cursor.setPosition(textBlock.position());
        cursor.select(QTextCursor::LineUnderCursor);
        //cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
        //if(!text.isEmpty() && !cursor.atStart()) cursor.insertBlock();


        if(isCommentOnly) {
            // コメント解除
            text = QString::fromStdString(inp::comm::preUncommentedString(text.toStdString()));
            cursor.insertText(text);
        } else {
            // コメントアウト
            cursor.insertText(QString("c " + text));
        }

    }
    cursor.movePosition(QTextCursor::Right);

    // anchorの復元
    auto currentPos = cursor.position();
    cursor.setPosition(anchorPos);
    cursor.setPosition(currentPos, QTextCursor::KeepAnchor);
}


// 選択範囲、あるいはカーソル行をコメント/解除する。
void CustomPlainTextEdit::commentRegion()
{
    auto cursor = this->textCursor();
    auto anchorPos = cursor.anchor();
    auto cursorPos = cursor.position();
    std::vector<int> lines;
    if(anchorPos == cursorPos) {
        // テキスト選択なしの場合現在行をコメントをリストに登録。
        lines.emplace_back(cursor.blockNumber());
    } else {
        auto cursorBlockNumber = cursor.blockNumber();
        bool isCursorAtBlockStart = cursor.atBlockStart();
        cursor.setPosition(anchorPos);
        auto anchorBlockNumber = cursor.blockNumber();
        bool isAnchorAtBlockStart = cursor.atBlockStart();
        if(anchorBlockNumber == cursorBlockNumber) {
            // テキストが選択されているが行内なので現在行をリストに追加。
            lines.emplace_back(cursorBlockNumber);
        } else {
            // テキスト選択が行をまたぐ場合。
            // cursorかanchorが行頭にある場合、後ろにある方のその行は含めない。
            if(isCursorAtBlockStart && cursorBlockNumber > anchorBlockNumber) --cursorBlockNumber;
            if(isAnchorAtBlockStart && cursorBlockNumber < anchorBlockNumber) --anchorBlockNumber;

            std::pair<int, int> startAndEnd = std::make_pair(anchorBlockNumber, cursorBlockNumber);
            if(startAndEnd.first > startAndEnd.second) std::swap(startAndEnd.first, startAndEnd.second);
            for(int i = startAndEnd.first; i <= startAndEnd.second; ++i) lines.emplace_back(i);
        }
    }

    //mDebug() << "comment/uncomment lineindex ===" << lines;
    //mDebug() << "before anchor pos ===" << cursor.anchor();
    cursor.beginEditBlock();
    commentInOutLines(lines, cursor);
    cursor.endEditBlock();
//    mDebug() << "after anchor pos ===" << cursor.anchor();
//    mDebug() << "selected text===" << cursor.selectedText();

    // FIXME cursor.selectedTextとguiで選択されているtextが異なる。

    return;
}



// NOTE 一旦Ctrl-wheelで拡大縮小させるとCtrl-+でフォントサイズが変わらなくなる。但しこれは素のplaintexteditでも同じなので仕様かも
void CustomPlainTextEdit::wheelEvent(QWheelEvent *event)
{
    // Ctrl押しながらの時はzoomをリクエスト。それ以外の場合は通常処理
    if(event->modifiers() == Qt::ControlModifier ) {
        if(event->delta() > 0) {
            this->zoomIn();
        } else {
            this->zoomOut();
        }
        event->accept();
        // ここでQPlainTextEdit::wheelEvent(event);するとスクロールしてしまうから実行しない
    } else{
        QPlainTextEdit::wheelEvent(event);
    }
    //QPlainTextEdit::wheelEvent(event);

}

void CustomPlainTextEdit::resizeEvent(QResizeEvent *event)
{
	QPlainTextEdit::resizeEvent(event);

	QRect cr = contentsRect();
	lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CustomPlainTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    // いまカーソルの有る位置での行テキスト(ブロック)を取得し、includeカードなら
    // ファイルを開くを追加する。TODO 本当はクリックした位置で取得したい
    std::string text = this->textCursor().block().text().trimmed().toStdString();
    std::smatch sm;

    QString fileName;
    if(std::regex_search(text, sm, inp::mcnp::getReadCardPattern())) {
        fileName = QString::fromStdString(inp::mcnp::procReadCard(sm.suffix()).first);
    } else if (std::regex_search(text, sm, inp::phits::getIncludeCardPattern())) {
        fileName = QString::fromStdString(inp::phits::procInflCard(text).first);
    }


    QMenu *menu = createStandardContextMenu();
    if(!fileName.isEmpty()) {
        QString menuStr = tr("Open \"") + fileName + tr("\"");
        QAction *openAct = menu->addAction(menuStr);
        menu->insertAction(menu->actions().at(0), openAct);
        connect(openAct, &QAction::triggered, this, [&](){emit requestOpenFile(fileName);});
    }
    menu->exec(event->globalPos());
    delete menu;
    //QPlainTextEdit::contextMenuEvent(event);
}

void CustomPlainTextEdit::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::StyleChange) {
        isDark_ = global::isDarkTheme;
        delete highlighter_;
		if(mode_ == McMode::MCNP) {
			highlighter_ = new McnpHighlighter(this->document(), isDark_);
		} else {
			highlighter_ = new PhitsHighlighter(this->document(), isDark_);
		}
    } else if(event->type() == QEvent::FontChange) {
        // NOTE newなどで動的に作成されたWidgetはQApplication::setFont()が効かないので手動で調整する。
    }
    rescalePixmaps();
	QPlainTextEdit::changeEvent(event);
}

void CustomPlainTextEdit::keyPressEvent(QKeyEvent *event)
{
	if(event->key() == Qt::Key_Insert) {
		this->setOverwriteMode(!this->overwriteMode());
	}
	QPlainTextEdit::keyPressEvent(event);
}

void CustomPlainTextEdit::updateLineNumberAreaWidth(int newBlockCount)
{
    Q_UNUSED(newBlockCount)
    // texteditのマージンはここ
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

//void CustomPlainTextEdit::highlightCurrentLine()
//{
//	QList<QTextEdit::ExtraSelection> extraSelections;
//	if (!isReadOnly()) {
//		QTextEdit::ExtraSelection selection;
//		QColor lineColor = isDark_ ? QColor(Qt::yellow).lighter(20) : QColor(Qt::yellow).lighter(190);
//		selection.format.setBackground(lineColor);
//		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
//		selection.cursor = textCursor();
//		selection.cursor.clearSelection();
//		extraSelections.append(selection);
//	}
//	setExtraSelections(extraSelections);
//}

void CustomPlainTextEdit::updateLineNumberArea(const QRect &rect, int dy)
{
	if (dy){
		lineNumberArea->scroll(0, dy);
	} else {
		lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
	}
    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth(0);
}


