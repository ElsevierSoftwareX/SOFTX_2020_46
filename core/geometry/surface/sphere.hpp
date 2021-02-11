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
#ifndef SPHERE_HPP
#define SPHERE_HPP

#include <iostream>
#include <memory>
#include <string>
#include "surface.hpp"

#include "vtksurfaceheaders.hpp"



namespace math {
template <unsigned int M> class Vector;
}

namespace geom {




class Sphere : public Surface
{
public:
    enum TYPE {S, SO, SX, SY, SZ};

    // 球は名前、中心座標、半径で生成する。
    Sphere(const std::string& name, const math::Point& center, const double& radius);

    math::Point center() const {return center_;}
    double radius() const {return radius_;}

    // virtualの実装
    // 入力ファイルとして使える文字列を返す。 "name S x y z r"のような。
    std::string toInputString() const override;
    std::string toString() const override;
    std::unique_ptr<Surface> createReverse() const override;
    void transform(const math::Matrix<4> &matrix) override;
    bool isForward(const math::Point &point) const override;
    math::Point getIntersection(const math::Point &point, const math::Vector<3>& direction) const override;
    std::vector<std::vector<Plane>> boundingPlanes() const override;
    std::shared_ptr<Surface> makeDeepCopy(const std::string &newName) const override;


    BoundingBox generateBoundingBox() const override;
#ifdef ENABLE_GUI
public:
	virtual vtkSmartPointer<vtkImplicitFunction> generateImplicitFunction() const override;
#endif

private:
	math::Point center_;
	double radius_;


public:
	static std::unique_ptr<Sphere> createSphere(const std::string &name,
												const std::vector<double> &params,
												const math::Matrix<4> &trMatrix,
												enum TYPE type, bool warnPhitsCompat);

};


std::ostream& operator << (std::ostream& os, const Sphere& sp);


}  // end namespace geom
#endif // SPHERE_HPP
