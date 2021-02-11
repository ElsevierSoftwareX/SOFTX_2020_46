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
#include "progress_utils.hpp"

#include <memory>
#ifdef ENABLE_GUI
#include <QApplication>
#include <QString>
#endif
namespace{

const int WIDTH = 40;  // CUI時に表示する文字列の最大値
}

ProgressReceiver::ProgressReceiver(
    const std::string &title,
    const std::string &cancelLabel,
    int numTargets,
    const std::string &progressFile)
    : progressFileName_(progressFile)
{
    if(!progressFileName_.empty()) ofs_.open(progressFileName_.c_str());
#ifdef ENABLE_GUI
	dialog_ = std::make_shared<QProgressDialog>(QString::fromStdString(title),
												QString::fromStdString(cancelLabel),
												0, numTargets, nullptr);
	if(cancelLabel.empty()) dialog_->setCancelButton(0);
	dialog_->show();
#else
//    std::cerr << title << std::endl;
    (void) title;
	(void) cancelLabel;
	(void) numTargets;
#endif
}

// 進捗に対応したvalueとそのプログレスバー表示を更新する。
void ProgressReceiver::setValue(int count)
{
#ifdef ENABLE_GUI
	dialog_->setValue(count);
#else
	(void) count;
#endif
}

// ダイアログ/コマンドラインへテキストを表示する。CUIの場合、CRで行頭に戻ってから表示する。
void ProgressReceiver::setLabelText(const std::string &str)
{
#ifdef ENABLE_GUI
	dialog_->setLabelText(QString::fromStdString(str));
#else
    // CUIの場合CRで行頭に戻してから表示する。
	std::cerr <<"\r" << str;
	std::cerr.flush();
#endif
}

bool ProgressReceiver::wasCanceled() const
{
// CUIではプログレスのキャンセルはしなくてもよい。Ctrl-Cでキャンセルにするという方法もあるが
// 一般にsignalをマスクするのはバグを生みガチで良い方針ではないのでやめておく。
// そもそもWindowsでは Ctrl-CでSIGINT発生しないので面倒というのもある。
#ifdef ENABLE_GUI
	return dialog_->wasCanceled();
#else
	return false;
#endif
}

// dialogを閉じる。cuiでは何もしない。
void ProgressReceiver::close()
{
#ifdef ENABLE_GUI
	dialog_->close();
#endif
}

// GUIのフリーズを防止するためにQApp::processEventsを呼ぶ。CUIならフリーズしてOKなので何もしない。
void ProgressReceiver::processEvents()
{
#ifdef ENABLE_GUI
	QApplication::processEvents();
#endif
}

// ダイアログのアップデート。当然cuiでは何もしない。
void ProgressReceiver::update()
{
#ifdef ENABLE_GUI
	dialog_->update();
#endif
}

// 進捗状況の数値を書き込む。
void ProgressReceiver::writeProgressFile(double d)
{
    if(ofs_.is_open()) {
        ofs_.close();
        ofs_.open(progressFileName_.c_str());
        if(!ofs_) {
            throw std::runtime_error(std::string("File cannot be opened. file ===") + progressFileName_);
        }
        ofs_ << d;
        ofs_.flush();
    }
}

std::string what(const std::exception_ptr &eptr)
{
	if (!eptr) { throw std::bad_exception(); }
	try {std::rethrow_exception(eptr);}
	catch (const std::exception &e) {return e.what();}
	catch (...) {return "";}
}
