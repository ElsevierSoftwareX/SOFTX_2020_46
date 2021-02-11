#-------------------------------------------------
#
# Project created by QtCreator 2017-03-14T14:41:47
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_phits_inputsubsectiontest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app



SOURCES += tst_phits_inputsubsectiontest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

include ($$PWD/../../../../../testconfig.pri)
include ($$PWD/../../../../../../core/io/input/phits/phitsinputsubsection.pri)
