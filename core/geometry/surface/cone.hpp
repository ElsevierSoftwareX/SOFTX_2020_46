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
#ifndef CONE_HPP
#define CONE_HPP


#include "surface.hpp"
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"

#include "vtksurfaceheaders.hpp"

namespace geom {

class Cone : public Surface
{
public:
	// TYPE::KXOがMCNPのKXに対応、 TYPE::KXは K/Xに対応
	enum TYPE{KX, KXO, KY, KYO, KZ, KZO, KA};
    // surface名、 頂点、 頂点から底面方向への方向ベクトル、頂点から単位長さ動いた位置での半径(=tan(0.5頂角))、 正負どちらのシートか
	Cone(const std::string name, const math::Point &point, const math::Vector<3> &direction, double rad, int sheet);

	// virtualの実装
	std::string toInputString() const override;
	std::unique_ptr<Surface> createReverse() const override;
	bool isForward(const math::Point &p) const override;
	math::Point getIntersection(const math::Point& point, const math::Vector<3>& direction) const override;
	std::string toString() const override;
	void transform(const math::Matrix<4> &matrix) override;
	std::vector<std::vector<Plane>> boundingPlanes() const override;
    std::shared_ptr<Surface> makeDeepCopy(const std::string& newName) const override;

private:
	math::Point vertex_;  // 円錐頂点
	math::Vector<3> axis_;  // 軸方向単位ベクトル
	double radius_;         // 頂点から軸方向へ1動いた位置での半径(=の勾配)
	int sheet_;             // 正：+側1シート、負：-側1シート、0：2シート
	/*
	 *  axis_方向のシートがプラス側シート
	 *  逆方向のシートがマイナス側シート
	 * この定義に合うようにMCNP入力からrefPoint_, axis_を設定する。
	 */
	// ↓は高速化するためにコンストラクタで事前に計算しておく
	double cos_;  // cosθ(θは円錐頂点の角度cone angle) =(0, pi/2))
	math::Matrix<3> M_;  // 軸ベクトルテンソル= axis^T axis

public:
	static std::unique_ptr<Cone> createCone(const std::string &name,
                                             std::vector<double> params,
                                             const math::Matrix<4> &trMatrix,
                                            TYPE type, bool warnPhitsCompat);

	BoundingBox generateBoundingBox() const override;

#ifdef ENABLE_GUI
public:
	virtual vtkSmartPointer<vtkImplicitFunction> generateImplicitFunction() const override;
#endif
};

}  // end namespace geom
#endif // CONE_HPP
