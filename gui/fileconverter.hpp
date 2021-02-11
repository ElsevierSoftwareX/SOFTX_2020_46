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
#ifndef FILECONVERTER_HPP
#define FILECONVERTER_HPP

#include <string>
#include <utility>
#include <vector>
#include <QFileInfo>

#include <vtkBMPWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkPNGWriter.h>

#include "../core/io/fileformat.hpp"

#include <vtkSmartPointer.h>
#include <vtkImageWriter.h>

class vtkPolyData;
class vtkRenderWindow;

namespace ff {


void exportFile(FORMAT3D format, const std::string &fileBaseName, vtkPolyData *polyData, double factor, bool reverse = false);

std::pair<QFileInfo, ff::FORMAT2D> getFileAndFormat2D(std::vector<ff::FORMAT2D> excludingFormats =std::vector<ff::FORMAT2D>());

// formatから適当にwriterを作成して返す。
vtkSmartPointer<vtkImageWriter> getVtk2DWriter(ff::FORMAT2D format);

void exportRenderWindowToRasterGraphics(vtkRenderWindow *rwindow);


}
#endif // FILEFORMAT_HPP
