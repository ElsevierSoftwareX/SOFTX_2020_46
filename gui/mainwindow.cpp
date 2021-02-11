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
#include "mainwindow.hpp"
#include "ui_mainwindow.h"

#include <string>
#include <thread>
#include <vector>

#include <QDir>
#include <QIcon>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QList>
#include <QLocale>
#include <QMessageBox>
#include <QScrollBar>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>

#include "fileconverter.hpp"
#include "globals.hpp"
#include "languages.hpp"
#include "inputviewer/inputviewer.hpp"
#include "geometryviewer/geometryviewer.hpp"
#include "sectionalviewer/sectionalviewer.hpp"
#include "xsviewer/xsviewer.hpp"
#include "../core/image/cellcolorpalette.hpp"
#include "../core/option/config.hpp"
#include "../core/utils/message.hpp"
#include "../core/utils/system_utils.hpp"
#include "guiutils.hpp"
#include "subdialog/licensedialog.hpp"
#include "subdialog/messagebox.hpp"

namespace {
const int MAX_LOG_LINES = 1000;
const int MAX_FONT_POINT = 50;
const int MIN_FONT_POINT = 5;
}

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	 ui->setupUi(this);
     ui->tabWidget->setTabPosition(QTabWidget::West);
     QTabBar *bar = ui->tabWidget->tabBar();
     // icon original sizeは　180:160 = 9:8
     bar->setIconSize(QSize(70, 75));
}


