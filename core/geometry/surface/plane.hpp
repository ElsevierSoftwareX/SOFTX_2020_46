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
#ifndef PLANE_HPP
#define PLANE_HPP

#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

#include "core/math/nvector.hpp"
#include "surface.hpp"
#include "vtksurfaceheaders.hpp"


namespace geom{

class BoundingPlaneInfo;


// Bounding planeとして使用する場合はnameを空にすること。
class Plane : public Surface
{
	friend bool isSamePlane(const Plane &p1, const Plane &p2);
	friend bool isSamePlane(const std::shared_ptr<const Plane> &p1, const std::shared_ptr<const Plane> &p2);
	friend bool operator < (const Plane &pl1, const Plane &pl2);
public:
	enum TYPE{P, PX, PY, PZ};
	enum class NormalType{ClockWise, CoutnerClock, MCNP};
	/*
	 * normalとdistanceについて。
	 * 面の位置はnormal*distanceに存在すると扱う。
	 * 即ちVectorとPointをコンストラクタに取る時
	 * Plane(Vec{1, 0, 0}, Point{10, 0, 0})と
	 * Plane(Vec{-1, 0, 0}, Point{-10, 0, 0})は
	 * 同じ位置に存在し、法線が逆向きとなる。
	 *
	 *
	 * ・コンストラクタ  Plane(normal, point)
	 *   は内部で Plane(normal, distance = pt*normal)を呼ぶ。
	 * ・コンストラクタPlane(normal, distance)は
	 *     distance_ = distance/normal.abs();
	 *
	 * では一体メンバ変数normal_, distance_がなにを意味するかと言うと
	 * notmal_ は法線ベクトル(裏面でもちゃんと裏を向かせる)
	 * distance_ は面までの距離をnormal_
	 *
	 *
	 * リファクタリング方針！！！！！！
	 * ・メンバ変数normal_はウラ面でも実際の法線ベクトルに一致させる
	 *		→メソッドnormal()は単にreturn normal_;とする。
	 * ・メンバ変数distance_はnormal_*distance_が面上の点になるように定義する。
	 *		→表面とウラ面ではdistance_の符号は逆になる。
	 *	distance_は法線と面上の点の内積に等しい。
	 *  distance_は陰関数の定数項×−1に等しい。
	 *  distance_はMCNPのPXの最後の入力に等しい
	 *	createReverseで生成する裏面のnormal_は表面のnormal_×-1
	 *	createReverseで生成する裏面のdistance_は表面のdistance__×-1
	 *
	 * では裏面と表面の違いはというと、
	 * ・isForwardで距離0になった時の扱い
	 * ・ID値が対応する表面のID×-1
	 */
    Plane(const std::string &name, const math::Vector<3> &normal, double distance,
          bool hasBoundingPlane = true);
    Plane(const std::string &name, const math::Vector<3> &normal, const math::Point &pt,
          bool hasBoundingPlane = true);
    Plane(const std::string &name, const math::Point &p1, const math::Point &p2, const math::Point &p3,
          NormalType flag, bool hasBoundingPlane = true);
	// リファクタリングする。
	const math::Vector<3> &normal() const {return normal_;}
	double distance() const {return distance_;}
	// 入力ファイルにそのまま使える形式での文字列出力
	std::string toInputString() const override;
	// 点ptとの距離。ptが法線側にある場合は正、逆側なら負
	double distanceToPoint(const math::Point &pt) const;
	// ベクトルnと同じ側を向く(n*normal_>0)ように法線ベクトルを裏返す(かそのまま)。面の位置は変わらない。
	void alignNormal(const math::Vector<3> &n);

	// virtualの実装
	std::unique_ptr<Surface> createReverse() const override;
	bool isForward(const math::Point &p) const override;
	math::Point getIntersection(const math::Point& point, const math::Vector<3>& direction) const override;
	std::string toString() const override; // ndリファクタリングこkまでOK
	void transform(const math::Matrix<4> &matrix) override;
	std::vector<std::vector<Plane>> boundingPlanes() const override;
    std::shared_ptr<Surface> makeDeepCopy(const std::string &newName) const override;

private:
	// normal_は必ず単位ベクトルであること。コンストラクタで保証するべし。
	math::Vector<3> normal_;  // 法線ベクトル
	double distance_;  // 原点からの距離

// staticメンバ
public:
	// MCNP入力文字列相当からPlaneを生成。removeSign==trueなら面名のマイナスは除去する(が法線方向は変わらない)
	// 文字列ではTR番号およびTRSFは含まないこと。
	static std::unique_ptr<Plane> fromString(const std::string &srcStr, bool removeSign);
	//だいたいこれで生成
	static std::unique_ptr<Plane> createPlane(const std::string &name,
											  const std::vector<double> &params,
											  const math::Matrix<4> &trMatrix,
											  TYPE type,
											  bool warnPhitsCompat);
	static TYPE strToType(const std::string &str);
	//  面plへの点ptの(法線方向への)射影
	static math::Point projection(const Plane &pl, const math::Point &pt);
	// 面の同一性チェック
	static bool isSamePlane(const Plane &p1, const Plane &p2);
	// 面の同一性チェック。指しているplaneのnormalとplaneが同じならばtrue。
	// operator==(shared_ptr<Plane>, shared_ptr<Plane>)は指している先が同じ時にtrueなので注意。
	static bool isSamePlane(const std::shared_ptr<const Plane> &p1, const std::shared_ptr<const Plane> &p2);

	// 3平面から交点を返す。交点なしならmath::Point::INVALID_VECTORが返る
	static math::Point intersection(const Plane &p1, const Plane &p2, const Plane &p3);
	static math::Point intersection(const std::shared_ptr<Plane> &p1,
									const std::shared_ptr<Plane> &p2,
									const std::shared_ptr<Plane> &p3);


	BoundingBox generateBoundingBox() const override;
#ifdef ENABLE_GUI
public:
	virtual vtkSmartPointer<vtkImplicitFunction> generateImplicitFunction() const override;
#endif
};

bool operator < (const Plane &pl1, const Plane &pl2);

}  // end namespace geom
#endif // PLANE_HPP
