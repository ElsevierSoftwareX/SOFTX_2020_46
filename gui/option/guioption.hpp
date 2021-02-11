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
#ifndef GUIOPTION_HPP
#define GUIOPTION_HPP

#include <functional>
#include <vector>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

class MainWindow;

class GuiOption
{
public:
	using ProcFuncType = std::function<void(const QCommandLineParser&,QStringList*)>;

	GuiOption(const QCommandLineOption &qop,  const ProcFuncType &func);

	const QCommandLineOption &qOption() const {return qOption_;}
	void proc(const QCommandLineParser &parser, QStringList* strList){procFunc_(parser, strList);}

	static const std::vector<GuiOption> &createOptionList(MainWindow *mainWindow);

private:
	QCommandLineOption qOption_;
	ProcFuncType procFunc_;

};

#endif // GUIOPTION_HPP
