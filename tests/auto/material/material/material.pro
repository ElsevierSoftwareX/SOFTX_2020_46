#-------------------------------------------------
#
# Project created by QtCreator 2017-02-10T09:42:55
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_materialtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_materialtest.cpp

include ($$PWD/../../../testconfig.pri)
include ($$PWD/../../../../core/material/material.pri)
include ($$PWD/../../../../core/material/materials.pri)



TESTXS_PATH = $$clean_path($$PROJECT/component/libacexs/example/tests)
TESTXS_PATH = $$replace(TESTXS_PATH, /, /)
#message("testpath="$$TESTXS_PATH)

DEFINES += TEST_XSDATA_DIR='\\"$$TESTXS_PATH\\"'
#DEFINES += TEST_XSDATA_DIR='\\"/home/\\"'

