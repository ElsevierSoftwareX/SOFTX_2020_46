!TRCARD_PRI{
TRCARD_PRI=1

include ($$PWD/../ijmr.pri)
include ($$PWD/../dataline.pri)
include ($$PWD/../../../formula/fortran/fortnode.pri)
include ($$PWD/../../../utils/matrix_utils.pri)

HEADERS *= \
    $$PROJECT/core/math/nmatrix.hpp \
    $$PROJECT/core/math/nvector.hpp \
    $$PROJECT/core/io/input/common/trcard.hpp \
    $$PROJECT/core/io/input/common/datacard.hpp \

SOURCES *= \
    $$PROJECT/core/io/input/common/trcard.cpp \
    $$PROJECT/core/io/input/common/datacard.cpp \
    $$PROJECT/core/math/nvector.cpp \
}
