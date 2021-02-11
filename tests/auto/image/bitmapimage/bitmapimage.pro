#-------------------------------------------------
#
# Project created by QtCreator 2017-01-30T17:15:48
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_bitmapiamge
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += \

SOURCES += tst_bitmapimage.cpp \

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include ($$PWD/../../../testconfig.pri)
include ($$PWD/../../../../core/image/tracingraydata.pri)
include ($$PWD/../../../../core/image/bitmapimage.pri)
include ($$PWD/../../../..//core/image/image.pri)


HEADERS *= \
#    $$PWD/../../../../core/utils/string_utils.hpp \
#    $$PWD/../../../../core/utils/numeric_utils.hpp \
    $$PWD/../../../../core/io/input/dataline.hpp \
    $$PWD/../../../../core/io/input/ijmr.hpp \


SOURCES *= \
#    $$PWD/../../../../core/utils/string_utils.cpp \
#    $$PWD/../../../../core/utils/numeric_utils.cpp \
    $$PWD/../../../../core/io/input/dataline.cpp \
    $$PWD/../../../../core/io/input/ijmr.cpp \
    $$PWD/../../../../core/io/input/common/commoncards.cpp \



