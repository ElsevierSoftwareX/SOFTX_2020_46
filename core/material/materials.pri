!MATERIALS_PRI{
MATERIALS_PRI=1


include ($$PWD/../io/input/dataline.pri)


HEADERS *= \
    $$PROJECT/core/material/materials.hpp \
    $$PROJECT/core/io/input/common/commoncards.hpp \
    $$PROJECT/core/io/input/original/original_metacard.hpp \

SOURCES *= \
    $$PROJECT/core/material/materials.cpp \
    $$PROJECT/core/io/input/common/commoncards.cpp \
    $$PROJECT/core/io/input/original/original_metacard.cpp \

}
