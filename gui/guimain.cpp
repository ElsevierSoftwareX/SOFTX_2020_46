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

#include <functional>
#include <map>
#include <regex>
#include <string>
#include <unordered_map>


#include <QApplication>
#include <QDateTime>

#include <QDesktopWidget>
#include <QLocale>

#include <QScreen>
#include <QStringList>
#include <QSurfaceFormat>
#include <QTimer>
#include "qvtkopenglwrapperwidget.hpp"
#include <vtkObject.h>

#define DEFINE_GLOBAL_
#include "globals.hpp"
// log転送クラスはグローバルオブジェクトにする。のでDEFINE_GLOBAL_が必要。
#define DEFINE_GLOBAL_
#include "../core/utils/message.hpp"
#undef DEFINE_GLOBAL_

#include "option/guioption.hpp"
#include "../core/utils/system_utils.hpp"
#include "../core/utils/string_utils.hpp"
#include "../core/option/config.hpp"
#include "option/guiconfig.hpp"
#include "subdialog/messagebox.hpp"

/*
 * 基本方針
 *
 * 1．QWidgetのメモリ管理は極力コンストラクタ時parent指定の自動管理を利用する。
 *		・QVTK関係ではstd::shared_ptrやQSharedがうまく動作しないことがある。
 *		・std::shared_ptr::getでconnectポインタ破棄時の自動disconnectのタイミングが不定な場合がある
 *		・parent未指定newは極力使用しない
 * 2．QTableViewのようにポインタをセットすると以後そのクラスがdeleteするようになるものもあるので
 *    その場合は仕方ないから生ポインタを使う(そうでないと2重開放になるのでshared_ptr::getを渡してはならない)
 *
 */


// FIXME polyhedronを断面カットすると断面が閉じない。多分Polyhedronの陰関数は多分内と外の判定が逆転している
#include <QCommandLineParser>

#include <QMetaType>
#include <QMetaObject>
#include <QString>
#include <QSharedPointer>
Q_DECLARE_METATYPE(QSharedPointer<QString>)
Q_DECLARE_METATYPE(QSharedPointer<std::string>)
Q_DECLARE_METATYPE(std::string)


//#include <QDebug>
//#include <QOpenGLContext>
//#include <QOpenGLFunctions>

#include "guiutils.hpp"
/*
 * GUIへのオプションはロングスタイルのみにする。
 * ショートオプションはCUIへ渡す用にする。
 */



#define QUOTE(ARG) QQQ(ARG)
#define QQQ(ARG) #ARG


int main(int argc, char *argv[])
{
#ifdef DISABLE_VTK_WARNING
    vtkObject::GlobalWarningDisplayOff();
#endif
    qRegisterMetaType<QSharedPointer<QString>>("QSharedPointer<QString>");
    qRegisterMetaType<QSharedPointer<QString>>("QSharedPointer<std::string>");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::unordered_multimap<std::string, std::string>>("std::unordered_multimap<std::string, std::string>");
    qRegisterMetaType<std::pair<std::string, std::string>>("std::pair<std::string, std::string>");  // simulationクラスでファイルオープンに成功したことを伝える。
	qRegisterMetaType<GuiConfig>("GuiConfig");
	// 日付チェックするならここで

#ifdef ENABLE_EXPIRE_DATE
	QDateTime current = QDateTime::currentDateTime();
	const QString BETA_TEST_END = QUOTE(ENABLE_EXPIRE_DATE);
	QDateTime limitDay = QDateTime::fromString(BETA_TEST_END, "yyyy-MM-dd");
	qWarning() << "Expire day =" << limitDay;
	auto daysto = current.daysTo(limitDay);
	if(daysto < 0) {
        GMessageBox::warning(Q_NULLPTR, "warning", "Beta test license has expired at " + BETA_TEST_END, true);
		std::exit(EXIT_FAILURE);
	} else {
		mWarning() << "This beta test license will expire at " + BETA_TEST_END;
	}
#endif

    QApplication app (argc, argv);
    // これがないとまともに動作しない
    // https://github.com/Kitware/VTK/blob/master/Examples/GUI/Qt/SimpleView/main.cxx 等参照
//    QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());
//    QSurfaceFormat::setDefaultFormat(QVTKOpenGLStereoWidget::defaultFormat());
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLWrapperWidget::defaultFormat());

	// デフォルトでlogファイル書き込みを有効化する(windowsでは)
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
	global::logFwd.enableLogFile("gxsview.log");
