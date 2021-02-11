!MATRIX_UTILS_PRI{
MATRIX_UTILS_PRI=1

include ($$PWD/../../core/formula/fortran/fortnode.pri)

HEADERS *= \
    $$PROJECT/core/math/nmatrix.hpp \
    $$PROJECT/core/utils/string_utils.hpp \
    $$PROJECT/core/utils/matrix_utils.hpp \

SOURCES *= \
    $$PROJECT/core/utils/string_utils.cpp \
    $$PROJECT/core/utils/matrix_utils.cpp \

}
