!PHITSANGULARDISTRIBUTION_PRI {
PHITSANGULARDISTRIBUTION_PRI = 1

include ($$PWD/../discretedistribution.pri)
include ($$PWD/../multigroupdistribution.pri)
include ($$PWD/../proportionaldistribution.pri)

include ($$PWD/phitsdistribution.pri)
include ($$PWD/../../io/input/dataline.pri)
include ($$PWD/../../io/input/phits/phits_metacards.pri)
include ($$PWD/../../formula/fortran/fortnode.pri)



HEADERS *= \
    $$PROJECT/core/source/phits/phitsangulardistribution.hpp \
    $$PROJECT/core/math/constants.hpp \
    $$PROJECT/core/physics/physconstants.hpp \


SOURCES *= \
    $$PROJECT/core/source/phits/phitsangulardistribution.cpp \
    $$PROJECT/core/math/constants.cpp \
    $$PROJECT/core/physics/physconstants.cpp \




}
