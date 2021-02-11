QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app



include ($$PWD/../../../testconfig.pri)
include ($$PWD/../../../../core/image/color.pri)

SOURCES +=  tst_color.cpp



