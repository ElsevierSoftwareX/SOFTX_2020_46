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
#include "sectionalviewer.hpp"
#include "ui_sectionalviewer.h"

#include <algorithm>
#include <cstring>

#include <QFont>
#include <QFontMetrics>
#include <QFileDialog>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QPixmap>
#include <QScrollBar>
#include <QTimer>

#include "xpmarray.hpp"
#include "../globals.hpp"
#include "../core/utils/message.hpp"
#include "../core/image/bitmapimage.hpp"
#include "../core/utils/string_utils.hpp"
#include "../subdialog/messagebox.hpp"
#include "../option/guiconfig.hpp"

namespace {
const int OFFSET_PIXEL = 4;
const size_t MAX_HISTORY = 50;
}


SectionalViewer::SectionalViewer(QWidget *parent, const QString &tabText, const GuiConfig *gconf) :
	TabItem(parent, tabText, gconf),
	ui(new Ui::SectionalViewer), historyItr_(historyStack_.end()), pixmap_(nullptr), pixmapItem_(nullptr), frameItem_(nullptr)
{
	ui->setupUi(this);
	ui->graphicsView->setScene(&scene_);
	scene_.clear();
	ui->lineEditCommand->installEventFilter(this);

	resizeTimer_.setSingleShot(true);
	connect(&resizeTimer_, &QTimer::timeout, this, &SectionalViewer::handleResize);

	// pixmap_に適当に初期ダミーデータ(真っ白)を与えておけばresizeEventで適当に大きさを設定してくれるはず。
	const char * const dummyxpm[] = {
	"2 2 1 1",
	". s  0  c #ffffff",
	"..",
	".."
	};
	pixmap_.reset(new QPixmap(dummyxpm));
}


SectionalViewer::~SectionalViewer() {delete ui;}


void SectionalViewer::init()
{
	// 2DViewerタブを開いた時は必ずコマンド欄にフォーカスを移す
	ui->lineEditCommand->setFocus();
	if(hasInitialized_) return;
	int w = ui->splitter->width();
//	int infoWidth = std::min(static_cast<int>(std::round(0.4*w)), QFontMetrics(QFont()).width('0')*50);
	int infoWidth = std::min(static_cast<int>(std::round(0.4*w)), QFontMetrics(QFont()).horizontalAdvance('0')*50);
	ui->splitter->setSizes(QList<int>{infoWidth, w - infoWidth});
	TabItem::init();
	//plotter_->execCommandLineString("pz 0");
	//handleFittingResolutionToScreen();
	// とりあえず最初にダミー画像でも与えておけば適当にリサイズできるはず。
}

void SectionalViewer::exportToRasterGraphics()
{
	if(!pixmap_) {
        GMessageBox::warning(this, tr("Warning"), tr("2D view is empty (not plotted yet?)."), true);
		return;
	}

	auto infoPair = ff::getFileAndFormat2D();
	auto format = infoPair.second;
	auto finfo = infoPair.first;
	if(infoPair.second == ff::FORMAT2D::NOT_DEFINED) return;

	QImage img = pixmap_->toImage();
	img.save(finfo.absoluteFilePath(), QString::fromStdString(ff::format2DToStr(format)).toLatin1());
	mDebug() << "File" << finfo.absoluteFilePath() << " saved. size=" << img.size() << "foramt=" << ff::format2DToStr(format);
}

void SectionalViewer::retranslate() {ui->retranslateUi(this);}


void SectionalViewer::updateSimulationData(std::shared_ptr<const Simulation> sim)
{
	this->clear();
	if(!sim) return;
	simulation_ = sim;
	// ここでの第二引数はplotterがローカルに持つスレッド数なのでポインタなどではなく値を入れる
	plotter_ = std::make_shared<term::InteractivePlotter>(simulation_, guiConfig_->cuiConfig.numThread, this->guiConfig_->cuiConfig.verbose);
	connect(plotter_.get(), &term::InteractivePlotter::sectionPlotted,
			this, &SectionalViewer::handlePlottedImage);
	connect(plotter_.get(), &term::InteractivePlotter::sectionPlotted,
			this, &SectionalViewer::updateSectViewFinished);
	connect(plotter_.get(), &term::InteractivePlotter::plotterConfigChanged,
			this, &SectionalViewer::handlePlotterConfigChanged);
	connect(plotter_.get(), &term::InteractivePlotter::requestResize,
			this, &SectionalViewer::handleResize);
	connect(plotter_.get(), &term::InteractivePlotter::sendMessage,
			this, &SectionalViewer::handleCommandResultMessage);
	connect(plotter_.get(), &term::InteractivePlotter::requestFittingResolutionToScreen,
			this, &SectionalViewer::handleFittingResolutionToScreen);
	connect(plotter_.get(), &term::InteractivePlotter::requestPrintGuiHistory,
			this, &SectionalViewer::handlePrintGuiHistory);
	connect(plotter_.get(), &term::InteractivePlotter::localNumThreadsChanged,
			this, [=](int num) {emit localNumThreadsChanged(num);});
	handlePlotterConfigChanged();  // 初期設定を反映させる。
}

