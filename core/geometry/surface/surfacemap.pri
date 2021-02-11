!SURFACEMAP_PRI{
SURFACEMAP_PRI=1

include ($$PROJECT/core/geometry/surface/surface_utils.pri)
include ($$PROJECT/core/geometry/cell/boundingbox.pri)
include ($$PROJECT/core/utils/matrix_utils.pri)

HEADERS *= \
    $$PROJECT/core/geometry/surface/surfacemap.hpp \
    $$PROJECT/core/math/nvector.hpp \
    $$PROJECT/core/utils/matrix_utils.hpp \
    $$PROJECT/core/utils/string_utils.hpp \
    $$PROJECT/core/io/input/cardcommon.hpp


SOURCES *= \
    $$PROJECT/core/geometry/surface/surfacemap.cpp \
    $$PROJECT/core/math/nvector.cpp \
    $$PROJECT/core/utils/matrix_utils.cpp \
    $$PROJECT/core/utils/string_utils.cpp \
    $$PROJECT/core/io/input/cardcommon.cpp

}
