HEADERS *= \
    $$PROJECT/core/geometry/geometry.hpp \
    $$PROJECT/core/geometry/geom_utils.hpp \
    $$PROJECT/core/geometry/surfacecreator.hpp \
    $$PROJECT/core/geometry/cellcreator.hpp \
    $$PROJECT/core/geometry/latticecreator.hpp \
    $$PROJECT/core/geometry/tetracreator.hpp \
    $$PROJECT/core/geometry/tetrahedron.hpp \
    $$PROJECT/core/geometry/tracingworker.hpp \
    $$PROJECT/core/utils/progress_utils.hpp \


SOURCES *= \
    $$PROJECT/core/geometry/geometry.cpp \
    $$PROJECT/core/geometry/geom_utils.cpp \
    $$PROJECT/core/geometry/surfacecreator.cpp \
    $$PROJECT/core/geometry/cellcreator.cpp \
    $$PROJECT/core/geometry/latticecreator.cpp \
    $$PROJECT/core/geometry/tetracreator.cpp \
    $$PROJECT/core/geometry/tetrahedron.cpp \
    $$PROJECT/core/geometry/tracingworker.cpp \
    $$PROJECT/core/utils/progress_utils.cpp \



include($$PWD/cell/cell.pri)
include($$PWD/surface/surface.pri)
include($$PWD/macro/macro.pri)

