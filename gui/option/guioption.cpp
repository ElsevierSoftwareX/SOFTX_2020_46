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
#include "guioption.hpp"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTimer>

#include "../core/utils/message.hpp"
#include "../core/utils/system_utils.hpp"
#include "globals.hpp"
#include "mainwindow.hpp"

// 引数リストを格納したstrListからオプションの部分を削除する関数
void RemoveOptStrings(const QStringList &optNames, QStringList *strList) {
	int index = 0;
	for(auto &optName: optNames) {
		while(index = strList->indexOf(QRegExp("--" + optName + "=.*")), index != -1) {
			strList->removeAt(index);
		}
	}
	// --option=val型のオプション文字列を削除した後に --option型の削除をする。
	for(auto &optName: optNames) {
		while(index = strList->indexOf(QRegExp("--" + optName + "$")), index != -1) {
			strList->removeAt(index);
		}
	}
}


GuiOption::GuiOption(const QCommandLineOption &qop, const GuiOption::ProcFuncType &func)
	:qOption_(qop), procFunc_(func)
{;}


const std::vector<GuiOption> &GuiOption::createOptionList(MainWindow *mainWindow)
{
	using ComOpt = QCommandLineOption;


	// オプションの情報設定
	// --css
	QDir cssDir("://css");
	QString themeNames;
	for(auto &themeName: cssDir.entryList()) {  // themeはリソースファイルをスキャンして追加
		themeNames += "|" + themeName;
	}
	themeNames = themeNames.mid(1);
	ComOpt cssOpt(QStringList() << "css",                //オプション文字列
				  "Apply style sheet", //help表示の際のオプション説明
				  themeNames,       //オプション指定例
				  "");  // デフォルト値
	auto cssFunc = [cssOpt, mainWindow](const QCommandLineParser &parser, QStringList *argList){
		RemoveOptStrings(cssOpt.names(), argList);
		QString css = parser.value(cssOpt);
		// QTimerでeventLoop突入後にMainWindow::applyThemeを実行させないと反映されない
		QTimer::singleShot(0, [=](){mainWindow->applyTheme(css);});
	};

	// --verbose
	ComOpt verboseOpt(QStringList{"verbose"}, "Set verbose flag on", "", "not set");
	auto verboseFunc = [verboseOpt](const QCommandLineParser &parser, QStringList *argList) {
		(void)parser;
		RemoveOptStrings(verboseOpt.names(), argList);
		// ここにveboseオプション処理
		// coreの方に-vとして渡す
		argList->append("-v");
	};

	// --threads
	ComOpt threadOpt(QStringList{"threads"}, "Number of threads. positive:number of threads, negative or 0: auto", "integer", "1");
	auto threadFunc = [threadOpt](const QCommandLineParser &parser, QStringList *argList){
		RemoveOptStrings(threadOpt.names(), argList);
		bool ok = false;
		int val = parser.value(threadOpt).toInt(&ok);
		if(ok) {
			val = utils::guessNumThreads(val);
		} else {
			mWarning() << "--thread=" << val << "is not a legal option, num thread=1 was applied.";
			val = 1;
		}
		argList->append(QString::fromStdString(std::string("-t=") + std::to_string(val)));
	};

	// --no-xs
	ComOpt noXsOpt(QStringList{"no-xs"}, "Do not read xs files.", "", "not set");
	auto noXsFunc = [noXsOpt](const QCommandLineParser &parser, QStringList *argList) {
		(void)parser;
		RemoveOptStrings(noXsOpt.names(), argList);
		// core機能なのでcoreの方に-no-xsとして渡す
		argList->append("-no-xs");
	};

	// --xsdir=
	ComOpt xsdirOpt(QStringList{"xsdir"}, "XSDIR file path.", "string", "empty");
	auto xsdirFunc = [xsdirOpt](const QCommandLineParser &parser, QStringList *argList){
		RemoveOptStrings(xsdirOpt.names(), argList);
		argList->append("-xsdir="+parser.value(xsdirOpt));
	};

	// --disable-log
	// このオプションはGUI時のログをコントロールするオプションなのでcoreには与えない
	ComOpt disableLogOpt(QStringList{"disable-log"}, "Disable output to a default log file", "", "not set(windows) / set (ohter)");
	auto disableLogFunc = [disableLogOpt](const QCommandLineParser &parser, QStringList *argList) {
		Q_UNUSED(parser);
		RemoveOptStrings(disableLogOpt.names(), argList);
		global::logFwd.disableLogFile();
	};

	// --enable-log
	ComOpt enableLogOpt(QStringList{"enable-log"}, "Enable output to a default log file", "", "set(windows) / not set(other)");
	auto enableLogFunc = [enableLogOpt](const QCommandLineParser &parser, QStringList *argList) {
		Q_UNUSED(parser);
		RemoveOptStrings(enableLogOpt.names(), argList);
		global::logFwd.enableLogFile("gxsview.log");
	};

	// --color=
	ComOpt colorOpt(QStringList{"color"}, "Apply user defined palette file", "string", "empty");
	auto colorOptFunc = [colorOpt, mainWindow](const QCommandLineParser &parser, QStringList *argList) {
		RemoveOptStrings(colorOpt.names(), argList);
		argList->append("-color="+parser.value(colorOpt));
	};

	// --config-file=
	ComOpt confFileOpt(QStringList{"config-file"}, "Apply config file", "string", "empty");
	auto confFileOptFunc = [confFileOpt, mainWindow](const QCommandLineParser &parser, QStringList *argList) {
		RemoveOptStrings(confFileOpt.names(), argList);
		GuiConfig gconf = GuiConfig::fromJsonFile(parser.value(confFileOpt).toStdString());
		mainWindow->setGuiConfig(gconf);
		mDebug() << "Config set!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
	};
//	// --integer-name
//	ComOpt intNameOpt(QStringList{"integer-name"}, "Treat cell and surface names as integers.", "", "not set");
//	auto intNameFunc = [intNameOpt](const QCommandLineParser &parser, QStringList *argList) {
//		(void)parser;
//		RemoveOptStrings(intNameOpt.names(), argList);
//		// core機能なのでcoreの方に-integer-nameとして渡す
//		argList->append("-integer-name");
//	};



	static const std::vector<GuiOption> optionList{
		{GuiOption(cssOpt, cssFunc)},
		{GuiOption(verboseOpt, verboseFunc)},
		{GuiOption(threadOpt, threadFunc)},
		{GuiOption(noXsOpt, noXsFunc)},
		{GuiOption(xsdirOpt, xsdirFunc)},
		{GuiOption(disableLogOpt, disableLogFunc)},
		{GuiOption(enableLogOpt, enableLogFunc)},
		{GuiOption(colorOpt, colorOptFunc)},
		{GuiOption(confFileOpt, confFileOptFunc)},
	};


	return optionList;
}
