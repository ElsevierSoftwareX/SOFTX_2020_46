!PHITSDISTRIBUTION_PRI {
PHITSDISTRIBUTION_PRI=1

HEADERS *= \
    $$PROJECT/core/source/phits/phitsdistribution.hpp \
    $$PROJECT/core/utils/numeric_utils.hpp \

SOURCES *= \
    $$PROJECT/core/source/phits/phitsdistribution.cpp \
    $$PROJECT/core/utils/numeric_utils.cpp \

include ($$PWD/../../formula/fortran/fortnode.pri)
include ($$PWD/../../io/input/phits/phits_metacards.pri)


}
