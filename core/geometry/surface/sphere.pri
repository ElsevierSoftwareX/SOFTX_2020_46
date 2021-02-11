!SPHERE_PRI{
SPHERE_PRI=1

include ($$PWD/surface.pri)

HEADERS *= \
    $$PROJECT/core/geometry/surface/sphere.hpp \
    $$PROJECT/core/math/nvector.hpp \

SOURCES *= \
    $$PROJECT/core/geometry/surface/sphere.cpp \
    $$PROJECT/core/math/nvector.cpp \


}
