
# VTK later than 9 appends "d" in debug build.
CONFIG(debug,debug|release) {win32{ D=d }}
LIBS += \
  -lvtkChartsCore-$${VTK_MAJOR_VER}$${D} \
  -lvtkCommonColor-$${VTK_MAJOR_VER}$${D} \
  -lvtkCommonComputationalGeometry-$${VTK_MAJOR_VER}$${D} \
  -lvtkCommonCore-$${VTK_MAJOR_VER}$${D} \
  -lvtkCommonDataModel-$${VTK_MAJOR_VER}$${D} \
  -lvtkCommonExecutionModel-$${VTK_MAJOR_VER}$${D} \
  -lvtkCommonMath-$${VTK_MAJOR_VER}$${D} \
  -lvtkCommonMisc-$${VTK_MAJOR_VER}$${D} \
  -lvtkCommonTransforms-$${VTK_MAJOR_VER}$${D} \
  -lvtkCommonSystem-$${VTK_MAJOR_VER}$${D} \
  -lvtkDICOMParser-$${VTK_MAJOR_VER}$${D} \
  -lvtkFiltersCore-$${VTK_MAJOR_VER}$${D} \
  -lvtkFiltersExtraction-$${VTK_MAJOR_VER}$${D} \
  -lvtkFiltersGeneral-$${VTK_MAJOR_VER}$${D} \
  -lvtkFiltersGeometry-$${VTK_MAJOR_VER}$${D} \
  -lvtkFiltersHybrid-$${VTK_MAJOR_VER}$${D} \
  -lvtkFiltersModeling-$${VTK_MAJOR_VER}$${D} \
  -lvtkFiltersStatistics-$${VTK_MAJOR_VER}$${D} \
  -lvtkFiltersSources-$${VTK_MAJOR_VER}$${D} \
  -lvtkGUISupportQt-$${VTK_MAJOR_VER}$${D} \
  -lvtkImagingColor-$${VTK_MAJOR_VER}$${D} \
  -lvtkImagingCore-$${VTK_MAJOR_VER}$${D} \
  -lvtkImagingFourier-$${VTK_MAJOR_VER}$${D} \
  -lvtkImagingGeneral-$${VTK_MAJOR_VER}$${D} \
  -lvtkImagingHybrid-$${VTK_MAJOR_VER}$${D} \
  -lvtkImagingSources-$${VTK_MAJOR_VER}$${D} \
  -lvtkInfovisCore-$${VTK_MAJOR_VER}$${D} \
  -lvtkInteractionStyle-$${VTK_MAJOR_VER}$${D} \
  -lvtkInteractionWidgets-$${VTK_MAJOR_VER}$${D} \
  -lvtkIOCore-$${VTK_MAJOR_VER}$${D} \
  -lvtkIOExport-$${VTK_MAJOR_VER}$${D} \
  -lvtkIOExportGL2PS-$${VTK_MAJOR_VER}$${D} \
  -lvtkIOImage-$${VTK_MAJOR_VER}$${D} \
  -lvtkIOGeometry-$${VTK_MAJOR_VER}$${D} \
  -lvtkIOLegacy-$${VTK_MAJOR_VER}$${D} \
  -lvtkIOPLY-$${VTK_MAJOR_VER}$${D} \
  -lvtkIOXML-$${VTK_MAJOR_VER}$${D} \
  -lvtkIOXMLParser-$${VTK_MAJOR_VER}$${D} \
  -lvtkParallelCore-$${VTK_MAJOR_VER}$${D} \
  -lvtkParallelDIY-$${VTK_MAJOR_VER}$${D} \
  -lvtkRenderingAnnotation-$${VTK_MAJOR_VER}$${D} \
  -lvtkRenderingContext2D-$${VTK_MAJOR_VER}$${D} \
  -lvtkRenderingContextOpenGL2-$${VTK_MAJOR_VER}$${D} \
  -lvtkRenderingCore-$${VTK_MAJOR_VER}$${D} \
  -lvtkRenderingFreeType-$${VTK_MAJOR_VER}$${D} \
  -lvtkRenderingGL2PSOpenGL2-$${VTK_MAJOR_VER}$${D} \
  -lvtkRenderingSceneGraph-$${VTK_MAJOR_VER}$${D} \
  -lvtkRenderingOpenGL2-$${VTK_MAJOR_VER}$${D} \
  -lvtkRenderingUI-$${VTK_MAJOR_VER}$${D} \
  -lvtkRenderingVolume-$${VTK_MAJOR_VER}$${D} \
  -lvtkRenderingVtkJS-$${VTK_MAJOR_VER}$${D} \
  -lvtkdoubleconversion-$${VTK_MAJOR_VER}$${D} \
  -lvtkexpat-$${VTK_MAJOR_VER}$${D} \
  -lvtkfreetype-$${VTK_MAJOR_VER}$${D} \
  -lvtkglew-$${VTK_MAJOR_VER}$${D} \
  -lvtkgl2ps-$${VTK_MAJOR_VER}$${D} \
  -lvtkjpeg-$${VTK_MAJOR_VER}$${D} \
  -lvtkjsoncpp-$${VTK_MAJOR_VER}$${D} \
  -lvtkloguru-$${VTK_MAJOR_VER}$${D} \
  -lvtklz4-$${VTK_MAJOR_VER}$${D} \
  -lvtklzma-$${VTK_MAJOR_VER}$${D} \
  -lvtkmetaio-$${VTK_MAJOR_VER}$${D} \
  -lvtkpng-$${VTK_MAJOR_VER}$${D} \
  -lvtkpugixml-$${VTK_MAJOR_VER}$${D} \
  -lvtksys-$${VTK_MAJOR_VER}$${D} \
  -lvtktiff-$${VTK_MAJOR_VER}$${D} \
  -lvtkzlib-$${VTK_MAJOR_VER}$${D} \
  -lvtkViewsContext2D-$${VTK_MAJOR_VER}$${D} \
  -lvtkViewsCore-$${VTK_MAJOR_VER}$${D} \
