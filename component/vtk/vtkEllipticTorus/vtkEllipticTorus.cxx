/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkEllipticTorus.cxx,v $

  Author: Seiki Ohnishi, 
  based on Sebastien Jourdain's vtkTorus
=========================================================================*/
#include "vtkEllipticTorus.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkEllipticTorus);

// Construct torus with R=5, a=1, b=1
vtkEllipticTorus::vtkEllipticTorus()
{
  this->MajorRadius = 5.0;
  this->MinorVRadius = 1.0;
  this->MinorHRadius = 1.0;
}

double vtkEllipticTorus::EvaluateFunction(double x[3])
{
	double R2 = MajorRadius*MajorRadius,
		a2 = MinorVRadius*MinorVRadius,
		b2 = MinorHRadius*MinorHRadius;
	double x2 = x[0]*x[0], y2 = x[1]*x[1], z2 = x[2]*x[2];
	double term1 = b2*z2 + a2*(x2 + y2 + R2 - b2);
	term1 *= term1;
	return term1 - 4*a2*a2*R2*(x2 + y2);
}

// Evaluate torus normal.
void vtkEllipticTorus::EvaluateGradient(double x[3], double g[3])
{
	// TODO This calculation is very rough. 
	double normXY = sqrt(x[0] * x[0] + x[1] * x[1]);
	if(normXY == 0){
		g[0] = 2.0 * this->MajorRadius;
		g[1] = 0;
		g[2] = 2.0 * x[2];
	} else {
		double factor = 1 - (this->MajorRadius/normXY);
		g[0] = 2.0 * x[0] * factor;
		g[1] = 2.0 * x[1] * factor;
		g[2] = 2.0 * x[2];
	}
}

void vtkEllipticTorus::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Torus: R=" << this->MajorRadius << " a="
	 << this->MinorVRadius << " b=" << this->MinorHRadius << "\n";
}
