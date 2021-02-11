SOURCES += \
    $$PROJECT/core/geometry/surface/sphere.cpp \
    $$PROJECT/core/geometry/surface/surface.cpp \
    $$PROJECT/core/geometry/surface/plane.cpp \
    $$PROJECT/core/geometry/surface/cylinder.cpp \
    $$PROJECT/core/geometry/cell/cell.cpp \
    $$PROJECT/core/io/input/dataline.cpp \
    $$PROJECT/core/io/input/mcnp/mcnp_metacards.cpp \
    $$PROJECT/core/io/input/common/trcard.cpp \
    $$PROJECT/core/io/input/common/commoncards.cpp \
    $$PROJECT/core/io/input/inputdata.cpp \
    $$PROJECT/core/io/input/phits/phits_metacards.cpp \
    $$PROJECT/core/io/input/mcmode.cpp \
    $$PROJECT/core/io/input/phits/transformsection.cpp \
    $$PROJECT/core/io/input/phits/phitsinputsubsection.cpp \
    $$PROJECT/core/io/input/meshdata.cpp \
    $$PROJECT/core/io/input/surfacecard.cpp \
    $$PROJECT/core/io/input/cellcard.cpp \
    $$PROJECT/core/io/input/filldata.cpp \
    $$PROJECT/core/io/input/cardcommon.cpp \
    $$PROJECT/core/io/input/mcnp/mcnpinput.cpp \
    $$PROJECT/core/io/input/common/datacard.cpp \
    $$PROJECT/core/io/input/mcnp/modecard.cpp \
    $$PROJECT/core/io/input/phits/phitsinput.cpp \
    $$PROJECT/core/io/input/ijmr.cpp \
    $$PROJECT/core/io/input/cellparameter.cpp \
    $$PROJECT/core/io/input/qad/qadinput.cpp \
    $$PROJECT/core/io/input/cg/cgbody.cpp \
    $$PROJECT/core/io/input/cg/cgzone.cpp \
    $$PROJECT/core/io/input/phits/phitsinputsection.cpp \
    $$PROJECT/core/io/input/original/original_metacard.cpp \
    $$PROJECT/core/physics/particle/particle.cpp \
    $$PROJECT/core/physics/particle/tracingparticle.cpp \
    $$PROJECT/core/image/xpmcolors.cpp \
    $$PROJECT/core/image/tracingraydata.cpp \
    $$PROJECT/core/geometry/geometry.cpp \
    $$PROJECT/core/terminal/interactiveplotter.cpp \
    $$PROJECT/core/option/config.cpp \
    $$PROJECT/core/terminal/interactiveplotter.command.cpp \
    $$PROJECT/core/terminal/customterminal.cpp \
    $$PROJECT/core/material/material.cpp \
    $$PROJECT/core/material/nuclide.cpp \
    $$PROJECT/core/source/phits/phitssource.cpp \
    $$PROJECT/core/source/phits/phitscylindersource.cpp \
    $$PROJECT/core/math/nvector.cpp \
    $$PROJECT/core/source/abstractdistribution.cpp \
    $$PROJECT/core/source/discretedistribution.cpp \
    $$PROJECT/core/utils/message.cpp \
    $$PROJECT/core/source/multigroupdistribution.cpp \
    $$PROJECT/core/source/proportionaldistribution.cpp \
    $$PROJECT/core/utils/container_utils.cpp \
    $$PROJECT/core/physics/physconstants.cpp \
    $$PROJECT/core/tally/pointdetector.cpp \
    $$PROJECT/core/simulation.cpp \
    $$PROJECT/core/utils/matrix_utils.cpp \
    $$PROJECT/core/image/bitmapimage.cpp \
    $$PROJECT/core/image/pixelarray.cpp \
    $$PROJECT/core/image/color.cpp \
    $$PROJECT/core/utils/string_utils.cpp \
    $$PROJECT/core/geometry/macro/box.cpp \
    $$PROJECT/core/geometry/macro/rpp.cpp \
    $$PROJECT/core/geometry/macro/macro.cpp \
    $$PROJECT/core/geometry/macro/sph.cpp \
    $$PROJECT/core/geometry/macro/rcc.cpp \
    $$PROJECT/core/geometry/macro/rhp.cpp \
    $$PROJECT/core/geometry/surface/surfacemap.cpp \
    $$PROJECT/core/geometry/surfacecreator.cpp \
    $$PROJECT/core/geometry/cellcreator.cpp \
    $$PROJECT/core/formula/fortran/fortnode.cpp \
    $$PROJECT/core/formula/formula_utils.cpp \
    $$PROJECT/core/geometry/surface/cone.cpp \
    $$PROJECT/core/geometry/macro/trc.cpp \
    $$PROJECT/core/geometry/surface/quadric.cpp \
    $$PROJECT/core/geometry/surface/torus.cpp \
    $$PROJECT/core/geometry/macro/ell.cpp \
    $$PROJECT/core/geometry/macro/wed.cpp \
    $$PROJECT/core/tally/pktally.cpp \
    $$PROJECT/core/buildup/buildupfactor.cpp \
    $$PROJECT/core/physics/particle/uncollidedphoton.cpp \
    $$PROJECT/core/geometry/cell/boundingbox.cpp \
    $$PROJECT/core/source/phits/phitsenergydistribution.cpp \
    $$PROJECT/core/source/phits/phitsangulardistribution.cpp \
    $$PROJECT/core/source/phits/phitsdistribution.cpp \
    $$PROJECT/core/source/phits/phitssourcefunction.cpp \
    $$PROJECT/core/geometry/surface/triangle.cpp \
    $$PROJECT/core/geometry/surface/polyhedron.cpp \
    $$PROJECT/core/material/nmtc.cpp \
    $$PROJECT/core/utils/system_utils.cpp \
    $$PROJECT/core/utils/numeric_utils.cpp \
    $$PROJECT/core/geometry/macro/arb.cpp \
    $$PROJECT/core/geometry/latticecreator.cpp \
    $$PROJECT/core/geometry/tetracreator.cpp \
    $$PROJECT/core/geometry/tetrahedron.cpp \
    $$PROJECT/core/fielddata/fieldcolordata.cpp \
    $$PROJECT/core/material/materials.cpp \
    $$PROJECT/core/utils/time_utils.cpp \
    $$PROJECT/core/fielddata/xyzmeshtallydata.cpp \
    $$PROJECT/core/utils/progress_utils.cpp \
    $$PROJECT/core/geometry/tracingworker.cpp \
    $$PROJECT/core/image/pixelmergingworker.cpp \
    $$PROJECT/core/physics/particleexception.cpp \
    $$PROJECT/core/image/cellcolorpalette.cpp \
    $$PROJECT/core/image/matnamecolor.cpp \
    $$PROJECT/core/utils/json_utils.cpp \
    $$PROJECT/core/geometry/cell/bb_utils.cpp \
    $$PROJECT/core/geometry/cell_utils.cpp \
    $$PROJECT/core/geometry/macro/qua.cpp \
    $$PROJECT/core/geometry/macro/rec.cpp \
    $$PROJECT/core/geometry/macro/tor.cpp \
    $$PROJECT/core/geometry/surf_utils.cpp \
    $$PROJECT/core/geometry/surface/quadric.bs.cpp \
    $$PROJECT/core/geometry/surface/surface_utils.cpp \
    $$PROJECT/core/io/input/baseunitelement.cpp \
    $$PROJECT/core/io/input/cg/cgconversionmap.cpp \
    $$PROJECT/core/io/input/filling_utils.cpp \
    $$PROJECT/core/io/input/mars/marsinput.cpp \
    $$PROJECT/core/io/fileformat.cpp \






