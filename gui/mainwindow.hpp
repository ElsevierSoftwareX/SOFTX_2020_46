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
#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <functional>
#include <unordered_map>

#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QTranslator>

#include "option/guiconfig.hpp"
#include "loghighlighter.hpp"
#include "simulationobject.hpp"
#include "tabitem.hpp"
#include "verticaltabwidget.hpp"

class GeometryViewer;
class SectionalViewer;


namespace Ui {
class MainWindow;
}



class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	void init(const QString &initialFileName);
	//explicit MainWindow(QWidget *parent, QStringList argList);
	~MainWindow();
	void setGuiConfig(const GuiConfig &guiConf) {guiConfig_ = guiConf; applyGuiConfigToWidget();}
	GuiConfig guiConfig() const {return guiConfig_;}

signals:
	void requestChangeStyleSheet(const QString &str);
	void inputFilesChanged(std::unordered_multimap<std::string, std::string> inpMap);
	// mainWindowのguiConfig_メンバが変更された時に発信される。
	// GUIパーツはポインタを共有しているのでとくにemitで送る必要はない。
	void guiConfigChanged();

protected:
	void changeEvent(QEvent*event) override final;
	void keyPressEvent(QKeyEvent *event) override;

private:
	Ui::MainWindow *ui;
    //VerticalTabWidget *verticalTabWidget_;
	SimulationObject simulationObj_;
    QString currentInputFile_;
    QString currentTheme_;
	std::unordered_map<int, TabItem*> tabMap_;
	LogHighlighter *higlighter_;
	QString currentLanguage_;
	QTranslator translator_;
	GuiConfig guiConfig_;

	void applyGuiConfigToWidget();  // (configが変更された時などに)guiConfig_の設定を適用する。
	void readSimulationObjectFromFile();
	void loadLanguage(QString languageName);  	// 言語名(ja, en等)を指定して翻訳をロード
	void retranslate();
    // Font
    void setBiggerFont();
    void setSmallerFont();
    // defaultのスタイルシートに戻す。引数がtrueならcssをクリアする。
    void setDefaultStyleSheet(bool flag);


public slots:
	void showStatusMessage(QString str);
	void printLog(QSharedPointer<std::string> strP);
	void setFontSize(int sz);
	void applyTheme(const QString &themeName);


private slots:
    void openFile();
    void showVersion();
	void showLicense();
	void showSystemInfo();
	void updateGlobalNumThreads(int num);
    void setGlobalStyleSheet(QString css);
	void exportTabToRaster();
	void exportTabToVector();

};

#endif // MAINWINDOW_HPP
