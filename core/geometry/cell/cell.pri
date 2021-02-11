!CELL_PRI{
CELL_PRI=1

# 依存するクラスのpriを読み込む
include ($$PROJECT/core/geometry/cell/boundingbox.pri)
include ($$PROJECT/core/geometry/surface/surfacemap.pri)
include ($$PROJECT/component/libacexs/libacexs.pri)

SOURCES *= \
    $$PROJECT/core/geometry/cell/cell.cpp \
    $$PROJECT/core/physics/physconstants.cpp \
#  ↑cell.hppの時点で必要なファイル。
    $$PROJECT/core/material/material.cpp \
    $$PROJECT/core/utils/time_utils.cpp \
# ↑cell.cppで直接必要になっているファイル。
#  ここまでのファイルは直接依存なので分離不可能。
    $$PROJECT/core/material/nuclide.cpp \ # materialで必要

HEADERS *= \
    $$PROJECT/core/geometry/cell/cell.hpp \
    $$PROJECT/core/formula/logical/lpolynomial.hpp \
    $$PROJECT/core/physics/physconstants.hpp \
    $$PROJECT/core/material/material.hpp \
    $$PROJECT/core/utils/time_utils.hpp \
    $$PROJECT/core/material/nuclide.hpp \ # materialで必要

}
