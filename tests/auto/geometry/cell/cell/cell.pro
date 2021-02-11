#-------------------------------------------------
#
# Project created by QtCreator 2016-11-18T12:09:21
#
#-------------------------------------------------

QT       += testlib
QT       -= gui

TARGET = tst_celltest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/geometry/cell/cell.pri)

SOURCES *=  \
    tst_celltest.cpp \
    $$PWD/../../../../../core/geometry/surf_utils.cpp \


include ($$PWD/../../../../../component/libacexs/libacexs.pri)


