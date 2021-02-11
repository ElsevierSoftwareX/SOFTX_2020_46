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
#ifndef GUIUTILS_HPP
#define GUIUTILS_HPP

#include <string>

#include <QColor>
#include <QFont>
#include <QPushButton>
#include <QString>
#include <QWidget>

namespace gutils {


struct OpenGLInfo{
	OpenGLInfo():version(-1), isMesa(false){;}
	std::string vendor;
	std::string renderer;
	std::string versionStr;
	double version;
	bool isMesa;

	std::string toString() const;
};

OpenGLInfo getOpenGLInfo();

// CSSによるWidget背景色へのにcolorを適用/取得
void setWidgetColor(const QColor& color, QWidget *widget);
QColor getWidgetColor(const QWidget *widget);

// 色をダイアログで取得し、col, widgetにセットする。
void chooseAndSetColor(QWidget* parent, QColor *col, QWidget *widget);

// QFont::toString()の大半は不要な情報なので削除
QString simpleFontString(const QString &fontString);

// fontDialogからフォントを取得してfontへ代入し、labelへ反映させる
void chooseAndSetFont(QWidget *parent, QFont *font, QPushButton *button);

}

#endif // GUIUTILS_HPP
