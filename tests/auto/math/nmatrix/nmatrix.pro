#-------------------------------------------------
#
# Project created by QtCreator 2017-01-13T15:46:09
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_nmatrixtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

include ($$PWD/../../../testconfig.pri)
include ($$PWD/../../../../core/math/nmatrix.pri)
include ($$PWD/../../../../core/formula/fortran/fortran.pri)


SOURCES *= \
    tst_nmatrixtest.cpp  \
    $$PROJECT/core/math/nvector.cpp \
    $$PROJECT/core/utils/numeric_utils.cpp \
    $$PROJECT/core/utils/matrix_utils.cpp \
    $$PROJECT/core/utils/string_utils.cpp \
    $$PROJECT/core/formula/formula_utils.cpp \

HEADERS += \
    $$PROJECT/core/math/nvector.hpp \
    $$PROJECT/core/utils/numeric_utils.hpp \
    $$PROJECT/core/utils/matrix_utils.hpp \
    $$PROJECT/core/utils/string_utils.hpp \
    $$PROJECT/core/formula/formula_utils.hpp \

DEFINES += SRCDIR=\\\"$$PWD/\\\"
