!INPUTDATA_PRI {
INPUTDATA_PRI=1

include ($$PWD/dataline.pri)
include ($$PWD/common/trcard.pri)
include (&&PWD/../../../option/config.pri)

HEADERS *= \
    $$PROJECT/core/io/input/inputdata.hpp \
    $$PROJECT/core/io/input/mcmode.hpp \
    $$PROJECT/core/io/input/phits/phits_metacards.hpp \
    $$PROJECT/core/io/input/phits/phits_metacards.hpp \
    $$PROJECT/core/io/input/mcnp/mcnp_metacards.hpp \
    $$PROJECT/core/io/input/original/original_metacard.hpp \
    $$PROJECT/core/utils/system_utils.hpp \
    $$PROJECT/core/physics/physconstants.hpp \
	$$PROJECT/core/option/config.hpp \

SOURCES *= \
    $$PROJECT/core/io/input/inputdata.cpp \
    $$PROJECT/core/io/input/mcmode.cpp \
    $$PROJECT/core/io/input/phits/phits_metacards.cpp \
    $$PROJECT/core/io/input/phits/phits_metacards.cpp \
    $$PROJECT/core/io/input/mcnp/mcnp_metacards.cpp \
    $$PROJECT/core/io/input/original/original_metacard.cpp \
    $$PROJECT/core/utils/system_utils.cpp \
    $$PROJECT/core/physics/physconstants.cpp \
	$$PROJECT/core/option/config.cpp \
}
