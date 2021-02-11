#-------------------------------------------------
#
# Project created by QtCreator 2017-05-30T09:09:31
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_equationsolvertest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app



SOURCES += tst_equationsolvertest.cpp

include ($$PWD/../../../testconfig.pri)
include ($$PWD/../../../../core/math/equationsolver.pri)

