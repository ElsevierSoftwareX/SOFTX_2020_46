#-------------------------------------------------
#
# Project created by QtCreator 2016-11-17T16:04:18
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_message_testtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


include ($$PWD/../../../testconfig.pri)

SOURCES *=  tst_messagetest.cpp \
    $$PROJECT/core/utils/stream_utils.hpp \


HEADERS *= \
    $$PROJECT/core/utils/message.hpp \
    $$PROJECT/core/utils/stream_utils.hpp


DEFINES += SRCDIR=\\\"$$PWD/\\\"
