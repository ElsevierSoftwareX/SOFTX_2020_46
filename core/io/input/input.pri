!INPUT_PRI{
INPUT_PRI=1

HEADERS *= \
    $$PROJECT/core/input/dataline.hpp \
    $$PROJECT/core/input/mcmode.hpp \
    $$PROJECT/core/input/mcnp/mcnp_metacards.hpp \
    $$PROJECT/core/input/mcnp/mcnpinput.hpp \
    $$PROJECT/core/input/mcnp/modecard.hpp \
    $$PROJECT/core/input/original/original_metacard.hpp \
    $$PROJECT/core/input/common/commoncards.hpp \
    $$PROJECT/core/input/common/trcard.hpp \
    $$PROJECT/core/input/common/datacard.hpp \
    $$PROJECT/core/input/phits/phits_metacards.hpp \
    $$PROJECT/core/input/phits/transformsection.hpp \
    $$PROJECT/core/input/phits/phitsinputsection.hpp \
    $$PROJECT/core/input/phits/phitsinput.hpp \
    $$PROJECT/core/input/phits/phitsinputsubsection.hpp \
    $$PROJECT/core/input/meshdata.hpp \
    $$PROJECT/core/input/cellcard.hpp  \
    $$PROJECT/core/input/cellparameter.hpp  \
    $$PROJECT/core/input/filldata.hpp  \
    $$PROJECT/core/input/filling_utils.hpp  \
    $$PROJECT/core/input/baseunitelement.hpp  \
    $$PROJECT/core/input/surfacecard.hpp \
    $$PROJECT/core/input/cardcommon.hpp \
    $$PROJECT/core/input/ijmr.hpp \
    $$PROJECT/core/input/mars/marsinput.hpp \
    $$PROJECT/core/input/qad/qadinput.hpp \
    $$PROJECT/core/input/cg/cgconversionmap.hpp \
    $$PROJECT/core/input/cg/cgbody.hpp \
    $$PROJECT/core/input/cg/cgzone.hpp \


SOURCES *= \
    $$PROJECT/core/input/inputdata.cpp \
    $$PROJECT/core/input/dataline.cpp \
    $$PROJECT/core/input/mcmode.cpp \
    $$PROJECT/core/input/mcnp/mcnp_metacards.cpp \
    $$PROJECT/core/input/mcnp/mcnpinput.cpp \
    $$PROJECT/core/input/mcnp/modecard.cpp \
    $$PROJECT/core/input/original/original_metacard.cpp \
    $$PROJECT/core/input/common/commoncards.cpp \
    $$PROJECT/core/input/common/trcard.cpp \
    $$PROJECT/core/input/common/datacard.cpp \
    $$PROJECT/core/input/phits/phits_metacards.cpp \
    $$PROJECT/core/input/phits/transformsection.cpp \
    $$PROJECT/core/input/phits/phitsinputsection.cpp \
    $$PROJECT/core/input/phits/phitsinputsubsection.cpp \
    $$PROJECT/core/input/phits/phitsinput.cpp \
    $$PROJECT/core/input/meshdata.cpp \
    $$PROJECT/core/input/cellcard.cpp  \
    $$PROJECT/core/input/cellparameter.cpp  \
    $$PROJECT/core/input/filldata.cpp  \
    $$PROJECT/core/input/filling_utils.cpp  \
    $$PROJECT/core/input/baseunitelement.cpp  \
    $$PROJECT/core/input/surfacecard.cpp \
    $$PROJECT/core/input/cardcommon.cpp \
    $$PROJECT/core/input/ijmr.cpp \
    $$PROJECT/core/input/mars/marsinput.cpp \
    $$PROJECT/core/input/qad/qadinput.cpp \
    $$PROJECT/core/input/cg/cgconversionmap.cpp \
    $$PROJECT/core/input/cg/cgbody.cpp \
    $$PROJECT/core/input/cg/cgzone.cpp \

}
