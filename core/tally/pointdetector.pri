!POINTDETECTOR_PRI{
POINTDETECTOR_PRI=1

HEADERS *= \
    $$PROJECT/core/tally/pointdetector.hpp \
    $$PROJECT/core/tally/pktally.hpp \
    $$PROJECT/core/math/nvector.hpp \
    $$PROJECT/core/math/nmatrix.hpp \


SOURCES *= \
    $$PROJECT/core/tally/pointdetector.cpp \
    $$PROJECT/core/tally/pktally.cpp \
    $$PROJECT/core/math/nvector.cpp \

include ($$PWD/../io/input/phits/phitsinput.pri)
include ($$PWD/../io/input/phits/phitsinputsection.pri)
include ($$PWD/../io/input/phits/phitsinputsubsection.pri)
include ($$PWD/../io/input/meshdata.pri)

}
