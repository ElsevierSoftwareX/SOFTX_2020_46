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
#ifndef CHARTCONFIGDIALOG_H
#define CHARTCONFIGDIALOG_H

#include <QDialog>
#include "chartconfig.hpp"

namespace Ui {
class ChartConfigDialog;
}

class ChartConfigDialog : public QDialog
{
    Q_OBJECT

public:
	// 第三引数は前回編集中のラインスタイル番号
	ChartConfigDialog(QWidget *parent = 0, const ChartConfig &conf = ChartConfig(), int defaultLineNumber = 0);
    ~ChartConfigDialog();

    ChartConfig getChartConfig() const {return config_;}
	// 最後に編集していたラインスタイルのindex番号
	int getEditingLineNumber() const;
private:
    Ui::ChartConfigDialog *ui;
    ChartConfig config_;

    LEGEND_POS prev_xpos_;
    LEGEND_POS prev_ypos_;
	// widgetsをconfig_の状態にあわせて更新する。
	void updateWidgets(int defaultLineNumber);
    void chooseAndSetFont(QFont *font, QPushButton* button);

private slots:
	void setXtitle();
	void setYtitle();
    void chooseAxisTitleFont();
    void chooseAxisLabelFont();
    void chooseLegendFont();
    void chooseFontColor();
    void chooseAxisColor();
    void chooseBGColor();
	void choosePlotColor();
	void chooseXgridColor();
	void chooseYgridColor();
	// ラインスタイルの番号(型はQString)を受け取って、関連widgetsの現状反映を行う
	void changePlotLineWidgets(QString lineNumberString);
	void changePlotLineType(QString lineTypeString);
	void changeXgridLineType(QString lineTypeString);
	void changeYgridLineType(QString lineTypeString);
	void changePlotLineWidth(QString width);
	void changeXgridWidth(QString width);
	void changeYgridWidth(QString width);
	void setDefault();
    void handleDraggableChecked(bool b);
    void changeLegendXpos(QString str);
    void changeLegendYpos(QString str);


};

#endif // CHARTCONFIGDIALOG_H
