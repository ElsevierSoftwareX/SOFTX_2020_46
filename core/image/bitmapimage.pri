!BITMAPIMAGE_PRI{
BITMAPIMAGE_PRI=1

include ($$PWD/color.pri)

HEADERS *= \
    $$PROJECT/core/image/bitmapimage.hpp \
    $$PROJECT/core/image/pixelarray.hpp \
    $$PROJECT/core/image/xpmcolors.hpp \
    $$PROJECT/core/math/nvector.hpp \
    $$PROJECT/core/utils/string_utils.hpp \
    $$PROJECT/core/utils/numeric_utils.hpp \



SOURCES *= \
    $$PROJECT/core/image/bitmapimage.cpp \
    $$PROJECT/core/image/pixelarray.cpp \
    $$PROJECT/core/image/xpmcolors.cpp \
    $$PROJECT/core/math/nvector.cpp \
    $$PROJECT/core/utils/string_utils.cpp \
    $$PROJECT/core/utils/numeric_utils.cpp \

}
