 #-------------------------------------------------













































isEmpty(VTK_MAJOR_VER){
    #VTK_MAJOR_VER = 8.2
    VTK_MAJOR_VER = 9.0
    warning("\""$$VTK_MAJOR_VER"\""" is set to VTK_MAJOR_VER since it is not specified on the qmake command line.")
}

QT       += core gui widgets
TEMPLATE = app

CONFIG(debug,debug|release) {
    TARGET   = gxsview.debug
} else {
    TARGET   = gxsview
    #DEFINES += NO_DEBUG_OUT
}
DEFINES += DISABLE_VTK_WARNING



CONFIG += c++17
CONFIG +=sdk_no_version_check

DEFINES += QT_DEPRECATED_WARNINGS

target.path = ../tests/bin/
INSTALLS += target

RESOURCES += \
    resources/css/darkorange/darkorangestyle.qrc \
    resources/css/qdarkstyle/qdarkstyle.qrc \
    resources/lang/languages.qrc \
    resources/icon/icons.qrc


INCLUDEPATH += ../

DEFINES += ENABLE_GUI
DEFINES += DIRECTUSE_LIBACEXS
DEFINES += QT_NO_VERSION_TAGGING


SOURCES += \
    fileconverter.cpp \
    geometryviewer/cellobject.cpp \
    geometryviewer/polyconstructor.cpp \
    mainwindow.cpp \
    geometryviewer/geometryviewer.cpp \
    simulationobject.cpp \
    geometryviewer/camerainfo.cpp \
    geometryviewer/namecomparefunc.cpp \
    geometryviewer/custamtablewidget.cpp \
    sectionalviewer/sectionalviewer.cpp \
    subdialog/systeminfodialog.cpp \
    tabitem.cpp \
    sectionalviewer/xpmarray.cpp \
    guimain.cpp \
    languages.cpp \
    inputviewer/inputviewer.cpp \
    inputviewer/filetabitem.cpp \
    inputviewer/customplaintextedit.cpp \
    geometryviewer/settingpane.cpp \
    geometryviewer/cellpane.cpp \
    geometryviewer/axisarrows.cpp \
    geometryviewer/geometryviewerconfig.cpp \
    loghighlighter.cpp \
    inputviewer/phitshighlighter.cpp \
    subdialog/licensedialog.cpp \
    xsviewer/xsviewer.cpp \
    xsviewer/nuclidetablewidget.cpp \
    xsviewer/linestyle.cpp \
    xsviewer/chartconfig.cpp \
    xsviewer/chartconfigdialog.cpp \
    xsviewer/xsiteminfo.cpp \
    option/guioption.cpp \
    inputviewer/basehighlighter.cpp \
    inputviewer/mcnphighlighter.cpp \
    geometryviewer/colorpane.cpp \
    subdialog/messagebox.cpp \
    geometryviewer/bbcalculator.cpp \
    geometryviewer/geometryviewer.vtkeventhandler.cpp \
    geometryviewer/customqvtkwidget.cpp \
    geometryviewer/cell3dexporter.cpp \
    option/guiconfig.cpp \
    option/guiconfigdialog.cpp \
    guiutils.cpp \
    geometryviewer/polypainter.cpp \
    geometryviewer/cellexportdialog.cpp



