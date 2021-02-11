#-------------------------------------------------
#
# Project created by QtCreator 2017-01-31T20:03:38
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_tracingparticle2test
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app



SOURCES += tst_tracingparticle2test.cpp

include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/physics/particle/tracingparticle.pri)

# ここからテスト用ファイル。
include ($$PWD/../../../../../core/formula/logical/lpolynomial.pri)

HEADERS *= \
    $$PROJECT/core/geometry/cellcreator.hpp \
    $$PROJECT/core/geometry/surfacecreator.hpp \
    $$PROJECT/core/geometry/surf_utils.hpp \
    $$PROJECT/core/image/cellcolorpalette.hpp \

SOURCES *= \
    $$PROJECT/core/geometry/cellcreator.cpp \
    $$PROJECT/core/geometry/surfacecreator.cpp \
    $$PROJECT/core/geometry/surf_utils.cpp \
    $$PROJECT/core/image/cellcolorpalette.cpp \

include ($$PWD/../../../../../core/io/input/cellcard.pri)
include ($$PWD/../../../../../core/image/tracingraydata.pri)
include ($$PWD/../../../../../core/image/color.pri)
include ($$PWD/../../../../../core/image/bitmapimage.pri)
