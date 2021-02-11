INCLUDEPATH += $$LIBACEXS_SRCDIR

SOURCES *= \
    $$LIBACEXS_SRCDIR/acefile.cpp\
    $$LIBACEXS_SRCDIR/neutrontransportfile.cpp\
    $$LIBACEXS_SRCDIR/fissionneutrondata.cpp\
    $$LIBACEXS_SRCDIR/neutrondosimetryfile.cpp\
    $$LIBACEXS_SRCDIR/mt.cpp \
    $$LIBACEXS_SRCDIR/CrossSection.cpp \
    $$LIBACEXS_SRCDIR/utils/string_util.cpp \
    $$LIBACEXS_SRCDIR/utils/utils_conv.cpp \
    $$LIBACEXS_SRCDIR/photoatomicfile.cpp \
    $$LIBACEXS_SRCDIR/xsdir.cpp


HEADERS *= \
    $$LIBACEXS_SRCDIR/acefile.hpp\
    $$LIBACEXS_SRCDIR/aceutils.hpp\
    $$LIBACEXS_SRCDIR/fissionneutrondata.hpp\
    $$LIBACEXS_SRCDIR/neutrontransportfile.hpp\
    $$LIBACEXS_SRCDIR/neutrondosimetryfile.hpp\
    $$LIBACEXS_SRCDIR/mt.hpp \
    $$LIBACEXS_SRCDIR/utils/string_util.hpp \
    $$LIBACEXS_SRCDIR/utils/utils_conv.hpp \
    $$LIBACEXS_SRCDIR/utils/utils_vector.hpp \
    $$LIBACEXS_SRCDIR/utils/utils_numeric.hpp \
    $$LIBACEXS_SRCDIR/photoatomicfile.hpp \
    $$LIBACEXS_SRCDIR/xsdir.hpp
