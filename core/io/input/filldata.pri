!FILLDATA_PRI {

FILLDATA_PRI = 1

include ($$PWD/../../utils/matrix_utils.pri)
include ($$PWD/../../geometry/cell/cell.pri)
include ($$PWD/filling_utils.pri)
include ($$PWD/cellcard.pri)

# cellcard と filldataは相互依存しているので互いをincludeすると循環してしまう。

HEADERS *= \
    $$PROJECT/core/io/input/filldata.hpp \
    $$PROJECT/core/io/input/baseunitelement.hpp \

SOURCES *= \
    $$PROJECT/core/io/input/filldata.cpp \
    $$PROJECT/core/io/input/baseunitelement.cpp \


}

