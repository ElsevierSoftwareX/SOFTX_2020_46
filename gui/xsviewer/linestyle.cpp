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
#include "linestyle.hpp"

#include <unordered_map>
#include <vector>

#include <vtkPlot.h>

#include "../../core/utils/message.hpp"

LineStyle::LineStyle() {;}

// penStyleをvtkPenStyleで返す
int LineStyle::vPenStyle() const
{
    //  vtkPenの線種は５種類SOLID_LINE, DASH_LINE, DOT_LINE, DASH_DOT_LINE, DASH_DOT_DOT_LINE
    // QPenの対応はQt::SolidLine Qt::DashLine Qt::DotLine Qt::DashDotLineQt::DashDotDotLine

    static std::map<Qt::PenStyle, int> penStyleMap {
		{Qt::NoPen, vtkPen::NO_PEN},
        {Qt::SolidLine, vtkPen::SOLID_LINE},
        {Qt::DashLine, vtkPen::DASH_LINE},
        {Qt::DotLine, vtkPen::DOT_LINE},
        {Qt::DashDotLine, vtkPen::DASH_DOT_LINE},
        {Qt::DashDotDotLine, vtkPen::DASH_DOT_DOT_LINE}
    };

    return penStyleMap.at(this->qPenStyle());
}

bool LineStyle::isDotted() const
{
    return qstyle_ == Qt::DashLine || qstyle_ == Qt::DotLine
        || qstyle_ == Qt::DashDotLine || qstyle_ == Qt::DashDotDotLine;
}

void LineStyle::applyToVtkPlot(vtkPlot *line)
{
    line->GetPen()->SetLineType(vPenStyle());
    line->SetColor(redD(), greenD(), blueD(), alphaD());
    line->SetWidth(width_);
}

void LineStyle::applyToVtkPen(vtkPen *pen)
{
    pen->SetColor(255*color_.redF(), 255*color_.greenF(), 255*color_.blueF());
    pen->SetLineType(vPenStyle());
    pen->SetWidth(width_);
}




void LineStyleCollection::setDefaultSize(size_t num)
{
    static std::vector<QColor> baseColors {
        QColor(Qt::red), QColor(Qt::green), QColor(Qt::blue),
        QColor(Qt::cyan), QColor(Qt::magenta), QColor(Qt::yellow), QColor(Qt::black)
    };
    static std::vector<Qt::PenStyle> basePenStyles
    {
        Qt::SolidLine, Qt::DashLine, Qt::DotLine, Qt::DashDotLine, Qt::DashDotDotLine
    };

    lineStyles_.clear();
    lineStyles_.reserve(num);
    for(size_t i = 0; i < num; ++i) {
        lineStyles_.emplace_back(baseColors.at(i%baseColors.size()),
                                 basePenStyles.at(i%basePenStyles.size()), 1);
    }

	// 線種5通り、線色7通りなので組み合わせは35通りになる。
	//
	// 線種毎の設定。見づらいので点線類は線幅を2倍にする
    for(size_t i = 0; i < lineStyles_.size(); ++i) {
		switch(lineStyles_.at(i).qPenStyle()) {
		case Qt::SolidLine:
			break;
		case Qt::DashLine:
			lineStyles_.at(i).setWidth(lineStyles_.at(i).width()*2);
			break;
		case Qt::DotLine:
			lineStyles_.at(i).setWidth(lineStyles_.at(i).width()*4);
			break;
		case Qt::DashDotLine:
			lineStyles_.at(i).setWidth(lineStyles_.at(i).width()*2);
			break;
		case Qt::DashDotDotLine:
			lineStyles_.at(i).setWidth(lineStyles_.at(i).width()*2);
			break;
		default:
			assert(!"this dosenot happen.");
		}

    }

    //TODO darkテーマ用の色マップも
}


const std::map<QString, Qt::PenStyle> &getStringPenstyleMap()
{
	static const std::map<QString, Qt::PenStyle> penStrMap
	{
		{"NoPen", Qt::NoPen}, {"SolidLine", Qt::SolidLine}, {"DashLine", Qt::DashLine},
		{"DotLine", Qt::DotLine}, {"DashDotLine", Qt::DashDotLine}, {"DashDotDotLine", Qt::DashDotDotLine},
		{"CustomDashLine", Qt::CustomDashLine}
	};
	return penStrMap;
}

QString toPenstyleString(int num)
{
	auto penMap = getStringPenstyleMap();
	for(auto it = penMap.begin(); it != penMap.end(); ++it) {
		if(static_cast<int>(it->second) == num) return it->first;
	}
	throw std::out_of_range(std::to_string(num) + " is not a Qt::PenStyle");
}

Qt::PenStyle fromStringToPenstyle(const QString &styleString)
{
	return getStringPenstyleMap().at(styleString);
}


