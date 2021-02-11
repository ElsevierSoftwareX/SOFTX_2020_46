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
#ifndef CYLINDER_HPP
#define CYLINDER_HPP

#include "surface.hpp"

#include <memory>
#include <string>

#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"

#include "vtksurfaceheaders.hpp"


namespace geom {

class Cylinder: public Surface
{
public:
	enum TYPE{CX, CXO, CY, CYO, CZ, CZO, CA};
	Cylinder(const std::string name, const math::Point &point, const math::Vector<3> &direction, double rad);

	// virtualの実装
	std::string toInputString() const override;
	std::unique_ptr<Surface> createReverse() const override;
	bool isForward(const math::Point &p) const override;
	math::Point getIntersection(const math::Point& point, const math::Vector<3>& direction) const override;
	std::string toString() const override;
	void transform(const math::Matrix<4> &matrix) override;
	std::vector<std::vector<Plane>> boundingPlanes() const override;
    std::shared_ptr<Surface> makeDeepCopy(const std::string &newName) const override;

private:
	math::Point refPoint_;  // 参照点(軸上の一点)
	math::Vector<3> refDirection_;  // 軸方向は単位ベクトルとする。これはコンストラクタで保証する。
	double radius_;

public:
	static std::unique_ptr<Cylinder> createCylinder(const std::string &name,
													const std::vector<double> &params,
													const math::Matrix<4> &trMatrix,
													TYPE type, bool warnPhitsCompat);

	BoundingBox generateBoundingBox() const override;
#ifdef ENABLE_GUI
public:
	virtual vtkSmartPointer<vtkImplicitFunction> generateImplicitFunction() const override;
#endif
};

}  // end namespace geom
#endif // CYLINDER_HPP
