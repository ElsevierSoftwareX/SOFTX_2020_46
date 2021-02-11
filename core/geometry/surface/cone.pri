!CONE_PRI{
CONE_PRI=1

include ($$PWD/surface.pri)

SOURCES *= \
    $$PROJECT/core/geometry/surface/cone.cpp \
    $$PROJECT/core/utils/numeric_utils.cpp \
    $$PROJECT/core/math/nvector.cpp \


HEADERS *= \
    $$PROJECT/core/geometry/surface/cone.hpp \
    $$PROJECT/core/utils/numeric_utils.hpp \
    $$PROJECT/core/math/nvector.hpp \


}
