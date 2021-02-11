#-------------------------------------------------
#
# Project created by QtCreator 2016-11-25T09:41:56
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_inputdata_testtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES +=  tst_inputdatatest.cpp


HEADERS +=


DEFINES += SRCDIR=\\\"$$PWD/\\\"

include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/io/input/inputdata.pri)


