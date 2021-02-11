!QUADRIC_PRI{
QUADRIC_PRI=1

include ($$PWD/surface.pri)

HEADERS *= \
    $$PROJECT/core/geometry/surface/quadric.hpp \
    $$PROJECT/core/geometry/surface/quadric.bs.hpp \

SOURCES *= \
    $$PROJECT/core/geometry/surface/quadric.cpp \
    $$PROJECT/core/geometry/surface/quadric.bs.cpp \

}
