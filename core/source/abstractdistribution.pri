!ABSTRACTDISTRIBUTION_PRI {
ABSTRACTDISTRIBUTION_PRI = 1
include ($$PWD/../io/input/dataline.pri)
include ($$PWD/../formula/fortran/fortnode.pri)
include ($$PWD/../io/input/phits/phits_metacards.pri)

HEADERS *= \
    $$PROJECT/core/source/abstractdistribution.hpp \
    $$PROJECT/core/math/constants.hpp \

SOURCES *= \
    $$PROJECT/core/source/abstractdistribution.cpp \
    $$PROJECT/core/math/constants.cpp \

}
