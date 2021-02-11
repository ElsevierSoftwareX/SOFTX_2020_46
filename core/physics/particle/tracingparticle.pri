!TRACINGPARTICLE_PRI{
TRACINGPARTICLE_PRI=1

include ($$PWD/../../geometry/cell/cell.pri)
include ($$PWD/particle.pri)

HEADERS *= \
    $$PROJECT/core/physics/particle/tracingparticle.hpp

SOURCES *= \
    $$PROJECT/core/physics/particle/tracingparticle.cpp


}
