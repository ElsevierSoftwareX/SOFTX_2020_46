!TERMINAL_PRI {
TERMINAL_PRI = 1

#include ($$PWD/../image/bitmapimage.pri)

HEADERS *= \
    $$PROJECT/core/terminal/customterminal.hpp \
#    $$PROJECT/core/terminal/interactiveplotter.hpp \



SOURCES *= \
    $$PROJECT/core/terminal/customterminal.cpp \
#    $$PROJECT/core/terminal/interactiveplotter.cpp \
#    $$PROJECT/core/utils/string_utils.cpp \
#    $$PROJECT/core/simulation.cpp

}
