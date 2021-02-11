!CELLCARD_PRI{
CELLCARD_PRI=1



include($$PWD/../../formula/fortran/fortnode.pri)
include($$PWD/../../formula/logical/lpolynomial.pri)
include($$PWD/../../geometry/surface/surfacemap.pri)
include($$PWD/../../geometry/cell/cell.pri)
include($$PWD/filling_utils.pri)
include($$PWD/dataline.pri)
#include($$PWD/cellparameter.pri)
#include($$PWD/ijmr.pri)

HEADERS *= \
    $$PROJECT/core/io/input/cellcard.hpp \
    $$PROJECT/core/io/input/cellparameter.hpp \
    $$PROJECT/core/io/input/filldata.hpp \
    $$PROJECT/core/io/input/baseunitelement.hpp \
    $$PROJECT/core/io/input/surfacecard.hpp \
    $$PROJECT/core/io/input/cardcommon.hpp \
    $$PROJECT/core/math/nmatrix.hpp \
    $$PROJECT/core/geometry/cellcreator.hpp \
    $$PROJECT/core/geometry/surfacecreator.hpp \
    $$PROJECT/core/utils/string_utils.hpp \
# ここから間接依存
    $$PROJECT/core/geometry/cell_utils.hpp \ # Cellcreatorに必要
    $$PROJECT/core/geometry/tetracreator.hpp \ # Cellcreatorに必要
    $$PROJECT/core/geometry/latticecreator.hpp \ # Cellcreatorに必要
    $$PROJECT/core/geometry/tetrahedron.hpp \ # TetraCreatorに必要
    $$PROJECT/core/geometry/surf_utils.hpp \ # SurfaceCreatorに必要

SOURCES *= \
    $$PROJECT/core/io/input/cellcard.cpp \
    $$PROJECT/core/io/input/cellparameter.cpp \
    $$PROJECT/core/io/input/filldata.cpp \
    $$PROJECT/core/io/input/baseunitelement.cpp \
    $$PROJECT/core/io/input/surfacecard.cpp \
    $$PROJECT/core/io/input/cardcommon.cpp \
    $$PROJECT/core/geometry/cellcreator.cpp \
    $$PROJECT/core/geometry/surfacecreator.cpp \
    $$PROJECT/core/utils/string_utils.cpp \
# ここから間接依存
    $$PROJECT/core/geometry/cell_utils.cpp \
    $$PROJECT/core/geometry/tetracreator.cpp \ # Cellcreatorに必要
    $$PROJECT/core/geometry/latticecreator.cpp \ # Cellcreatorに必要
    $$PROJECT/core/geometry/tetrahedron.cpp \ # TetraCreatorに必要
    $$PROJECT/core/geometry/surf_utils.cpp \ # SurfaceCreatorに必要
}