void MainWindow::init(const QString &initialFileName)
{
	currentInputFile_ = initialFileName;
	ui->splitter->setOpaqueResize(false);
	ui->plainTextEditForLog->setMaximumBlockCount(MAX_LOG_LINES);
    higlighter_ = new LogHighlighter(ui->plainTextEditForLog->document(), global::isDarkTheme);

	// タブ生成
	GeometryViewer *geometryViewer = new GeometryViewer(this, tr(""), &guiConfig_);
	geometryViewer->setIndex(ui->tabWidget->addTab(geometryViewer, QIcon("://icon/svg/3d.svg"), geometryViewer->tabText()));
	tabMap_[geometryViewer->index()] = geometryViewer;
	SectionalViewer *sectionalViewer = new SectionalViewer(this, tr(""), &guiConfig_);
	sectionalViewer->setIndex(ui->tabWidget->addTab(sectionalViewer, QIcon("://icon/svg/2d.svg"), sectionalViewer->tabText()));
	tabMap_[sectionalViewer->index()] = sectionalViewer;
	XsViewer *xsViewer = new XsViewer(this, tr(""), &guiConfig_);
	xsViewer->setIndex(ui->tabWidget->addTab(xsViewer, QIcon("://icon/svg/xs.svg"), xsViewer->tabText()));
    tabMap_[xsViewer->index()] = xsViewer;
	InputViewer *inputViewer = new InputViewer(this, tr(""),  &guiConfig_);
	inputViewer->setIndex(ui->tabWidget->addTab(inputViewer, QIcon("://icon/svg/input.svg"), inputViewer->tabText()));
	tabMap_[inputViewer->index()] = inputViewer;


	// ログ管理関係のwidgetをstatusbarに移動
	ui->statusBar->addWidget(ui->labelLog);
	ui->statusBar->addWidget(ui->checkBoxLogHide);
	ui->statusBar->addWidget(ui->pushButtonClearLog);


	// 2DViewer
	connect(sectionalViewer, SIGNAL(localNumThreadsChanged(int)), this, SLOT(updateGlobalNumThreads(int)));
	// inputviewer
	connect(this, &MainWindow::guiConfigChanged, inputViewer, &InputViewer::handleGuiConfigChanged);

	// ############ Menu
	// tabに応じてメニューのenable/disableを変更する
    ui->menu_Edit->setDisabled(true);
    ui->menu_Export3D->setDisabled(true);
	ui->actionto_raster_graphics->setDisabled(true);
	ui->actionto_vector_graphics->setDisabled(true);
	ui->actionto_text_data->setDisabled(true);
	/*
	 * 3Dtab: 3d, raster
	 * 2Dtab: raster
	 * xstab: raster, vector, csv
	 */
	connect(geometryViewer, &GeometryViewer::tabFocused,
			this, [=](bool flag) {
				ui->menu_Export3D->setEnabled(flag);
				ui->actionto_raster_graphics->setEnabled(flag);
			});
	connect(sectionalViewer, &SectionalViewer::tabFocused,
			this, [=](bool flag) {
				ui->actionto_raster_graphics->setEnabled(flag);
			});
	connect(xsViewer, &XsViewer::tabFocused,
			this,[=](bool flag) {
				ui->actionto_raster_graphics->setEnabled(flag);
				ui->actionto_vector_graphics->setEnabled(flag);
				ui->actionto_text_data->setEnabled(flag);
			});
	connect(inputViewer, &InputViewer::tabFocused,
			this, [=](bool flag) {
				ui->menu_Edit->setEnabled(flag);
				ui->menu_Export->setDisabled(flag);
			});

	// Edit
	connect(ui->actionSave, &QAction::triggered, inputViewer, &InputViewer::handleSaveAction);
	connect(ui->actionReload, &QAction::triggered, inputViewer, &InputViewer::handleReloadAction);
	connect(ui->actionCut, &QAction::triggered, inputViewer, &InputViewer::handleCutAction);
	connect(ui->actionCopy, &QAction::triggered, inputViewer, &InputViewer::handleCopyAction);
	connect(ui->actionPaste,  &QAction::triggered, inputViewer, &InputViewer::handlePasteAction);
	connect(ui->actionFind,  &QAction::triggered, inputViewer, &InputViewer::handleFindAction);
	connect(ui->actionUndo,  &QAction::triggered, inputViewer, &InputViewer::handleUndoAction);
	connect(ui->actionRedo,  &QAction::triggered, inputViewer, &InputViewer::handleRedoAction);
    connect(ui->actionCommentRegion, &QAction::triggered, inputViewer, &InputViewer::handleCommentRegionAction);
	// FIXME inputViewerのui->tabWidgetで表示されているFileTabItemが変わった時、変更あり、undoable,redoableを調べてactionSave/Undo/Redoをenable/disableする
	// PlainTextEdit→FileTabItem→InputViewerの順にsignal伝播

	// Theme
	connect(ui->action_DefaultTheme, &QAction::triggered, [=](){applyTheme(QString());});
	QDir cssDir("://css");
	for(auto &cssName: cssDir.entryList()) {  // themeはリソースファイルをスキャンして追加
		auto themeAction = new QAction(cssName, this);
		connect(themeAction, &QAction::triggered, [=](){applyTheme(cssName);});
		ui->menu_Theme->addAction(themeAction);
	}
    this->setDefaultStyleSheet(true);

	// Lang
	// 初期設定言語＝システムの言語を適用
	// 翻訳ファイルは gui_ja.qmみたいな感じでlocaleの先頭2文字を末尾に入れる
	QString systemLocaleName = QLocale::system().name();		// "ja_JP"
	systemLocaleName.truncate(systemLocaleName.lastIndexOf('_')); // "ja"
	QString initialLangFile = QString("://lang/gui_") + systemLocaleName + ".qm";
	currentLanguage_ = systemLocaleName;
	translator_.load(initialLangFile);
	QApplication::installTranslator(&translator_);
	connect(ui->action_DefaultLang, &QAction::triggered, [=](){loadLanguage("en");});
	QDir langDir("://lang");
	// langFileName:言語ファイル名 gui_ja.qm, langname:言語名 ja,  一般言語名japanese
	for(auto &langFileName: langDir.entryList()) {
		// langName = ja, en等
		QString langName = QString::fromStdString(fileToLangName(langFileName.toStdString()));
		auto langAction = new QAction(QLocale::languageToString(QLocale(langName).language()), this);
		connect(langAction, &QAction::triggered, [=](){loadLanguage(langName);});
		ui->menu_Language->addAction(langAction);
	}
    // Font
    connect(ui->actionFontExpand, &QAction::triggered, [=](){setBiggerFont();});
    connect(ui->actionFontShrink, &QAction::triggered, [=](){setSmallerFont();});


	// Fileメニュー
	connect(ui->action_Open, &QAction::triggered,
			[=](){
				// file読み込み中はファイル読み込みアクションを無効化する。
				ui->action_Reload->setDisabled(true);
				ui->action_Open->setDisabled(true);
				if(inputViewer->confirmUnsavedFiles()) openFile();
				ui->action_Reload->setDisabled(false);
				ui->action_Open->setDisabled(false);
			});
	connect(ui->action_Reload, &QAction::triggered,
			[=]() {
                if(!inputViewer->confirmUnsavedFiles()) return;
                // ファイルを再読込する場合、一部のconfigは初期化する必要がある。
                guiConfig_.cuiConfig.colorMap.clear();
                readSimulationObjectFromFile();
//				ui->action_Reload->setDisabled(true);
//				ui->action_Open->setDisabled(true);
				ui->action_Reload->setDisabled(false);
				ui->action_Open->setDisabled(false);
			});
	// Exportメニュー
	connect(ui->action_vtp, &QAction::triggered, [=](){geometryViewer->exportTo(ff::FORMAT3D::VTP);});
	connect(ui->action_vtk, &QAction::triggered, [=](){geometryViewer->exportTo(ff::FORMAT3D::VTK);});
	connect(ui->action_ply, &QAction::triggered, [=](){geometryViewer->exportTo(ff::FORMAT3D::PLY);});
	connect(ui->action_stl, &QAction::triggered, [=](){geometryViewer->exportTo(ff::FORMAT3D::STL);});
	connect(ui->actionto_raster_graphics, &QAction::triggered,
			this, [=](){qobject_cast<TabItem*>(ui->tabWidget->currentWidget())->exportToRasterGraphics();});
	connect(ui->actionto_vector_graphics, &QAction::triggered,
			this, [=](){qobject_cast<TabItem*>(ui->tabWidget->currentWidget())->exportToVectorGraphics();});
	connect(ui->actionto_text_data, &QAction::triggered, this, [=](){xsViewer->exportToTextData();});


	// Tools
	connect(ui->action_Configuration, &QAction::triggered,
			this, [=]()
			{
                // simulationオブジェクトが読み込み済みでない場合はどうするか？
                std::map<std::string, img::MaterialColorData> currentMap;
                if(simulationObj_.simulation()) currentMap = simulationObj_.simulation()->defaultPalette()->colorMap();

                guiConfig_ = GuiConfig::getGuiConfig(guiConfig_, currentMap);
                applyGuiConfigToWidget();
                emit guiConfigChanged();
			});
    connect(ui->actionSave_config_file, &QAction::triggered,
            this, [=]()
            {
                std::string fileName = QFileDialog::getSaveFileName(this).toStdString();
                if(!fileName.empty()) {
                    std::ofstream ofs(utils::utf8ToSystemEncoding(fileName).c_str());
                    guiConfig_.dumpJson(ofs);
                }
            });
    connect(ui->actionLoad_config_file, &QAction::triggered,
            this, [=]()
            {
                std::string fileName = QFileDialog::getOpenFileName(this).toStdString();
                if(!fileName.empty()) {
                    std::ifstream ifs(utils::utf8ToSystemEncoding(fileName).c_str());
                    if(ifs.fail()) {
                        mWarning() << "Config file =" << fileName << "not found.";
                        return;
                    }
                    guiConfig_ = GuiConfig::fromJsonString(std::string(std::istreambuf_iterator<char>(ifs),
                                                                       std::istreambuf_iterator<char>()));
                    applyGuiConfigToWidget();
                    emit guiConfigChanged();
                }
            });


	// help
	connect(ui->action_Version, &QAction::triggered, this, &MainWindow::showVersion);
	connect(ui->action_License, &QAction::triggered, this, &MainWindow::showLicense);
	connect(ui->action_System_information, &QAction::triggered, this, &MainWindow::showSystemInfo);

	// SimulationObjectからのイベント伝搬 signal/slot
	connect(&simulationObj_, &SimulationObject::simulationChanged,
			geometryViewer, &GeometryViewer::updateSimulationDataX);
	connect(&simulationObj_, &SimulationObject::simulationChanged,
			sectionalViewer, &SectionalViewer::updateSimulationData);
	connect(&simulationObj_, &SimulationObject::simulationChanged,
			xsViewer, &XsViewer::updateSimulationData);
	// 入力ファイルが読み込まれたらシグナルを出すのでinputViewerへ伝える。
	connect(&simulationObj_, &SimulationObject::fileOpenSucceeded,
			inputViewer, &InputViewer::receiveOpenedFileName);
	// config変更もsimulationObj経由で伝える
	connect(this, &MainWindow::guiConfigChanged,
			&simulationObj_, &SimulationObject::handleGuiConfigChanged);
	connect(&simulationObj_, &SimulationObject::requestUpdateCellColor,
			geometryViewer, &GeometryViewer::updateViewColorOnly);


	// グローバルなログ転送クラスからのログをlineEdit表示する
    connect(&global::logFwd, &LogForwarder::fatalMessage, this, &MainWindow::printLog);
	connect(&global::logFwd, &LogForwarder::logMessage, this, &MainWindow::printLog);
    // fatalメッセージは当該箇所表示のためにInputVeiwwerへも流す
    connect(&global::logFwd, &LogForwarder::fatalMessage, inputViewer, &InputViewer::receiveFatalLog);

	// 3D描画終了でwindowsをアクティブにする showNomalを呼ぶとウインドウ最大化が解除されるので削除。
	connect(geometryViewer, &GeometryViewer::updateGeomViewFinished, this, [=](){raise();activateWindow();});
	connect(sectionalViewer, &SectionalViewer::updateSectViewFinished, this, [=](){raise();activateWindow();});

	// GUIの初期値調整。と引数で指定されている場合の初期入力ファイル読み込み
	QTimer::singleShot(0, [=](){
		double ratio = 0.8;
		int h = ui->centralWidget->size().height();
		QList<int> szs{static_cast<int>(h*ratio), static_cast<int>(h*(1-ratio))};
		ui->splitter->setSizes(szs);
		if(!currentInputFile_.isEmpty()) {
			readSimulationObjectFromFile();
		}
	});



	// 表示タブが変わったら初期化関数を呼ぶようにする。
	connect(ui->tabWidget, &QTabWidget::currentChanged, [=](int index){tabMap_.at(index)->init();});
	// 現在表示タブの初期化関数を呼ぶ
	tabMap_.at(ui->tabWidget->currentIndex())->init();

	//リリースビルドの場合初期状態でデバッグログを隠す
#ifdef QT_NO_DEBUG
	ui->plainTextEditForLog->hide();
	ui->checkBoxLogHide->setChecked(true);
#endif


}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::readSimulationObjectFromFile()
{
    QFileInfo info(currentInputFile_);
	this->simulationObj_.clear();

   try {

		this->simulationObj_.readFile(currentInputFile_.toStdString(), &guiConfig_);
		// ファイル読み取りに成功したらtabは先頭(3Dviewerを想定)をcurrentにする
        ui->tabWidget->setCurrentIndex(0);
		//fileNames = simulationObj_.simulation()->inputFileNames();
    } catch (std::exception &e) {
#ifndef FATAL_ABORT
        try{
#endif
            // エラー処理(発生箇所の伝搬など)のためmFatalを呼ぶが終了はさせない。
            GMessageBox::critical(Q_NULLPTR, "critical erorr", QString::fromStdString(e.what()), true);
            mFatal(e.what());
#ifndef FATAL_ABORT
        } catch (...) {

        }
#endif


        this->simulationObj_.clear();
        // エラー発生時にはInputViewerをcurrentにする。
        ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    }


	// 読み取りに成功したらタイトルを変更する。
	this->setWindowTitle(currentInputFile_);

	if(guiConfig_.cuiConfig.verbose && simulationObj_.simulation()) {
		mDebug() << "Input data:";
		mDebug() << simulationObj_.simulation()->finalInputText();
	}
}


