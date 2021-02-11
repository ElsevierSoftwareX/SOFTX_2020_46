#-------------------------------------------------
#
# Project created by QtCreator 2017-02-15T18:41:01
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_phitsangulardistributiontest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app



SOURCES += tst_phitsangulardistributiontest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/source/phits/phitsangulardistribution.pri)


