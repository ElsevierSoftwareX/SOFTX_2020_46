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
#ifndef LOGHIGHLIGHTER_HPP
#define LOGHIGHLIGHTER_HPP


#include <QString>
#include <QSyntaxHighlighter>
#include <QTextDocument>

class LogHighlighter : public QSyntaxHighlighter
{
public:
	LogHighlighter(QTextDocument *parent = 0, bool dark = false);
	virtual void highlightBlock(const QString &text) override final;
private:
	bool isDark_;
};








#endif // LOGHIGHLIGHTER_HPP
