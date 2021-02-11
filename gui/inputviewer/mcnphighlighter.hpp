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
#ifndef MCNPHIGHLIGHTER_HPP
#define MCNPHIGHLIGHTER_HPP

#include <QString>
#include <QTextDocument>

#include "basehighlighter.hpp"

class McnpHighlighter : public BaseHighlighter
{
public:
	McnpHighlighter(QTextDocument *parent = 0, bool isDark = false);
	virtual void highlightBlock(const QString &text) override final;

private:
	BaseHighlighter::FormatSetType createFormatMap(bool dark);
};

#endif // MCNPHIGHLIGHTER_HPP
