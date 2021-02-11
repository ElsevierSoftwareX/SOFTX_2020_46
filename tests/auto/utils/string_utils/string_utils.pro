#-------------------------------------------------
#
# Project created by QtCreator 2016-11-18T22:39:50
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_string_testtest
CONFIG   += console
CONFIG   -= app_bundle


TEMPLATE = app


include ($$PWD/../../../testconfig.pri)

SOURCES *=  tst_stringtest.cpp \
    $$PROJECT/core/utils/string_utils.cpp \
    $$PROJECT/core/utils/system_utils.cpp \
    $$PROJECT/core/utils/numeric_utils.cpp \
    $$PROJECT/core/math/constants.cpp \


HEADERS *= \
    $$PROJECT/core/utils/string_utils.hpp \
    $$PROJECT/core/utils/system_utils.hpp \
    $$PROJECT/core/utils/numeric_utils.hpp \
    $$PROJECT/core/math/constants.hpp \

DEFINES += SRCDIR=\\\"$$PWD/\\\"
