#-------------------------------------------------
#
# Project created by QtCreator 2016-11-25T16:02:53
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_trcard_testtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_trcardtest.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include ($$PWD/../../../../../testconfig.pri)
include ($$PWD/../../../../../../core/io/input/common/trcard.pri)



