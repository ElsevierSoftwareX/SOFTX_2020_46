!CUSTOMTERMINAL_PRI{
CUSTOMTERMINAL_PRI = 1


#include ($$PWD/../image/bitmapimage.pri)

HEADERS *= \
    $$PROJECT/core/terminal/customterminal.hpp \
    $$PROJECT/core/utils/string_utils.hpp \




SOURCES *= \
    $$PROJECT/core/terminal/customterminal.cpp \
    $$PROJECT/core/utils/string_utils.cpp \

}
