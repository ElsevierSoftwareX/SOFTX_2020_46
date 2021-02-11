#-------------------------------------------------
#
# Project created by QtCreator 2017-02-15T12:25:55
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_phitsenergydistributiontest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += tst_phitsenergydistributiontest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/source/phits/phitsenergydistribution.pri)