bool isDark(const QString &css)
{
	static const QRegExp widBgReg(R"(QWidget\s*\{([^{}]*)\})", Qt::CaseInsensitive);
	int pos = widBgReg.indexIn(css, 0);
	if(pos != -1) {
		static const QRegExp bgReg(R"(background-color:\s*(#.*);)", Qt::CaseInsensitive);
		pos = bgReg.indexIn(widBgReg.cap(1));
		if(pos != -1) {
			QColor col(bgReg.cap(1));
			//mDebug() << "colorStr===" << bgReg.cap(1);
			//mDebug() << "color===" << col.red() << col.green() << col.blue() << "v=" << col.value();
			return col.value() < 255*0.5;
		}
	}
	// widgetのbg色指定なしは非dark
	return false;
}



void MainWindow::applyTheme(const QString &themeName)
{
	//mDebug() << "Enter apply Theme !!!!! confirmFontSize===" << updateFontSize;
    currentTheme_ = themeName;
	//global::isDarkTheme = (currentTheme_.indexOf("dark", 0, Qt::CaseInsensitive) != -1);

	// isDark(css)はcss文字列が判明しないとdarkかどうかわからない。
	QString cssString;
	if(currentTheme_.isEmpty()) {
		global::isDarkTheme = isDark(cssString);
		emit requestChangeStyleSheet(cssString);
	} else {
		// cssファイルは "themeName"/style.qss
		QFile cssFile("://css/" + currentTheme_ + "/style.qss");
		if (cssFile.open(QFile::ReadOnly | QFile::Text)) {
			QTextStream ts(&cssFile);
			cssString = ts.readAll();
			global::isDarkTheme = isDark(cssString);
			emit requestChangeStyleSheet(cssString);
		} else {
			mWarning() << "Stylesheet = " << cssFile.fileName() << " not found.";
		}
	}
    // NOTE テーマを設定するとフォントサイズが小さくなることがあるので再度フォントサイズを設定する。
	//if(updateFontSize) this->setFontSize(QFont().pointSize());
}

