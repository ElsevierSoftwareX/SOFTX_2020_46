

HEADERS *= \
    $$PROJECT/core/source/abstractdistribution.hpp \
    $$PROJECT/core/source/multigroupdistribution.hpp \
    $$PROJECT/core/source/proportionaldistribution.hpp \
    $$PROJECT/core/source/discretedistribution.hpp \






SOURCES *= \
    $$PROJECT/core/source/abstractdistribution.cpp \
    $$PROJECT/core/source/multigroupdistribution.cpp \
    $$PROJECT/core/source/proportionaldistribution.cpp \
    $$PROJECT/core/source/discretedistribution.cpp \


include($$PWD/phits/phitssource.pri)