void SectionalViewer::updateNumThreads2D(int numThreads)
{
	// TODO 設定が変わった時の情報更新をする必要がある。
	if(plotter_->numThreads() != numThreads) {
		plotter_->setNumThreads(numThreads);
		// localNumThreadsChanged()はplotter_からemitされるので、ここではemitしない
		handlePlotterConfigChanged();  // 初期設定を反映させる。
	}
}

void SectionalViewer::resizeEvent(QResizeEvent *ev)
{
	// 頻繁にresize処理をしたくないので、
	// このクラス特有のリサイズ処理は100ms間resizeがなければ実行
	// connect(&resizeTimer_, &QTimer::timeout, this, &SectionalViewer::handleResize);
	// というようにhandleResizeに繋がれている。
	if(resizeTimer_.isActive()) resizeTimer_.stop();
	resizeTimer_.start(100);
	QWidget::resizeEvent(ev);
}

void SectionalViewer::clear()
{
	simulation_.reset();
//	// 古いアイテムはscene_.clear()で除去されるから、手動除去は不要
//	if(pixmapItem_ != nullptr) scene_.removeItem(pixmapItem_);
//	if(frameItem_ != nullptr) scene_.removeItem(frameItem_);
	scene_.clear();
}



void SectionalViewer::execCommand()
{
	// コマンド実行で大量のメッセージが発生すると受取に長時間掛かるので注意。
	QString command = (ui->lineEditCommand->text());
	ui->lineEditCommand->clear();
	QApplication::processEvents();

	if(plotter_) {
		plotter_->execCommandLineString(command.toStdString());
		if(historyStack_.empty() && !command.isEmpty()) {
			historyStack_.push_back(command);
		} else if(!command.isEmpty() && command != historyStack_.back()) {
			historyStack_.push_back(command);
		}

		if(historyStack_.size() >= MAX_HISTORY) historyStack_.pop_front();
		historyItr_ = historyStack_.end();
	}

    // コマンド実行後に画像を取得する方法は？
	// ENABLE_GUI時はplotter_ にQObjectになってもらってemit sectionPlottedで送ってもらう。
}

void SectionalViewer::handlePlotterConfigChanged()
{
	// ここでplotter_から情報を取得してlabelOriginなどに反映させる。
    ui->labelOrigin->setText(QString::fromStdString(utils::toString(plotter_->origin())));
    ui->labelLineWidth->setText(QString::fromStdString(utils::toString(plotter_->lineWidth())));
	ui->labelNumThread->setText(QString::fromStdString(utils::toString(plotter_->numThreads())));
    auto resopair = plotter_->resolution();
    ui->labelResolution->setText(QString::number(resopair.first) + ", "
                                 +  QString::number(resopair.second));
	auto widthpair = plotter_->width();
	ui->labelImageWidth->setText(QString::number(widthpair.first) + ", "
								 + QString::number(widthpair.second));
	ui->labelNormal->setText(QString::fromStdString(utils::toString(plotter_->normal())));
	ui->labelHdir->setText(QString::fromStdString(utils::toString(plotter_->hDir())));
	ui->labelVdir->setText(QString::fromStdString(utils::toString(plotter_->vDir())));
}


void SectionalViewer::handlePlottedImage(std::string xpmStr)
{
	if(xpmStr.empty()) return;
	XpmArray xpmArray(xpmStr);
    //mDebug() << "xpmStr===" << xpmStr;
	pixmap_.reset(new QPixmap(xpmArray.getArray()));
	// QPixmap::scaled()は解像度が減少する一方なのでクラスメンバで保持するpixmapはscaledしない
	// scaledすると画素が減少して回復は不可避であることに要注意。
	// よってaddPixmapの返り値で得られるQGraphicsPixmapItemも解像度が劣化しているので使わないこと。
	drawImage();
}