HEADERS += \
    $$PROJECT/core/geometry/surface/sphere.hpp \
    $$PROJECT/core/geometry/surface/surface.hpp \
    $$PROJECT/core/utils/message.hpp \
    $$PROJECT/core/geometry/surface/plane.hpp \
    $$PROJECT/core/geometry/surface/cylinder.hpp \
    $$PROJECT/core/geometry/cell/cell.hpp \
    $$PROJECT/core/io/input/dataline.hpp \
    $$PROJECT/core/io/input/mcnp/mcnp_metacards.hpp \
    $$PROJECT/core/io/input/common/trcard.hpp \
    $$PROJECT/core/io/input/common/commoncards.hpp \
    $$PROJECT/core/io/input/inputdata.hpp \
    $$PROJECT/core/io/input/phits/phits_metacards.hpp \
    $$PROJECT/core/io/input/mcmode.hpp \
    $$PROJECT/core/io/input/phits/transformsection.hpp \
    $$PROJECT/core/io/input/original/original_metacard.hpp \
    $$PROJECT/core/io/input/phits/phitsinputsection.hpp \
    $$PROJECT/core/io/input/phits/phitsinputsubsection.hpp \
    $$PROJECT/core/io/input/meshdata.hpp \
    $$PROJECT/core/io/input/cardcommon.hpp \
    $$PROJECT/core/io/input/surfacecard.hpp \
    $$PROJECT/core/io/input/cellcard.hpp \
    $$PROJECT/core/io/input/filldata.hpp \
    $$PROJECT/core/io/input/cardcommon.hpp \
    $$PROJECT/core/io/input/mcnp/mcnpinput.hpp \
    $$PROJECT/core/io/input/common/datacard.hpp \
    $$PROJECT/core/io/input/mcnp/modecard.hpp \
    $$PROJECT/core/io/input/phits/phitsinput.hpp \
    $$PROJECT/core/io/input/ijmr.hpp \
    $$PROJECT/core/io/input/cellparameter.hpp \
    $$PROJECT/core/io/input/qad/qadinput.hpp \
    $$PROJECT/core/io/input/cg/cgbody.hpp \
    $$PROJECT/core/io/input/cg/cgzone.hpp \
    $$PROJECT/core/physics/particle/particle.hpp \
    $$PROJECT/core/math/nmatrix.hpp \
    $$PROJECT/core/math/nmatrix_inl.hpp \
    $$PROJECT/core/physics/particle/tracingparticle.hpp \
    $$PROJECT/core/math/nvector.hpp \
    $$PROJECT/core/math/nvector_inl.hpp \
    $$PROJECT/core/image/xpmcolors.hpp \
    $$PROJECT/core/image/tracingraydata.hpp \
    $$PROJECT/core/geometry/geometry.hpp \
    $$PROJECT/core/utils/time_utils.hpp \
    $$PROJECT/core/utils/stream_utils.hpp \
    $$PROJECT/core/utils/utils.hpp \
    $$PROJECT/core/terminal/interactiveplotter.hpp \
    $$PROJECT/core/option/config.hpp \
    $$PROJECT/core/material/material.hpp \
    $$PROJECT/core/terminal/customterminal.hpp \
    $$PROJECT/core/material/nuclide.hpp \
    $$PROJECT/core/source/phits/phitssource.hpp \
    $$PROJECT/core/source/phits/phitscylindersource.hpp \
    $$PROJECT/core/math/constants.hpp \
    $$PROJECT/core/source/abstractdistribution.hpp \
    $$PROJECT/core/source/discretedistribution.hpp \
    $$PROJECT/core/source/multigroupdistribution.hpp \
    $$PROJECT/core/source/proportionaldistribution.hpp \
    $$PROJECT/core/utils/container_utils.hpp \
    $$PROJECT/core/physics/physconstants.hpp \
    $$PROJECT/core/tally/pointdetector.hpp \
    $$PROJECT/core/simulation.hpp \
    $$PROJECT/core/utils/matrix_utils.hpp \
    $$PROJECT/core/image/bitmapimage.hpp \
    $$PROJECT/core/image/pixelarray.hpp \
    $$PROJECT/core/utils/type_utils.hpp \
    $$PROJECT/core/image/color.hpp \
    $$PROJECT/core/utils/string_utils.hpp \
    $$PROJECT/core/geometry/macro/box.hpp \
    $$PROJECT/core/geometry/macro/rpp.hpp \
    $$PROJECT/core/geometry/macro/macro.hpp \
    $$PROJECT/core/geometry/macro/sph.hpp \
    $$PROJECT/core/geometry/macro/rcc.hpp \
    $$PROJECT/core/geometry/macro/rhp.hpp \
    $$PROJECT/core/geometry/surface/surfacemap.hpp \
    $$PROJECT/core/geometry/cellcreator.hpp \
    $$PROJECT/core/geometry/surfacecreator.hpp \
    $$PROJECT/core/formula/fortran/fortnode.hpp \
    $$PROJECT/core/formula/formula_utils.hpp \
    $$PROJECT/core/geometry/surface/cone.hpp \
    $$PROJECT/core/geometry/macro/trc.hpp \
    $$PROJECT/core/geometry/surface/quadric.hpp \
    $$PROJECT/core/geometry/surface/torus.hpp \
    $$PROJECT/core/math/equationsolver.hpp \
    $$PROJECT/core/geometry/macro/ell.hpp \
    $$PROJECT/core/geometry/macro/wed.hpp \
    $$PROJECT/core/tally/pktally.hpp \
    $$PROJECT/core/buildup/buildupfactor.hpp \
    $$PROJECT/core/geometry/macro/xyz.hpp \
    $$PROJECT/core/physics/particle/uncollidedphoton.hpp \
    $$PROJECT/core/geometry/surface/vtksurfaceheaders.hpp \
    $$PROJECT/core/geometry/cell/boundingbox.hpp \
    $$PROJECT/core/source/phits/phitsenergydistribution.hpp \
    $$PROJECT/core/source/phits/phitsangulardistribution.hpp \
    $$PROJECT/core/source/phits/phitsdistribution.hpp \
    $$PROJECT/core/source/phits/phitssourcefunction.hpp \
    $$PROJECT/core/geometry/surface/triangle.hpp \
    $$PROJECT/core/geometry/surface/polyhedron.hpp \
    $$PROJECT/core/material/nmtc.hpp \
    $$PROJECT/core/utils/system_utils.hpp \
    $$PROJECT/core/utils/numeric_utils.hpp \
    $$PROJECT/core/geometry/macro/arb.hpp \
    $$PROJECT/core/geometry/latticecreator.hpp \
    $$PROJECT/core/geometry/tetracreator.hpp \
    $$PROJECT/core/geometry/tetrahedron.hpp \
    $$PROJECT/core/fielddata/fieldcolordata.hpp \
    $$PROJECT/core/material/materials.hpp \
    $$PROJECT/core/fielddata/xyzmeshtallydata.hpp \
    $$PROJECT/core/formula/logical/lpolynomial.hpp \
    $$PROJECT/core/utils/progress_utils.hpp \
    $$PROJECT/core/geometry/tracingworker.hpp \
    $$PROJECT/core/image/pixelmergingworker.hpp \
    $$PROJECT/core/physics/particleexception.hpp \
    $$PROJECT/core/image/cellcolorpalette.hpp \
    $$PROJECT/core/image/matnamecolor.hpp \
    $$PROJECT/core/utils/json_utils.hpp \
   $$PROJECT/core/geometry/cell/bb_utils.hpp \
   $$PROJECT/core/geometry/cell_utils.hpp \
   $$PROJECT/core/geometry/macro/qua.hpp \
   $$PROJECT/core/geometry/macro/rec.hpp \
   $$PROJECT/core/geometry/macro/tor.hpp \
   $$PROJECT/core/geometry/surf_utils.hpp \
   $$PROJECT/core/geometry/surface/quadric.bs.hpp \
   $$PROJECT/core/geometry/surface/surface_utils.hpp \
   $$PROJECT/core/io/input/baseunitelement.hpp \
   $$PROJECT/core/io/input/cg/cgconversionmap.hpp \
   $$PROJECT/core/io/input/filling_utils.hpp \
   $$PROJECT/core/io/input/mars/marsinput.hpp \
   $$PROJECT/core/io/fileformat.hpp \
   $$PROJECT/core/utils/workerinterface.hpp \

