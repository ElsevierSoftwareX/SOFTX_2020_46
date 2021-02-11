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
#ifndef BOUNDINGBOX_HPP
#define BOUNDINGBOX_HPP

#include <array>
#include <atomic>
#include <limits>
#include <unordered_set>
#include <set>
#include <vector>

#include "core/math/nvector.hpp"


namespace geom {

class Plane;

/*
 * バウンディングボックス。AABB(軸平行境界ボックス)にしようと思ったが、
 * 軸平行だと斜めの平面のAABBが無限大の広さになり、
 * 斜め平面で定義されたセルのAABBの計算が難しくなる。
 *
 * ので、OOBB(Object-Oriented Bounding Boxを用いる。
 *
 * と思ったがOOBBの計算は大変すぎるのでやはりAABBでできる範囲をやる
 */

class BoundingBox
{
	typedef math::Vector<3> Vec;
	friend bool isOverlapping(const BoundingBox b1, const BoundingBox &b2);
	friend bool isSameBB(const BoundingBox b1, const BoundingBox &b2);
	friend bool hasIntersection(const BoundingBox bb, const Plane &pl);
public:
	static const double MAX_EXTENT;
	static BoundingBox universalBox();
	static BoundingBox emptyBox();
	// b1, b2両者を包含するBDを作成する。
	static BoundingBox OR (const BoundingBox &b1, const BoundingBox &b2);
	// b1, b2の重なる部分を包含するBDを作成する
	static BoundingBox AND (const BoundingBox &b1, const BoundingBox &b2);


	BoundingBox():xmin(0), xmax(0), ymin(0), ymax(0), zmin(0), zmax(0){;}
	BoundingBox(double x1, double x2, double y1, double y2, double z1, double z2);
	BoundingBox(const std::array<double, 6> &arr)
		:BoundingBox(arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]){;}

	// OOBB中心座標を返す
	math::Point center() const {return 0.5*math::Point{xmin+xmax, ymin+ymax, zmin+zmax};}
	std::string info() const;
	std::array<double, 6> range() const;
	double volume() const {return (xmax-xmin)*(ymax-ymin)*(zmax-zmin);}
	std::string toInputString() const;

	// xyzそれぞれの方向にfactor倍に拡大する。
	void expand(double factor);
	bool empty() const;
	/*
	 *  無限大ボックスならtrueを返す。
	 * 引数がtrueなら±xyz全ての方向に無限大の場合のみtrueを返し、
	 * 引数がfalseなら何れか一つの方向でも無限大の広がりがあればtrueを返す。
	 */
	bool isUniversal(bool compareStrinct) const;
	// thisがbbを完全に包含している場合trueを返す。
	bool contains(const BoundingBox &bb);

	// 未実装。つかわないこと
	bool hasIntersection(const math::Point &pt, const math::Vector<3> &dir) const;

	// アフィン変換の適用
	void transform(const math::Matrix<4> &matrix);
	// 平行移動のみの時は安いコストのこちらを使う
	void translation(const math::Vector<3> &transVec);
	// bb を構成する面を返す
    std::vector<std::vector<Plane> > boundingSurfaces(bool isInward) const;

	// 文字列"{xmin,xmax,ymin,ymax,zmin,zmax}"からBoundingBoxを作成
	static BoundingBox fromString(const std::string &str);
	static BoundingBox fromPoints(std::initializer_list<const math::Point *> lst);
	static BoundingBox fromPoints(std::vector<math::Point> points);
	// 面からのBB構築は時間が爆発する場合があるのでtimeoutFlagを設定する。
	static BoundingBox fromPlanes(std::atomic_bool *timeoutFlag, const std::vector<std::vector<Plane>> &planeVectors);

	// 第2引数のvectorに第三引数のvectorをマージする
	static std::vector<std::vector<Plane> > mergePlaneVectorsAnd(const std::atomic_bool *timeoutFlag,
									 const std::vector<std::vector<Plane> > &vecs1,
									 const std::vector<std::vector<Plane>> &vecs2);

	// 8個の頂点を返す
	std::vector<math::Point> vertices() const;

private:


	/*
	 * BoundingBox は直方体
	 * 内部表現は
	 *  xmin, xmax, ymin, ymax, zmin, zmaxの6データ
	 */
	double xmin, xmax, ymin, ymax, zmin, zmax;


};

// b1 と b2が交差していればtrue。完全に離れているか完全に含まれていればfalse
bool isOverlapping(const BoundingBox b1, const BoundingBox &b2);
// 同一性判定関数
bool isSameBB(const BoundingBox b1, const BoundingBox &b2);

// 面とBBが交差するならtrue
bool hasIntersection(const BoundingBox bb, const Plane &pl);







}
// end namespace geom

#endif // BOUNDINGBOX_HPP
