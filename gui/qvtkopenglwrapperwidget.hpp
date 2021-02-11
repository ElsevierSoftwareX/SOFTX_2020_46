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
#ifndef QVTKOPENGLWRAPPERWIDGET_H
#define QVTKOPENGLWRAPPERWIDGET_H

//#include <QVTKOpenGLWidget.h>
#include <vtkVersion.h>


#if VTK_MAJOR_VERSION >= 9
#include <QVTKOpenGLStereoWidget.h>
typedef QVTKOpenGLStereoWidget QVTKOpenGLWrapperWidget;

#else
#include <QVTKOpenGLWidget.h>
typedef QVTKOpenGLWidget QVTKOpenGLWrapperWidget;
#endif



#endif  // end include gurad.
