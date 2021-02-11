#-------------------------------------------------
#
# Project created by QtCreator 2017-02-06T11:00:42
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_terminaltest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -lpthread


include ($$PWD/../../testconfig.pri)
# InteractivePlotterはsimulationクラスを使うので全ファイル必要
include ($$PWD/../../../core/core.pri)
include ($$PWD/../../../component/libacexs/libacexs.pri)

SOURCES += tst_terminaltest.cpp \


