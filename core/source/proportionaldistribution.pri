!PROPORTIONALDISTRIBUTION_PRI{
PROPORTIONALDISTRIBUTION_PRI=1

include ($$PWD/abstractdistribution.pri)

HEADERS *= \
    $$PROJECT/core/source/proportionaldistribution.hpp \
    $$PROJECT/core/utils/numeric_utils.hpp \

SOURCES *= \
    $$PROJECT/core/source/proportionaldistribution.cpp \
    $$PROJECT/core/utils/numeric_utils.cpp \


}