// languageNameはja, en等
void MainWindow::loadLanguage(QString languageName)
{
	if(languageName == currentLanguage_) return;

	qApp->removeTranslator(&translator_);
	QLocale locale(languageName);
	QLocale::setDefault(locale);
	if(languageName == "en") {
		qApp->removeTranslator(&translator_);
		currentLanguage_ = languageName;
	} else {
		QString languageFileName = "://lang/gui_" + languageName + ".qm";
		if(translator_.load(languageFileName)) {
			mDebug() << "ロードする言語名 = " << QLocale::languageToString(locale.language());
			qApp->installTranslator(&translator_);
			currentLanguage_ = languageName;
		} else {
			mWarning() << "No language file found =" << languageFileName << ", no translation applied.";
		}
	}
}

void MainWindow::retranslate()
{
	ui->retranslateUi(this);
	for(auto &item: tabMap_) {
		//  NOTE 自分で作成したクラスはui->translateUiできないので手動で作業
		(item.second)->retranslate();
        ui->tabWidget->setTabText(item.first, QApplication::translate("MainWindow", (item.second)->tabText().toLatin1(), Q_NULLPTR));
	}
}


void MainWindow::openFile()
{
    currentInputFile_ = QFileDialog::getOpenFileName(this, "Open file");
    /*
     * NOTE QtのFileDialogでファイルを選択するとパスセパレータは￥ではなく／となる。
     * 入力ファイルに手書きで入力されるファイルパスのセパレータは普通￥なので
     * ここでは￥に統一する。
	 * Qtで扱う分にはどちらでも問題ないが、入力ファイル中での区切り文字にどちらが使われているか
	 * が問題となる。
     */
    currentInputFile_ = currentInputFile_.replace(QChar('/'), QChar(PATH_SEP));
	if(!currentInputFile_.isEmpty()) {
		QString absPath = QFileInfo(currentInputFile_).absolutePath();
		// カレントディレクトリを変更する。
		// 極力絶対パスを扱っているが一部対応に限界が有る(STLファイル読み込みなど)ため
		QDir::setCurrent(absPath);
		this->readSimulationObjectFromFile();
	}
}




