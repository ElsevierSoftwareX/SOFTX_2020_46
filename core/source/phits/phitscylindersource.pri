!PHITSCYLINDERSOURCE_PRI {
PHITSCYLINDERSOURCE_PRI=1 

include ($$PWD/phitssource.pri)
include ($$PWD/phitsangulardistribution.pri)
include ($$PWD/phitsenergydistribution.pri)
include ($$PWD/../discretedistribution.pri)
include ($$PWD/../multigroupdistribution.pri)
include ($$PWD/../proportionaldistribution.pri)
include ($$PWD/../../formula/fortran/fortnode.pri)
include ($$PWD/../../io/input/dataline.pri)
include ($$PWD/../../io/input/meshdata.pri)
include ($$PWD/../../io/input/phits/phits_metacards.pri)
include ($$PWD/../../io/input/phits/phitsinputsection.pri)


HEADERS *= \
    $$PROJECT/core/source/phits/phitscylindersource.hpp \
    $$PROJECT/core/math/nvector.hpp \
    $$PROJECT/core/utils/container_utils.hpp \

SOURCES *= \
    $$PROJECT/core/source/phits/phitscylindersource.cpp \
    $$PROJECT/core/math/nvector.cpp \
    $$PROJECT/core/utils/container_utils.cpp \




}