HEADERS  += \
    fileconverter.hpp \
    geometryviewer/cellobject.hpp \
    geometryviewer/polyconstructor.hpp \
    mainwindow.hpp \
    geometryviewer/geometryviewer.hpp \
    simulationobject.hpp \
    geometryviewer/camerainfo.hpp \
    geometryviewer/namecomparefunc.hpp \
    geometryviewer/custamtablewidget.hpp \
    sectionalviewer/sectionalviewer.hpp \
    subdialog/systeminfodialog.hpp \
    tabitem.hpp \
    sectionalviewer/xpmarray.hpp \
    languages.hpp \
    globals.hpp \
    qvtkopenglwrapperwidget.hpp \
    inputviewer/inputviewer.hpp \
    inputviewer/filetabitem.hpp \
    inputviewer/customplaintextedit.hpp \
    geometryviewer/settingpane.hpp \
    geometryviewer/cellpane.hpp \
    geometryviewer/axisarrows.hpp \
    geometryviewer/geometryviewerconfig.hpp \
    geometryviewer/vtkinclude.hpp \
    loghighlighter.hpp \
    inputviewer/phitshighlighter.hpp \
    subdialog/licensedialog.hpp \
    xsviewer/xsviewer.hpp \
    xsviewer/nuclidetablewidget.hpp \
    xsviewer/linestyle.hpp \
    xsviewer/chartconfig.hpp \
    xsviewer/chartconfigdialog.hpp \
    xsviewer/xsiteminfo.hpp \
    option/guioption.hpp \
    inputviewer/basehighlighter.hpp \
    inputviewer/mcnphighlighter.hpp \
    geometryviewer/mousevtkpicker.hpp \
    verticaltabwidget.hpp \
    geometryviewer/colorpane.hpp \
    subdialog/messagebox.hpp \
    geometryviewer/bbcalculator.hpp \
    geometryviewer/customqvtkwidget.hpp \
    geometryviewer/cell3dexporter.hpp \
    option/guiconfig.hpp \
    option/guiconfigdialog.hpp \
    guiutils.hpp \
    geometryviewer/polypainter.hpp \
    geometryviewer/cellexportdialog.hpp


FORMS    += \
    mainwindow.ui \
    geometryviewer/geometryviewer.ui \
    sectionalviewer/sectionalviewer.ui \
    inputviewer/inputviewer.ui \
    inputviewer/filetabitem.ui \
    geometryviewer/settingpane.ui \
    geometryviewer/cellpane.ui \
    subdialog/licensedialog.ui \
    subdialog/systeminfodialog.ui \
    xsviewer/xsviewer.ui \
    xsviewer/nuclidetablewidget.ui \
    xsviewer/chartconfigdialog.ui \
    geometryviewer/colorpane.ui \
    option/guiconfigdialog.ui \
    geometryviewer/cellexportdialog.ui


PROJECT = ..
include($$PROJECT/core/core.pri)

LIBACEXS_SRCDIR = ../component/libacexs/libsrc
include (../component/libacexs/libacexs.pri)

include(../component/vtk/vtkCustom.pri)




lessThan(VTK_MAJOR_VER, 9) {
    include("vtk8.pri")
} else {
    include("vtk9.pri")
}


isEmpty(VTK_DIR) {
    #VTK_MINOR_VER = 0
    VTK_MINOR_VER = 1
    BUILD_MODE=release
    CONFIG(debug,debug|release) {BUILD_MODE=debug}
    win32{
        # compiler
        COMPILER=msvc # MSVC 64bit
        !contains(QMAKE_TARGET.arch, x86_64) {COMPILER=msvc32} # MSVC 32bit
        # vtk_dir
		VTK_DIR = C:\vtk\vtk$${VTK_MAJOR_VER}.$${VTK_MINOR_VER}.mp.$${BUILD_MODE}.$${COMPILER}
    }
    unix{
		VTK_DIR = /opt/vtk/vtk$${VTK_MAJOR_VER}.$${VTK_MINOR_VER}.mp.$${BUILD_MODE}
        LIBS += -L/usr/lib/x86_64-linux-gnu -lfontconfig
    }
    warning("\""$$VTK_DIR"\""" is set to VTK_DIR since it is not specified on the qmake command line.")
}

LIBS += -L$$VTK_DIR/lib
win32{
	QMAKE_CXXFLAGS += /utf-8 /wd4503 /wd4267
	LIBS += -L$$VTK_DIR\bin -lAdvapi32 -lOpengl32
	!lessThan(VTK_MAJOR_VER, 9) {LIBS += -lUser32 -lgdi32 -lWs2_32 -lPsapi -lDbghelp}
}
unix{
	LIBS += -L/usr/lib/x86_64-linux-gnu -lfontconfig
}

INCLUDEPATH += ../ $$VTK_DIR/include/vtk-$$VTK_MAJOR_VER
QMAKE_LFLAGS += "-Wl,-rpath,$$VTK_DIR/lib"

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib
} # end mac
