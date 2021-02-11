!PHITSINPUT_PRI{
PHITSINPUT_PRI=1

include ($$PWD/../../../physics/particle/particle.pri)
include ($$PWD/../inputdata.pri)
include ($$PWD/phitsinputsection.pri)

HEADERS *= \
    $$PROJECT/core/io/input/phits/phitsinput.hpp \
    $$PROJECT/core/math/nmatrix.hpp \
    $$PROJECT/core/io/input/mcmode.hpp \

SOURCES *= \
    $$PROJECT/core/io/input/phits/phitsinput.cpp \
    $$PROJECT/core/io/input/mcmode.cpp \

}
