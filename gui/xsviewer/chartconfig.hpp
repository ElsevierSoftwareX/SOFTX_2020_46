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
#ifndef CHARTCONFIG_HPP
#define CHARTCONFIG_HPP

#include <memory>
#include <tuple>
#include <utility>

#include <QColor>
#include <QFont>
#include <QString>
#include <QStringList>

#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkSmartPointer.h>


#include "linestyle.hpp"

// vtkAxis::LEFTなどとstatic_cast<int>できるような順番になっている。
enum class LEGEND_POS : int{LEFT = 0, CENTER, RIGHT, TOP, BOTTOM, CUSTOM};

// グラフ領域及びプロット線の設定を保持するクラス
class ChartConfig {
    friend class ChartConfigDialog;
public:
    ChartConfig();
	const QString &xtitle() const {return xtitle_;}
	const QString &ytitle() const {return ytitle_;}
    const QFont &axisTitleFont() const {return axisTitleFont_;}
    const QFont &axisLabeFont() const {return axisLabelFont_;}
    const QFont &legendFont() const {return legendFont_;}
    const QColor &fontColor() const {return fontColor_;}
    const QColor &bgColor() const {return bgColor_;}
    LEGEND_POS xpos() const {return xpos_;}
    LEGEND_POS ypos() const {return ypos_;}
    bool haveApplied() const {return haveApplied_;}
    void setHaveApplied(bool b){haveApplied_ = b;}

    //vtk用関数
    void applyLineStyle(int num, vtkPlot* line){lineStyleCollection_.getLineStyle(num).applyToVtkPlot(line);}
    // chartに設定を適用する。
    void applyToChart(vtkSmartPointer<vtkChartXY> &chart);

    //static const QStringList &legendPosList();
    static const QStringList &legendHPosList();
    static const QStringList &legendVPosList();
    static LEGEND_POS fromStringToPos(const QString& str);
    static QString fromPosToString(LEGEND_POS pos);

private:
	QString xtitle_;
	QString ytitle_;
    QFont axisTitleFont_;
    QFont axisLabelFont_;
    QFont legendFont_;

    QColor fontColor_;  // fontの色
    QColor bgColor_;    // plot領域の背景色
    QColor axisColor_;  // axisの色
    LineStyle xgridLineStyle_;  // グリッド線のスタイル
    LineStyle ygridLineStyle_;
    LineStyleCollection lineStyleCollection_;
    LEGEND_POS xpos_;
    LEGEND_POS ypos_;
    bool haveApplied_;  // すでに一度でも適用済みかを管理するフラグ

};


#endif // CHARTCONFIG_HPP
