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
#ifndef CELLOBJECT_HPP
#define CELLOBJECT_HPP


#include "vtkSmartPointer.h"
#include "vtkActor.h"
#include "vtkImplicitFunction.h"
#include "../../core/geometry/cell/boundingbox.hpp"

struct CellObject
{
public:
	// コンストラクタ
	CellObject(const std::string &cellName,
			   const geom::BoundingBox &bb,
			   const vtkSmartPointer<vtkImplicitFunction> &ifunc,
			   const vtkSmartPointer<vtkActor> &actor, int nRefPts, int nRealPts);
	CellObject(std::string &&cellName,
			   geom::BoundingBox &&bb,
			   vtkSmartPointer<vtkImplicitFunction> &&ifunc,
			   vtkSmartPointer<vtkActor> &&actor,
			   int &&nRefPts, int &&nRealPts);


	std::string cellName_;
	geom::BoundingBox bb_;
	vtkSmartPointer<vtkImplicitFunction> implicitFunction_;
	vtkSmartPointer<vtkActor>actor_;
	int numRefPoints_;
	int numRealPoints_;
};


#endif // CELLOBJECT_HPP