//TODO もうすこしみばえよく。QDialogのほうが良いかも
void MainWindow::showVersion()
{
    QMessageBox msgBox(this);
    //メッセージ
    msgBox.setText(tr("Version") + ": " + QString(global::VERSION_STR));
    //タイトル
    //msgBox.setWindowTitle(tr("タイトル"));
    //ボタン
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();

}

void MainWindow::showLicense()
{
	LicenseDialog(this).exec();
}

#include "subdialog/systeminfodialog.hpp"
void MainWindow::showSystemInfo()
{
	SystemInfoDialog(this).exec();
}

void MainWindow::updateGlobalNumThreads(int num)
{
	guiConfig_.cuiConfig.numThread = num;
}


#include <QThread>
#include <QApplication>
#include <QMetaObject>
void MainWindow::setGlobalStyleSheet(QString css)
{
#ifdef WORKAROUND_QTBUG69204
	/*
     * NOTE スタイルシート問題個別のインスタンスでsetStyleSheetされている場合に
	 * QApplication::setStyleSheetを実行するとsegmentation faultが起こる場合がある。
	 * QTBUG-69878
	 */
	//qApp->setStyleSheet("");  // 効果なし
	//QMetaObject::invokeMethod(qApp, "setStyleSheet", Q_ARG(QString, css)); // 別スレッドを疑ったが効果なし
	// css = "QTabWidget::pane {border: 0 solid white; margin: -9px -9px -9px -9px;}"; // こういうのでもだめ
	// スタイルシート変更前に各インスタンスのcssをクリアすると治る(baaimoaru)
	// しかしこれはバグのため本来不要であるうえ100%防げるわけではない。
	// QTBUG-69878バグ https://bugreports.qt.io/browse/QTBUG-69878
	// QTBUG-69204は69878に包含される。

	for(auto &tabPair: tabMap_) {
		tabPair.second->clearStyleSheet();
	}
    // mainwindowもmacでのタブ表示問題を解決するためデフォルトでcssを設定しているのでクリアする。
    setDefaultStyleSheet(false);

	// おまじない程度の回避策 https://forum.qt.io/topic/56129/exception-on-setstylesheet-can-t-figure-why/10

    qApp->style()->unpolish(qApp);
#endif
	qApp->setStyleSheet(css);

}

