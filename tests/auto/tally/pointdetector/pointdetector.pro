#-------------------------------------------------
#
# Project created by QtCreator 2017-03-14T10:29:51
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_pointdetectortest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += tst_pointdetectortest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

include ($$PWD/../../../testconfig.pri)
include ($$PWD/../../../../core/tally/pointdetector.pri)

include ($$PWD/../../../../core/io/input/phits/phitsinputsection.pri)
include ($$PWD/../../../../core/io/input/dataline.pri)
