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
#ifndef TORUS_HPP
#define TORUS_HPP

#include "surface.hpp"

#include <memory>
#include <string>
#include <vector>

#include "vtksurfaceheaders.hpp"
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"

namespace geom {


class Torus : public Surface
{
public:
	enum class TYPE{TX, TY, TZ, TA};
	Torus(const std::string &name, const math::Point &center, const math::Vector<3> &axis,
		  double majorR, double minorVRadius, double minorHRadius);

	// virtualの実装
	std::string toInputString() const override;
	std::unique_ptr<Surface> createReverse() const override;
	std::string toString() const override;
	void transform(const math::Matrix<4> &matrix) override;
	bool isForward(const math::Point &point) const override;
	math::Point getIntersection(const math::Point& point, const math::Vector<3>& direction) const override;
	std::vector<std::vector<Plane>> boundingPlanes() const override;
    std::shared_ptr<Surface> makeDeepCopy(const std::string &newName) const override;

    math::Vector<3> normal(const math::Point &pos) const;

	static std::unique_ptr<Torus> createTorus(const std::string &name,
	                                           const std::vector<double> &params,
	                                           const math::Matrix<4> &trMatrix, TYPE type, bool warnPhitsCompat);
	math::Point center() const;  // (変換があれば)座標変換済みの”実際の”中心
	math::Vector<3> axis() const; // (変換があれば)座標変換済みの”実際の”軸方向単位ベクトル
    double majorRadius() const {return R;}
    double minorVRadius() const {return a;}
    double minorHRadius() const {return b;}



private:
	// トーラス中心は常に原点 式が大変になるから。トーラス軸方向は常にz軸上向き。
	// 一般Torusを実装しようとしていた時の名残でcenter_, axis_に値を代入できそうになっている所があることに注意
	const math::Point center_;
	const math::Vector<3> axis_;
	const double R;  // 大半径
	const double a;  // 縦小半径
	const double b;  // 横小半径
	const double a2; // 高速化と式のみやすさのためa*aを保持しておく
	const double b2;
	/*
	 *  回転行列。z軸平行原点中心トーラス以外はこれを利用する。
	 * TorusはtrMatrix_によって変換されているとして扱う。
	 * 実際の交点計算時にはsurfaceをtrMatrix_で変換するのではなく、
	 * 逆行列invMatrix_で粒子を変換し得られた交点をtrMatrix_で元の座標系に戻す。
	 *
	 * trMatrix_ :torus局所座標系→グローバル座標系への変換行列
	 * invMatrix_:グローバル座標系→torus局所座標系への変換行列
	 */
    std::unique_ptr<math::Matrix<4>> trMatrix_;
	math::Matrix<4> invMatrix_;

	BoundingBox generateBoundingBox() const override;
#ifdef ENABLE_GUI
public:
	virtual vtkSmartPointer<vtkImplicitFunction> generateImplicitFunction() const override;
#endif


};


}  // end namespace geom
#endif // TORUS_HPP
