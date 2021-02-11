QT += testlib
QT += gui
QT    += widgets
CONFIG += qt warn_on depend_includepath testcase

TEMPLATE = app

include ($$PWD/../../../testconfig.pri)

SOURCES +=  tst_progress_utils.cpp \
        $$PWD/../../../../core/utils/progress_utils.cpp \
        $$PWD/../../../../core/utils/numeric_utils.cpp \

HEADERS += $$PWD/../../../../core/utils/progress_utils.hpp
