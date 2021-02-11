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
#ifndef SECTIONALVIEWER_HPP
#define SECTIONALVIEWER_HPP

#include <deque>
#include <memory>

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPixmap>
#include <QTimer>

#include "../tabitem.hpp"
#include "../fileconverter.hpp"
#include "../../core/simulation.hpp"
#include "../../core/terminal/interactiveplotter.hpp"

struct GuiConfig;


namespace Ui {
class SectionalViewer;
}

class SectionalViewer : public TabItem
{
	Q_OBJECT

public:
    SectionalViewer(QWidget *parent = nullptr, const QString &tabText = "", const GuiConfig *gconf = nullptr);
    virtual ~SectionalViewer();
	void init() override final;
	void retranslate() override final;
	void exportToRasterGraphics() override final;

public slots:
	void updateSimulationData(std::shared_ptr<const Simulation> sim);
	void updateNumThreads2D(int numThreads);

signals:
	void localNumThreadsChanged(int num);
    void plotterConfigChanged();
	void updateSectViewFinished();

protected:
	void resizeEvent(QResizeEvent *ev) override;

private:
	Ui::SectionalViewer *ui;
	std::shared_ptr<const Simulation> simulation_;
	std::shared_ptr<term::InteractivePlotter> plotter_;
	// コマンドヒストリ保存スタック。今の実装ではterminalがヒストリを保持しているので
	// unixターミナルのヒストリはguiでは使えない。このためgui側で保持している。
	// ヒストリ機能はcui/guiで分けずにplotterに一本化したいところではある。
	std::deque<QString> historyStack_;
	std::deque<QString>::iterator historyItr_;

	QGraphicsScene scene_;
	std::unique_ptr<QPixmap> pixmap_;  // pixmap:画像本体
	QGraphicsPixmapItem *pixmapItem_;
	QGraphicsRectItem *frameItem_;     // 画像表示領域

	QTimer resizeTimer_;
	// method
	void clear();
	QSize drawImage();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
	// UIからコマンドを受け取って実行
	void execCommand();
	// プロット設定を変えるとplotterからsignalが来るのでそれを処理する。
    void handlePlotterConfigChanged();
	// xpm文字列を受け取って画像を表示する。
    void handlePlottedImage(std::string xpmStr);
	// resizeEventの実体。2D画像のスケーリングはそれなりの負荷なので一定時間以内のresizeは無視する。
	void handleResize();
	// プロット時のメッセージを受け取って表示する。
	void handleCommandResultMessage(QString message);
	// tracing解像度を描画領域の解像度に合わせる。
	void handleFittingResolutionToScreen();
	// historyコマンドの実行
	void handlePrintGuiHistory();
};

#endif // SECTIONALVIEWER_HPP
