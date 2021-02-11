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
#include "loghighlighter.hpp"

#include <regex>
#include <QColor>
#include <QRegExp>
#include <QTextCharFormat>

LogHighlighter::LogHighlighter(QTextDocument *parent, bool dark)
	:QSyntaxHighlighter(parent), isDark_(dark){;}


void LogHighlighter::highlightBlock(const QString &text)
{
	const QColor BLUE = (isDark_) ? QColor(150, 150, 255) : QColor(0, 0, 255);

	QTextCharFormat fatalFormat;
	fatalFormat.setFontWeight(QFont::Bold);
	fatalFormat.setForeground(Qt::red);

	QTextCharFormat warningFormat;
	warningFormat.setFontWeight(QFont::Bold);
	warningFormat.setForeground(BLUE);

	if(text.startsWith("fatal", Qt::CaseInsensitive)) {
		setFormat(0, text.size(), fatalFormat);
	} else if(text.startsWith("warning", Qt::CaseInsensitive)) {
		setFormat(0, text.size(), warningFormat);
	}
}






