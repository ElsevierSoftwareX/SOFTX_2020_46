/*!
 * gxsview version 1.2
 *
 * Copyright (c) 2020 Ohnishi Seiki and National Maritime Research Institute, Japan
 *
 * Released under the GPLv3
 * https://www.gnu.org/licenses/gpl-3.0.txt
 *
 * If you need to distribute with another license,
 * ask ohnishi@m.mpat.go.jp
 */
#ifndef VTKINCLUDE_HPP
#define VTKINCLUDE_HPP

//#include <QVTKWidget.h>
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);
#include <vtkGenericDataArray.h>
//#include <QVTKOpenGLWidget.h>
//#include <QVTKOpenGLStereoWidget.h>
#include "qvtkopenglwrapperwidget.hpp"
#include <vtkActor.h>
#include <vtkActor2DCollection.h>
#include <vtkAxesActor.h>
#include <vtkBooleanOperationPolyDataFilter.h>
#include <vtkContourFilter.h>
#include <vtkDataWriter.h>
#include <vtkFeatureEdges.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkMapper2D.h>
#include <vtkImplicitBoolean.h>
#include <vtkPlaneSource.h>
#include <vtkPLYWriter.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataWriter.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkSampleFunction.h>
#include <vtkSphereSource.h>
#include <vtkSmartPointer.h>
#include <vtkSTLWriter.h>
#include <vtkTextProperty.h>
#include <vtkWindowToImageFilter.h>
#include <vtkXMLPolyDataWriter.h>




#endif // VTKINCLUDE_HPP
