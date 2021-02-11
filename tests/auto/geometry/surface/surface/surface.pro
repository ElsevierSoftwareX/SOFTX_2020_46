#-------------------------------------------------
#
# Project created by QtCreator 2016-11-18T12:24:48
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_surfacetest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_surfacetest.cpp \


HEADERS += \


DEFINES += SRCDIR=\\\"$$PWD/\\\"

include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/geometry/surface/sphere.pri)
include ($$PWD/../../../../../core/geometry/surface/plane.pri)
include ($$PWD/../../../../../core/geometry/surface/cylinder.pri)
include ($$PWD/../../../../../core/geometry/surface/surfacemap.pri)
