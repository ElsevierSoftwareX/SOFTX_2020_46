!PARTICLE_PRI{
PARTICLE_PRI=1

include ($$PWD/../../geometry/cell/cell.pri)

HEADERS *= \
    $$PROJECT/core/physics/particle/particle.hpp \
    $$PROJECT/core/physics/particleexception.hpp \
#    $$PROJECT/core/physics/particle/tracingparticle.hpp \
#    $$PROJECT/core/physics/particle/uncollidedphoton.hpp \



SOURCES *= \
    $$PROJECT/core/physics/particle/particle.cpp \
    $$PROJECT/core/physics/particleexception.cpp \
#    $$PROJECT/core/physics/particle/tracingparticle.cpp \
#    $$PROJECT/core/physics/particle/uncollidedphoton.cpp \

}
