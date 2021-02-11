!MULTIGROPUDISTRIBUTION_PRI{
MULTIGROPUDISTRIBUTION_PRI=1

include ($$PWD/abstractdistribution.pri)

HEADERS *= \
    $$PROJECT/core/source/multigroupdistribution.hpp \
    $$PROJECT/core/utils/numeric_utils.hpp \

SOURCES *= \
    $$PROJECT/core/source/multigroupdistribution.cpp \
    $$PROJECT/core/utils/numeric_utils.cpp \

}
