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
#ifndef QUADRIC_HPP
#define QUADRIC_HPP

#include "surface.hpp"

#include <memory>
#include <string>
#include <vector>

#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"

#include "vtksurfaceheaders.hpp"

namespace geom {


class Quadric : public Surface
{
public:
	enum TYPE{SQ, GQ};
	Quadric(const std::string &name, const std::vector<double> &params);

	// virtualの実装
	std::string toInputString() const override;
	std::unique_ptr<Surface> createReverse() const override;
	std::string toString() const override;
	void transform(const math::Matrix<4> &matrix) override;
	bool isForward(const math::Point &p) const override;
	math::Point getIntersection(const math::Point& point, const math::Vector<3>& direction) const override;
	std::vector<std::vector<Plane>> boundingPlanes() const override;
    std::shared_ptr<Surface> makeDeepCopy(const std::string &newName) const override;

private:
    // A-KはMCNP/PHITSマニュアルの定義に準ずる
	double A_, B_, C_, D_, E_, F_, G_, H_, J_, K_;
//    std::unique_ptr<math::Matrix<4>> matrix_;   //  粒子の座標を局所座標に変換するTransform
//	math::Matrix<4> invMatrix_;                 // ↑の逆行列=Surfaceに対するTransform
	std::vector<double> parameters() const {return std::vector<double>{A_, B_, C_, D_, E_, F_, G_, H_, J_, K_};}

public:
    // MCNPのmnemonicとパラメータから二次曲面をunique_ptrで生成する
	static std::unique_ptr<Quadric> createQuadric(const std::string &name,
												  const std::vector<double> &params,
												  const math::Matrix<4> &trMatrix,
												  TYPE type,
												  bool warnPhitsCompat);
#ifdef ENABLE_GUI
public:
	virtual vtkSmartPointer<vtkImplicitFunction> generateImplicitFunction() const override;
#endif
};


}  // end namespace geom
#endif // QUADRIC_HPP
