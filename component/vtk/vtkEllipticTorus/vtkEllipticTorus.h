/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkEllipticTorus.h,v $

  Author: Seiki Ohnihsi,
  
 This works is based on Sebastien Jourdain's vtkTorus
  
=========================================================================*/
// .NAME vtkEllipticTorus - implicit function for a elliptic torus
// .SECTION Description
// vtkEllipticTorus computes the implicit function and function gradient for
// a elliptic torus.
// vtkEllipticTorus is a concrete implementation of vtkImplicitFunction.
// The torus, ellipse section rotate around the z-axis.
// (Use the superclass' vtkImplicitFunction transformation matrix if necessary
// to reposition.) 

#ifndef __vtkEllipticTorus_h
#define __vtkEllipticTorus_h

#include "vtkCommonDataModelModule.h" // added by sohnishi for New()
#include "vtkImplicitFunction.h"

// for making library
//class VTKCOMMONDATAMODEL_EXPORT vtkEllipticTorus : public vtkImplicitFunction
// for including directly
class vtkEllipticTorus : public vtkImplicitFunction
{
public:
  // Description
  // Construct torus.
  static vtkEllipticTorus *New();

  vtkTypeMacro(vtkEllipticTorus,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  
  // Description
  // Evaluate elliptic torus equation.
  double EvaluateFunction(double x[3]) override;
  double EvaluateFunction(double x, double y, double z) override
  {
	  return this->vtkImplicitFunction::EvaluateFunction(x, y, z);
  }

  // Description
  // Evaluate cone normal.
  void EvaluateGradient(double x[3], double g[3]) override;

  // Description:
  // Set/Get the major radius and vertical, horizontal minor radius.
  vtkSetMacro(MinorVRadius,double);
  vtkGetMacro(MinorVRadius,double);
  vtkSetMacro(MinorHRadius,double);
  vtkGetMacro(MinorHRadius,double);
  vtkSetMacro(MajorRadius,double);
  vtkGetMacro(MajorRadius,double);

protected:
  vtkEllipticTorus();
  ~vtkEllipticTorus() {}

  double MinorVRadius;
  double MinorHRadius;
  double MajorRadius;

private:
  vtkEllipticTorus(const vtkEllipticTorus&);  // Not implemented.
  void operator=(const vtkEllipticTorus&);  // Not implemented.
};

#endif