void SectionalViewer::handleResize()
{
	// ここでプロッターに最適な解像度を設定する。ray-tracingの解像度と表示領域の解像度を合わせる。
	if(plotter_) {
		// 画面サイズが変わった場合とりあえず画面を更新する。
		auto sz = drawImage();
		if(sz.width() != 0 && sz.height() != 0) {
			std::string command = "r " + std::to_string(sz.width()) + " " + std::to_string(sz.height());
			plotter_->execCommandLineString(command);
		}
	}
}

void SectionalViewer::handleCommandResultMessage(QString message)
{
	ui->plainTextEditCommandResult->appendPlainText(message);
	// 追加したら末尾までスクロールする。
	ui->plainTextEditCommandResult->verticalScrollBar()
			->setValue(ui->plainTextEditCommandResult->verticalScrollBar()->maximum());
}

// ray-tracingの解像度を1ray-1pixelになるように調節する。
void SectionalViewer::handleFittingResolutionToScreen()
{
	if(pixmap_ != nullptr) handleResize();
}

void SectionalViewer::handlePrintGuiHistory()
{
	std::stringstream ss;
	ss << "HISTORY:" << std::endl;
	for(const auto &com: historyStack_) {
		ss << com.toStdString() << std::endl;
	}
	mDebug() << ss.str();
	handleCommandResultMessage(QString::fromStdString(ss.str()));
}


// GUI WidgetのresizeEvent発生時の描画。返り値は描画後の新しい画像サイズ(pixels)
QSize SectionalViewer::drawImage()
{
	// 一度も描画していなければpixmap_はnullptr
	if(pixmap_ == nullptr)  return QSize(0, 0);
	clear();

	// graphicsView.sizeは2D表示領域全体のサイズ。ウインドウの大きさ依存でだいたい横長長方形
	QSize graphicsViewSize = ui->graphicsView->size();
	// 描画サイズはその2D表示領域に若干のオフセットを付けて表示する
	QSize newSize(graphicsViewSize.width()-OFFSET_PIXEL, graphicsViewSize.height()-OFFSET_PIXEL);
	// 新しいサイズは正方形とする。
	auto length = std::min(newSize.width(), newSize.height());
	newSize = QSize(length, length);

	// 描画時には新しいpixmapを作成する。オリジナルのpixmap_をリサイズしてしまうと
	// 縮小はいいが後で拡大できなくなるから。(プロットセずに縮小→拡大したときに元に戻せるように)
	if(pixmap_->size() != newSize) {
		QPixmap newPixmap = pixmap_->scaled(newSize, Qt::KeepAspectRatio);
		pixmapItem_ = scene_.addPixmap(newPixmap);
		frameItem_ = scene_.addRect(newPixmap.rect());
		newSize = newPixmap.size();
	} else {
		pixmapItem_ = scene_.addPixmap(*pixmap_.get());
		frameItem_ = scene_.addRect(pixmap_->rect());
		newSize = pixmap_->size();
	}

	// 表示領域の大きさはframeItemと同じものに統一。
	scene_.setSceneRect(frameItem_->rect());
	ui->graphicsView->fitInView(frameItem_, Qt::KeepAspectRatio);
	return newSize;
}


bool SectionalViewer::eventFilter(QObject *obj, QEvent *event)
{
	if(obj == ui->lineEditCommand) {
		QEvent::Type type = event->type();
		if(type == QEvent::KeyPress) {
			int key = static_cast<QKeyEvent*>(event)->key();

			if(key == Qt::Key_Up) {
				if(historyItr_ != historyStack_.begin()) --historyItr_;
				if(!historyStack_.empty()) ui->lineEditCommand->setText(*(historyItr_));
				return true;
			} else if (key == Qt::Key_Down) {
				if(historyItr_ != historyStack_.end()) ++historyItr_;
				if(!historyStack_.empty()) {
					if(historyItr_ == historyStack_.end()) {
						ui->lineEditCommand->setText("");
					} else {
						ui->lineEditCommand->setText(*(historyItr_));
					}
				}
				return true;
			} else {
				return false;
			}
		}
	}
	return false;

}
