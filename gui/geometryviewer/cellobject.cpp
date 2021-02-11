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
#include "cellobject.hpp"

#include "../../core/geometry/cell/boundingbox.hpp"

CellObject::CellObject(const std::string &cellName,
					   const geom::BoundingBox &bb,
					   const vtkSmartPointer<vtkImplicitFunction> &ifunc,
					   const vtkSmartPointer<vtkActor> &actor,
					   int nRefPts, int nRealPts)
	: cellName_(cellName), bb_(bb), implicitFunction_(ifunc), actor_(actor),
	  numRefPoints_(nRefPts), numRealPoints_(nRealPts)
{;}

CellObject::CellObject(std::string &&cellName,
					   geom::BoundingBox &&bb,
					   vtkSmartPointer<vtkImplicitFunction> &&ifunc,
					   vtkSmartPointer<vtkActor> &&actor,
					   int &&nRefPts, int &&nRealPts)
	: cellName_(cellName), bb_(bb), implicitFunction_(ifunc), actor_(actor),
	  numRefPoints_(nRefPts), numRealPoints_(nRealPts)
{;}
