#-------------------------------------------------
#
# Project created by QtCreator 2017-04-26T10:43:51
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_surfacemaptest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

unix {
#QMAKE_CXX = clang++
}


# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
DEFINES += SRCDIR=\\\"$$PWD/\\\"
include ($$PWD/../../../../testconfig.pri)



include ($$PWD/../../../../../core/geometry/surface/surfacemap.pri)

SOURCES *= \
    tst_surfacemaptest.cpp \

SOURCES *= $$PWD/../../../../../core/geometry/surf_utils.cpp
include ($$PWD/../../../../../core/geometry/cell/cell.pri)
