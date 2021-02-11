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
#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include <array>
#include <exception>
#include <sstream>
#include "vtksurfaceheaders.hpp"

#include "surface.hpp"
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"

namespace geom{

/*
 * 三角形クラス
 *
 * math::は
 * 行ベクトル 右手系 行メジャー
 *
 */


// NOTE クラスによってメンバ変数のnormal_がウラ面で逆向いているものと逆向いていないものがある。
//      メンバnormal_は表面と同じ向きで、isForawrdでは表面の結果を反転させているだけ
//      メンバnormal_は正しく裏側を向いていて isForwardは反転させずに実行している
// の2種類が混じっている
//  normal()が正しい向きを向いてればどちらでもいい。が後者は境界面直上の取扱に注意する必要が出る。
//  Triangleではnormal()はconstリファレンスを返したいのでnormal_は正しい側を向かせてisForawardでも反転させない
//

class Triangle : public Surface
{
	// 時計回りが表
public:
    Triangle (const std::string &name, const std::array<std::shared_ptr<math::Point>, 3> &v, double torelance = 1e-5, bool ccw = false);

    Triangle(const std::string &name, const std::array<math::Point, 3> &v, bool ccw = false);

	bool isNeighbor(const Triangle &tri);
	// pt を頂点に含んでいる場合true
	bool hasVertex(const std::shared_ptr<math::Point> &vertex) const {
		return vertex == vertices_[0] || vertex  == vertices_[1] || vertex  == vertices_[2];
	}


	std::string toString() const override;  // 情報文字列
	std::string toInputString() const override;  // SurfaceCard入力に使える文字列
	std::unique_ptr<Surface> createReverse() const override; // 裏面生成
	void transform(const math::Matrix<4> &matrix) override;  // 変換行列(回転と並進)による変換方法を規定
	bool isForward(const math::Point& point) const override;
	math::Point getIntersection(const math::Point& point, const math::Vector<3>& direction) const override;
	// 交点がエッジ上にあるかをチェックしながら交点を計算する。
	// 交点がエッジ上にある場合pair.firstがtrueとなる。
	// getIntersectionでは交点がエッジに来る場合は交点無しとしていた
	std::pair<bool, math::Point> getIntersection2(const math::Point& point, const math::Vector<3>& direction) const;
	std::vector<std::vector<Plane>> boundingPlanes() const override;
    std::shared_ptr<Surface> makeDeepCopy(const std::string &newName) const override;

	const math::Vector<3> &normal() const {return normal_;}
	const math::Point center() const {return 0.3333333333333 * ((*vertices_[0].get()) + *vertices_[1].get() + *vertices_[2].get());}
	const std::array<std::shared_ptr<math::Point>, 3> &vertices() const {return vertices_;}


	static std::unique_ptr<Triangle> createTriangle(const std::string &name,
												const std::vector<double> &params,
												const math::Matrix<4> &trMatrix,
												bool warnPhitsCompat);
	BoundingBox generateBoundingBox() const override;
#ifdef ENABLE_GUI
public:
	virtual vtkSmartPointer<vtkImplicitFunction> generateImplicitFunction() const override;
#endif

private:
//	std::array<math::Point, 3> vertices_;
	std::array<std::shared_ptr<math::Point>, 3> vertices_;
	math::Vector<3> normal_;


	// normal, vertex両指定のコンストラクタはcreateReverseでしか使わないので非公開
	Triangle(const std::string &name,
			 const std::array<math::Point, 3> &v,
			 const math::Vector<3> &n)
		:Surface("TRIANGLE", name), normal_(n)
	{
		for(size_t i = 0; i < 3; ++i) {
			vertices_[i] = std::make_shared<math::Point>(v[i]);
		}
	}
	Triangle(const std::string &name,
			 const std::array<std::shared_ptr<math::Point>, 3> &v,
			 const math::Vector<3> &n)
		:Surface("TRIANGLE", name), vertices_(v), normal_(n)
    {}


};

std::ostream& operator << (std::ostream& os, const Triangle& tri);

}  // end namespace geom

#endif // TRIANGLE_HPP
