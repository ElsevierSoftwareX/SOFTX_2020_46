!PHITSENERGYDISTRIBUTION {
PHITSENERGYDISTRIBUTION = 1

include ($$PWD/../multigroupdistribution.pri)
include ($$PWD/../discretedistribution.pri)
include ($$PWD/../proportionaldistribution.pri)
include ($$PWD/phitsdistribution.pri)

HEADERS *= \
    $$PROJECT/core/source/phits/phitsenergydistribution.hpp \
    $$PROJECT/core/utils/string_utils.hpp \

SOURCES *= \
    $$PROJECT/core/source/phits/phitsenergydistribution.cpp \
    $$PROJECT/core/utils/string_utils.cpp \

}
