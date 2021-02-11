!PHYSICS_PRI{
PHYSICS_PRI = 1

SOURCES *= \
    $$PROJECT/core/physics/physconstants.cpp \
    $$PROJECT/core/physics/particleexception.cpp \

HEADERS *= \
    $$PROJECT/core/physics/physconstants.hpp \
    $$PROJECT/core/physics/particleexception.hpp \


include ($$PWD/particle/particle.pri)

}
