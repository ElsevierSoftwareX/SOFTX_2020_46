#-------------------------------------------------
#
# Project created by QtCreator 2017-01-11T09:40:57
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_phits_metacardstest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += tst_phits_metacardstest.cpp
HEADERS += \



DEFINES += SRCDIR=\\\"$$PWD/\\\"

include (../../../../testconfig.pri)
include (../../../../../core/input/phits/phits_metacards.pri)
