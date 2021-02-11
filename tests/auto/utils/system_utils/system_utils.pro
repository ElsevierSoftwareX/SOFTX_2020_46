QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

include ($$PWD/../../../testconfig.pri)


HEADERS *=  $$PROJECT/core/utils/system_utils.hpp

SOURCES *=  tst_system_util.cpp \
            $$PROJECT/core/utils/system_utils.cpp
