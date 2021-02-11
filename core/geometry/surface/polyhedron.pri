!POLYHEDRON_PRI{
POLYHEDRON_PRI=1

include ($$PWD/surface.pri)
include ($$PWD/triangle.pri)

HEADERS *= \
    $$PROJECT/core/geometry/surface/polyhedron.hpp \
    $$PROJECT/core/math/nvector.hpp \

SOURCES *= \
    $$PROJECT/core/geometry/surface/polyhedron.cpp \
    $$PROJECT/core/math/nvector.cpp \

}