void MainWindow::exportTabToRaster()
{
	qobject_cast<TabItem*>(ui->tabWidget->currentWidget())->exportToRasterGraphics();
}

void MainWindow::exportTabToVector()
{
	qobject_cast<TabItem*>(ui->tabWidget->currentWidget())->exportToVectorGraphics();
}



void MainWindow::showStatusMessage(QString str) {ui->statusBar->showMessage(str);}

//void MainWindow::printLog(const QString &str)
//void MainWindow::printLog(QSharedPointer<QString> strP)
void MainWindow::printLog(QSharedPointer<std::string> strP)
{
    ui->plainTextEditForLog->appendPlainText(QString::fromStdString(*(strP.data())));
}



void MainWindow::setFontSize(int sz)
{
	guiConfig_.uiFont.setPointSize(sz);
	QApplication::setFont(guiConfig_.uiFont);

	// NOTE スタイルシートが設定されているとQApplication:setFontの結果が外見に反映されない。
	// https://stackoverflow.com/questions/9308715/setstylesheet-fixates-font-no-longer-updates-for-font-propagation
	// スタイルシートのfont変更伝搬とQAppのfont変更伝搬システムは異なるから反映されないらしい。

	// ということなので強引な解決法として、一旦スタイルシートをクリアしてから再設定している。
	auto theme = currentTheme_;
    setDefaultStyleSheet(false);
    if(theme.isEmpty()) {
        setDefaultStyleSheet(true);
    } else {
        applyTheme(theme);
    }

	// フォント変更後は changeEvent(QEvent::FontChange:)が発生するので独自のsignalをemitする必要はない。

}

