QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_filldata.cpp

include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/io/input/filldata.pri)