#endif


    // ここから引数とオプションの処理
    // オプションの中にはmainWindowのslotにシグナル接続するものがあるので
    // オプション処理の前にmainWindowのインスタンスを作る
    MainWindow *mainWindow = new MainWindow(Q_NULLPTR);
    QStringList argList = QApplication::arguments();

    /*
     * Option処理は
     * 1．GUIオプションを処理
     * 2．残った引数からCUIオプションを生成
     * という手順。
     */
    QCommandLineParser parser;
    parser.addHelpOption();  	//--help,-hをQCommandLineParserで自動で処理するように依頼
    parser.addPositionalArgument("filename","input file.");  //オプション以外の純粋な引数についての指定とその説明
    std::vector<GuiOption> optList = GuiOption::createOptionList(mainWindow);
    for(auto &opt: optList) parser.addOption(opt.qOption());
    //parse()がfalseを返す場合未定義のオプションが設定されているのでエラー
    if(!parser.parse(argList)){
        QString error_str("illegal option --");
        for(auto &errName: parser.unknownOptionNames()) error_str.append("\"" + errName + "\"");
        mFatal{error_str};
    } else {
        parser.process(argList);
    }
    // オプションの実行。同じオプションが複数あった場合は先に出てくる値の方が有効となる(今の実装では)。
    // オプションを実行すると(1)オプションに応じたargListの加工、(2)グローバル変数への反映の何れかが行われる。
    for(auto &opt: optList) if(parser.isSet(opt.qOption())) opt.proc(parser, &argList);
    // guiオプションの作成。今の所コマンドラインオプションから永続的に設定されるGUI独自の項目はないのでここでguiConfigを生成して良い。
    std::vector<std::string> argStrList;
    for(auto &qstr: argList) argStrList.push_back(qstr.toStdString());
	// GUIの設定構造体はcuiの設定構造体を含む。
	GuiConfig guiConfig = mainWindow->guiConfig();
	mDebug() << "config:\n" << guiConfig.cuiConfig.toString();
	guiConfig.cuiConfig.procOptions(&argStrList);
    // argListの方からはオプション文字列は削除されていないことに注意。
    mainWindow->setGuiConfig(guiConfig);
    if(argStrList.size() == 1) argStrList.push_back("");  // ダミーの初期inputファイル名として空の文字列を追加
    assert(argStrList.size() == 2); // ここでargStrListはプログラム名と引数の2個になっているはず。
    mainWindow->init(QString::fromStdString(argStrList.at(1)));  // mainwindowには正味の引数のみ(現時点ではプログラム名とphits入力ファイル名のみ。)

    QRect rec = QGuiApplication::screens().at(0)->geometry();
	mainWindow->resize(static_cast<int>(rec.width()*0.8),  static_cast<int>(rec.height()*0.8));
    mainWindow->show();

    QObject::connect(mainWindow, SIGNAL(requestChangeStyleSheet(QString)),
                     mainWindow, SLOT(setGlobalStyleSheet(QString)));


    // OpenGL バージョンチェック
    // QVTKOpenGLWidgetが可視状態になるまではQOpenGLContext::currentContextはnullptr
    // なのでイベントループに入ってからチェックを実行する。
    // イベントループ内でのwidgetのshowとopenGLチェックの実行順序は不定なので
    // 再度イベントループに入ってから実行するようにQTimer::singleShotは入れ子にする。
        QTimer::singleShot(100, [=](){
            QTimer::singleShot(100, [=](){
            constexpr double REQUIRED_OPENGL_VERSION = 2.0;
            constexpr double REQUIRED_MESA_VERSION = 3.2;

//            std::cout << "Start OpenGL check!!!!!!";
            auto glinfo = gutils::getOpenGLInfo();
            if((glinfo.isMesa && glinfo.version < REQUIRED_MESA_VERSION)||(glinfo.version < REQUIRED_OPENGL_VERSION)) {
                QString message = QObject::tr("OpenGL version");
                message += (glinfo.version < 0) ? "(Not found) " :  "(=" + QString::number(glinfo.version) + ") ";
                message += QObject::tr("does not meet the requirement.");
                message += "<ul>";
                message += "<li> > " + QString::number(REQUIRED_OPENGL_VERSION, 'f', 1) + " for hardware or</li>"
                        +  "<li> > " + QString::number(REQUIRED_MESA_VERSION, 'f', 1) + " for mesa.</li>";
                message += "</ul>";
                message += QObject::tr("3D geometry and xs viewer may not work precisely.<br><br>");
                message += QString("System OpenGL info:<br>");
                if(glinfo.version < 0) {
                    message += "Not available.";
                } else {
                    message += " vendor = "   + QString::fromStdString(glinfo.vendor) + "<br>"
                            + " renderer = " + QString::fromStdString(glinfo.renderer) + "<br>"
                            + " version = "  + QString::fromStdString(glinfo.versionStr);
                }
                GMessageBox::critical(Q_NULLPTR, QObject::tr("warning"), message, true);
            }
            });
        });


    return app.exec();
}

#undef QQQ
#undef QUOTE
