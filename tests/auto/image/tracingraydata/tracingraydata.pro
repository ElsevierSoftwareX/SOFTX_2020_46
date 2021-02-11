#-------------------------------------------------
#
# Project created by QtCreator 2017-02-01T10:57:30
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_tracingraydatatest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += tst_tracingraydatatest.cpp


include ($$PWD/../../../testconfig.pri)
include ($$PWD/../../../../core/image/tracingraydata.pri)


