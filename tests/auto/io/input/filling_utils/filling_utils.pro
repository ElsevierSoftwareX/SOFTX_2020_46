QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

TARGET = filling_utils

SOURCES +=  tst_filling_utils.cpp \


include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/io/input/filling_utils.pri)


