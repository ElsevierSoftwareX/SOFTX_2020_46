!MESHDATA_PRI{
MESHDATA_PRI=1


include ($$PWD/../../formula/fortran/fortnode.pri)
include ($$PWD/../input/phits/phits_metacards.pri)
include ($$PWD/../input/dataline.pri)


HEADERS *= \
    $$PROJECT/core/io/input/meshdata.hpp \
    $$PROJECT/core/utils/numeric_utils.hpp \

SOURCES *= \
    $$PROJECT/core/io/input/meshdata.cpp \
    $$PROJECT/core/utils/numeric_utils.cpp \


}
