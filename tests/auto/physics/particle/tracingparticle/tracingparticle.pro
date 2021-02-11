#-------------------------------------------------
#
# Project created by QtCreator 2017-01-19T12:26:30
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_tracingparticletest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_tracingparticletest.cpp

HEADERS += 	\

include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/physics/particle/tracingparticle.pri)

HEADERS *= \
    $$PROJECT/core/geometry/cellcreator.hpp \
    $$PROJECT/core/geometry/surfacecreator.hpp \
    $$PROJECT/core/geometry/surf_utils.hpp \

SOURCES *= \
    $$PROJECT/core/geometry/cellcreator.cpp \
    $$PROJECT/core/geometry/surfacecreator.cpp \
    $$PROJECT/core/geometry/surf_utils.cpp \

include ($$PWD/../../../../../core/io/input/cellcard.pri)


