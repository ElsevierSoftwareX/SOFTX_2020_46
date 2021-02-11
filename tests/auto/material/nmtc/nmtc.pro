QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

include ($$PWD/../../../testconfig.pri)


HEADERS += $$PWD/../../../../core/material/nmtc.hpp
SOURCES +=  tst_nmtc.cpp \
    $$PWD/../../../../core/material/nmtc.cpp \

