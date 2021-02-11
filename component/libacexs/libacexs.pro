

TEMPLATE = subdirs

CONFIG += ordered


SUBDIRS += \
    libsrc \
    example \

win32{
    mingw{
        # MinGW
        COMPILER=mingw
    } else {
        # VC++
        COMPILER=msvc
        QMAKE_CXXFLAGS += /utf-8 /wd4503 /wd4267
    }
}


example.depends = libsrc

