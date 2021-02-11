!MATERIAL_PRI{
MATERIAL_PRI=1

include ($$PWD/../../component/libacexs/libacexs.pri)
include ($$PWD/../../core/formula/fortran/fortnode.pri)

HEADERS *= \
    $$PROJECT/core/material/material.hpp \
    $$PROJECT/core/material/nuclide.hpp \
    $$PROJECT/core/physics/physconstants.hpp \
    $$PROJECT/core/material/nmtc.hpp \
    $$PROJECT/core/utils/time_utils.hpp \
    $$PROJECT/core/utils/numeric_utils.hpp \
    $$PROJECT/core/utils/string_utils.hpp \
    $$PROJECT/core/utils/system_utils.hpp \


SOURCES *= \
    $$PROJECT/core/material/material.cpp \
    $$PROJECT/core/material/nuclide.cpp \
    $$PROJECT/core/physics/physconstants.cpp \
    $$PROJECT/core/material/nmtc.cpp \
    $$PROJECT/core/utils/time_utils.cpp \
    $$PROJECT/core/utils/numeric_utils.cpp \
    $$PROJECT/core/utils/string_utils.cpp \
    $$PROJECT/core/utils/system_utils.cpp \


}
