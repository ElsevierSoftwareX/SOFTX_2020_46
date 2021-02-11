!TORUS_PRI{
TORUS_PRI=1

include ($$PWD/surface.pri)
include ($$PWD/../../utils/matrix_utils.pri)

HEADERS *= \
    $$PROJECT/core/geometry/surface/torus.hpp \
    $$PROJECT/core/math/equationsolver.hpp \
    $$PROJECT/core/math/nvector.hpp \


SOURCES *=\
    $$PROJECT/core/geometry/surface/torus.cpp \
    $$PROJECT/core/math/nvector.cpp \


}



