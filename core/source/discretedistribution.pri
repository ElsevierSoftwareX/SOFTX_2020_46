!DISCRETEDISTRIBUTION_PRI{
DISCRETEDISTRIBUTION_PRI = 1

include ($$PWD/abstractdistribution.pri)

HEADERS *= \
    $$PROJECT/core/source/discretedistribution.hpp \
    $$PROJECT/core/utils/container_utils.hpp \


SOURCES *= \
    $$PROJECT/core/source/discretedistribution.cpp \
    $$PROJECT/core/utils/container_utils.cpp \


}
