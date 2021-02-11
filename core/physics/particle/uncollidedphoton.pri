!UNCOLLIDEDPHOTON_PRI{
UNCOLLIDEDPHOTON_PRI=1

include ($$PWD/tracingparticle.pri)

HEADERS *= \
    $$PROJECT/core/physics/particle/uncollidedphoton.hpp \
    $$PROJECT/core/math/nvector.hpp \
    $$PROJECT/core/physics/physconstants.hpp \

SOURCES *= \
    $$PROJECT/core/physics/particle/uncollidedphoton.cpp \
    $$PROJECT/core/math/nvector.cpp \
    $$PROJECT/core/physics/physconstants.cpp \

}
