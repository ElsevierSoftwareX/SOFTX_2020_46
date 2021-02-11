!PHITSSOURCE_PRI{
PHITSSOURCE_PRI = 1

include ($$PWD/../abstractdistribution.pri)
include ($$PWD/../../io/input/phits/phitsinput.pri)
include ($$PWD/../../physics/particle/uncollidedphoton.pri)

HEADERS *= \
    $$PROJECT/core/source/phits/phitssource.hpp \
    $$PROJECT/core/source/phits/phitscylindersource.hpp \
    $$PROJECT/core/source/phits/phitsdistribution.hpp \
    $$PROJECT/core/source/phits/phitsangulardistribution.hpp \
    $$PROJECT/core/source/phits/phitsenergydistribution.hpp \
    $$PROJECT/core/source/phits/phitssourcefunction.hpp \
    $$PROJECT/core/source/phits/phitssourcefunction.hpp \



SOURCES *= \
    $$PROJECT/core/source/phits/phitssource.cpp \
    $$PROJECT/core/source/phits/phitscylindersource.cpp \
    $$PROJECT/core/source/phits/phitssourcefunction.cpp \
    $$PROJECT/core/source/phits/phitsdistribution.cpp \
    $$PROJECT/core/source/phits/phitsangulardistribution.cpp \
    $$PROJECT/core/source/phits/phitsenergydistribution.cpp \
    $$PROJECT/core/source/phits/phitssourcefunction.cpp \


}
