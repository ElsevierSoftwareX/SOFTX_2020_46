QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle
DEFINES += USE_CSGJS

TEMPLATE = app


include ($$PWD/../../../../testconfig.pri)
include ($$PWD/../../../../../core/formula/logical/lpolynomial.pri)
include ($$PWD/../../../../../core/geometry/surface/surface.pri)
include ($$PWD/../../../../../core/geometry/surface/surfacemap.pri)
include ($$PWD/../../../../../core/geometry/surface/plane.pri)
include ($$PWD/../../../../../core/geometry/surface/sphere.pri)
include ($$PWD/../../../../../core/geometry/surface/cylinder.pri)
include ($$PWD/../../../../../core/geometry/surface/cone.pri)
include ($$PWD/../../../../../core/geometry/surface/torus.pri)
include ($$PWD/../../../../../component/csgjs-cpp/csgjs.pri)

SOURCES *=  tst_createcsgmodel.cpp \
    $$PWD/../../../../../core/geometry/surf_utils.cpp \



HEADERS *= \
    $$PWD/../../../../../core/geometry/surf_utils.hpp \