void MainWindow::setBiggerFont()
{
    int currentPoint = QFont().pointSize();
    if(currentPoint < MAX_FONT_POINT) {
        int delta = std::max(1, static_cast<int>(currentPoint*1.1 - currentPoint));
        setFontSize(currentPoint + delta);
    }
}

void MainWindow::setSmallerFont()
{
    int currentPoint = QFont().pointSize();
    if(currentPoint > MIN_FONT_POINT) {
        int delta = std::max(1, static_cast<int>(currentPoint - currentPoint*0.9));
        setFontSize(currentPoint - delta);
    }
}

// デフォルトでタブの色を設定しないとmacでは下が透けてしまう
// flagがfalseならcssをクリアするだけ
void MainWindow::setDefaultStyleSheet(bool flag)
{
    if(!flag) {
        ui->tabWidget->setStyleSheet("");
	}
//Qt5.12でmojaveでの表示が改善して背景を設定しなくても良くなるかとおもったらそんなことはなかった。
	else {
#if defined __APPLE__
		auto selected = QWidget::palette().color(QPalette::Highlight);
		auto bdColor = QWidget::palette().color(QPalette::Midlight);
		auto bgColor = QWidget::palette().color(QPalette::Window);
		auto fgColor = QWidget::palette().color(QPalette::WindowText);

		QString css = "QTabBar::tab {"
				"color: " + fgColor.name() + ";"
				"border: 0.5px solid " + bdColor.name() + ";"
				"background-color: " + bgColor.name() + ";"   // #323232
			"}\n";

		// TODO macとlinuxでwidgetのバックグラウンド色は違っていて、macでは別にselectedも白くならない。
		//     macのデフォルトのselectedは青色。まあどうてもいいけど。
		css += "QTabBar::tab:selected {"
				 "background-color: " + selected.name() + ";"
				"}";
#ifdef WORKAROUND_QTBUG69204
        // Qt5.12では未解決なのでこの措置は気休め。QTBUG69204の本質的な解決にはなっていない。
		ui->tabWidget->style()->unpolish(ui->tabWidget);
#endif  // end WORKAROUND_QTBUG69204

		ui->tabWidget->setStyleSheet(css);
#endif  // end __APPLE_
	}
}


void MainWindow::changeEvent(QEvent *event)
{
    if (Q_NULLPTR == event) return;
	//mDebug() << "ChangeEvent========== type=" << event->type();
	switch(event->type()) {
	case QEvent::LanguageChange:
		// アプリの言語が変更された場合はここ
		this->retranslate();
		break;
	case QEvent::LocaleChange:
		// システムの言語が変更された場合はここに来る。
		{
		QString locale = QLocale::system().name();
		locale.truncate(locale.lastIndexOf('_'));
		loadLanguage(locale);
		}
		break;
	case QEvent::StyleChange:
		// darkスタイルの場合logerのハイライターもダークに変更する
		delete higlighter_;
        higlighter_ = new LogHighlighter(ui->plainTextEditForLog->document(), global::isDarkTheme);
        if(currentTheme_.isEmpty()) setDefaultStyleSheet(true);
		break;
//	case QEvent::FontChange:
//		break;
	default:
		break;
	}
	QMainWindow::changeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	if ((event->modifiers() & Qt::ControlModifier) && (event->key() == Qt::Key_PageUp)) {
		int index = ui->tabWidget->currentIndex();
		index = (index < ui->tabWidget->count()-1) ? index+1 : 0;
		ui->tabWidget->setCurrentIndex(index);
		return;
	} else 	if ((event->modifiers() & Qt::ControlModifier) && (event->key() == Qt::Key_PageDown)) {
		int index = ui->tabWidget->currentIndex();
		index = (index > 0) ? index-1 : ui->tabWidget->count()-1;
		ui->tabWidget->setCurrentIndex(index);
		return;
	}
	QMainWindow::keyPressEvent(event);
}

void MainWindow::applyGuiConfigToWidget()
{
	QApplication::setFont(guiConfig_.uiFont);


}

