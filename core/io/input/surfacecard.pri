!SURFACECARD_PRI{
SURFACECARD_PRI=1

include ($$PWD/ijmr.pri)
include ($$PWD/../../geometry/surface/surface.pri)
include ($$PWD/../../formula/fortran/fortnode.pri)

HEADERS *= \
    $$PROJECT/core/math/nmatrix.hpp \
    $$PROJECT/core/io/input/surfacecard.hpp \
    $$PROJECT/core/io/input/cardcommon.hpp \

SOURCES *= \
    $$PROJECT/core/io/input/surfacecard.cpp \
    $$PROJECT/core/io/input/cardcommon.cpp \


}
