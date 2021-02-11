!PHITSINPUTSECTION_PRI{
PHITSINPUTSECTION_PRI=1

include ($$PWD/../dataline.pri)
include ($$PWD/../inputdata.pri)

HEADERS *= \
    $$PROJECT/core/io/input/phits/phitsinputsection.hpp \
    $$PROJECT/core/io/input/original/original_metacard.hpp \

SOURCES *= \
    $$PROJECT/core/io/input/phits/phitsinputsection.cpp \
    $$PROJECT/core/io/input/original/original_metacard.cpp \

}
