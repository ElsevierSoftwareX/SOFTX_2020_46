#-------------------------------------------------
#
# Project created by QtCreator 2018-07-04T15:51:23
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_planetest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += DIRECTUSE_LIBACEXS
win32{
    mingw{
        # MinGW
        COMPILER=mingw
    } else {
        # VC++
        COMPILER=msvc
        QMAKE_CXXFLAGS += /utf-8 /wd4503 /wd4267
    }
}

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/geometry/surface/plane.pri)

SOURCES += \
        tst_planetest.cpp  \


DEFINES += SRCDIR=\\\"$$PWD/\\\" \


