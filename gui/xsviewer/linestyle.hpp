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
#ifndef LINESTYLE_H
#define LINESTYLE_H

#include <map>
#include <vector>

#include <QColor>
#include <QPen>
#include <vtkPen.h>
#include <vtkPlot.h>


// Qt::Penstyle ←→String, Penstyle←→intの換算がしたい。
// Penstyle←→intはstatic_castで
// int→penstyleは変換テーブルで
// penstyle→stringは変換テーブルで
const std::map<QString, Qt::PenStyle> &getStringPenstyleMap();
QString toPenstyleString(int num);
Qt::PenStyle fromStringToPenstyle(const QString &styleString);

class LineStyle
{
	friend class ChartConfigDialog;
public:
    LineStyle();
    LineStyle(const QColor &col, Qt::PenStyle pstyle, int wid)
        :color_(col), qstyle_(pstyle), width_(wid)
    {;}
    // 普通のゲッター
    const QColor &qColor() const {return color_;}
    Qt::PenStyle qPenStyle() const {return qstyle_;}
    int width() const {return width_;}
    // セッター
    void setWidth(int wid) {width_ = wid;}


    // vtkに変換して返す
    int vPenStyle() const;
    // VTKにあわせてred gree blueはmax255のdoubleを返す。
    // ※Qtは0-255の整数か0-1のdouble
    double redD() const {return color_.redF()*255;}
    double greenD() const {return color_.greenF()*255;}
    double blueD() const {return color_.blueF()*255;}
    double alphaD() const {return color_.alphaF()*255;}
    bool isDotted() const;

    // vtkPlot(lineなど)への適用
    void applyToVtkPlot(vtkPlot *line);
    void applyToVtkPen(vtkPen *pen);


    static std::map<size_t, LineStyle> getDefaultMap(size_t num);

private:
    QColor color_;
    Qt::PenStyle qstyle_;
    int width_;
};


class LineStyleCollection
{
public:
    explicit LineStyleCollection(std::size_t num){setDefaultSize(num);}
    LineStyleCollection():LineStyleCollection(16){;}
    // num番目のラインスタイルを返す。定義済みのサイズより大きい場合は使い回しになる。
    LineStyle getLineStyle(size_t num) const {return lineStyles_.at(num%lineStyles_.size());}
	LineStyle *getLineStyleP(size_t num)  {return &lineStyles_.at(num%lineStyles_.size());}
    // デフォルトで定義済みとするラインスタイルの数を変更（デフォルトでは８）
    void setDefaultSize(size_t num);
	size_t size() const {return lineStyles_.size();}
	bool empty() const {return lineStyles_.empty();}

private:
    std::vector<LineStyle> lineStyles_;

};


#endif // LINESTYLE_H
