#-------------------------------------------------
#
# Project created by QtCreator 2017-05-12T16:19:31
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_cellcardtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_cellcardtest.cpp
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += SRCDIR=\\\"$$PWD/\\\"

include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/io/input/cellcard.pri)





