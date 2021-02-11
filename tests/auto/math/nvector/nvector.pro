#-------------------------------------------------
#
# Project created by QtCreator 2017-01-16T11:52:36
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_nvectortest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

include ($$PWD/../../../testconfig.pri)
include ($$PWD/../../../../core/math/nvector.pri)

SOURCES += tst_nvectortest.cpp



DEFINES += SRCDIR=\\\"$$PWD/\\\"
