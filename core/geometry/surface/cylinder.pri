!CYLINDER_PRI{
CYLINDER_PRI=1

include ($$PWD/surface.pri)

HEADERS *= \
    $$PROJECT/core/geometry/surface/cylinder.hpp \
    $$PROJECT/core/geometry/surface/sphere.hpp \
    $$PROJECT/core/math/nvector.hpp \



SOURCES *= \
    $$PROJECT/core/geometry/surface/cylinder.cpp \
    $$PROJECT/core/geometry/surface/sphere.cpp \
    $$PROJECT/core/math/nvector.cpp \

}
