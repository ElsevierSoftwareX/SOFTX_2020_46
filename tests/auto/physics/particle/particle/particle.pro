#-------------------------------------------------
#
# Project created by QtCreator 2016-12-06T17:09:51
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_particle_testtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += \
    tst_particletest.cpp



DEFINES += SRCDIR=\\\"$$PWD/\\\"

include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/physics/particle/particle.pri)

HEADERS += \
    $$PROJECT/core/geometry/surf_utils.hpp \
    $$PROJECT/core/geometry/cell_utils.hpp \

SOURCES *=  \
    $$PROJECT/core/geometry/surf_utils.cpp \
    $$PROJECT/core/geometry/cell_utils.cpp \
