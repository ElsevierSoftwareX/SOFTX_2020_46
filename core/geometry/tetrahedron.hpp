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
#ifndef TETRAHEDRON_HPP
#define TETRAHEDRON_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"
#include "core/utils/message.hpp"
#include "core/geometry/surface/surfacemap.hpp"

namespace geom{


class Surface;
class TetrahedronFactory;

class Tetrahedron :public std::enable_shared_from_this<Tetrahedron> {
	friend class TetrahedronFactory;
public:
	explicit Tetrahedron(const std::string &name):name_(name),groupID_(0){;}
	// 位相情報とnode辞書から構築
	Tetrahedron(const std::string &baseName,
				const std::array<size_t, 4> &ph,
				const std::unordered_map<std::size_t, math::Point> &nodes,
				std::unordered_set<std::string> *createdSet,
				SurfaceMap* smap);
	std::string name() const {return name_;}
	std::array<math::Point, 4> points() const {return points_;}
	std::unordered_map<std::shared_ptr<const Surface>, std::shared_ptr<Tetrahedron>> &neighborInfo() {return neighborInfo_;}
	std::string equation() const {return equation_;}
	std::string surroundingEquation() const {return surroundingEquation_;}
	int groupID() const {return groupID_;}
	void dump();
	// set**は隣への伝播を伴わない
	void setName(const std::string &name) {name_ = name;}
	//void setEquation(const std::string &eq) {equation_ = eq;}  // equationは自力で構築できるからセットする必要は無いはず…
	void setGroupID(int id) {groupID_ = id;}
	void setNeighbor(const std::shared_ptr<const Surface> &sideSurface, std::shared_ptr<Tetrahedron> neighbor);
	// (現時点で)隣接tetraを持たない独立面をセットする
	void setSurface(const std::shared_ptr<const Surface> &surface);
	// オプション的パラメータをセット
	void setParameter(const std::pair<std::string, std::string> &paramPair) {parameters_.emplace(paramPair);}
	// 同一IDを受け取った時は何もせず、異なるIDを受け取ったら更新して隣へ伝搬させる
	void receiveID(const std::shared_ptr<const Surface> &sideSurface, int newID);
	// 隣接テトラ情報を受け取る。このとき相手方にも自分が隣接であることを伝える。
	void receiveNeighbor(std::shared_ptr<const Surface> &sideSurface,
						 std::shared_ptr<Tetrahedron> neighbor,
						 const SurfaceMap &smap);
	// 第一引数の反対側に隣接neighborのポインタをnullptrにする。
	void removeNeighbor(const std::shared_ptr<const Surface> &surface);
	// 第一引数のより向こう側（但し第一引数は四面体構成面ではない）にある四面体要素に切り離しを依頼する。
	void removeOtherSideNeighbor(const std::shared_ptr<const Surface> &surface, const SurfaceMap &smap);

	std::vector<std::shared_ptr<const Surface> > allSurfaces() const;
	std::vector<std::shared_ptr<const Surface>> outerSurfaces() const;

	// 頂点で一つでもplaneの外(前方)ならばfalsee
	bool isBackward(const std::shared_ptr<const Surface> &plane) const;

	const std::unordered_map<std::string, std::string> &parameters() const {return parameters_;}

private:
	std::string name_;
	std::string equation_;
	std::string surroundingEquation_;
	int groupID_;
	std::array<math::Point, 4> points_;
	// 面の名前と面の向こう側にある四面体へのポインタ。
	// ※コンストラクタ終了時にはneighbor情報は正しくないことに注意。
	std::unordered_map<std::shared_ptr<const Surface>, std::shared_ptr<Tetrahedron>> neighborInfo_;
	std::unordered_map<std::string, std::string> parameters_;
};

class Tetrahedra {
public:
	Tetrahedra(const std::shared_ptr<Tetrahedron> &tet);
	const std::vector<std::shared_ptr<Tetrahedron>> elements() const {return elements_;}
	std::string outerEquation() const;
private:
	std::vector<std::shared_ptr<Tetrahedron>> elements_;

	bool isConvex() const;

	static int uniqueID;
};



class TetrahedronFactory{
public:
	TetrahedronFactory(){;}
	void append(const std::shared_ptr<Tetrahedron> &tet){tetraVector_.emplace_back(tet);}
	void generateConvexTetrahedra();

	std::vector<std::shared_ptr<Tetrahedron>> *tetraMap() {return &tetraVector_;}
	static void setUniqueID(const SurfaceMap &smap, int ignoreID, std::vector<std::shared_ptr<Tetrahedron>> *tetraVec);

	std::vector<Tetrahedra> tetrahedras() const {return tetrahedras_;}
	std::string surroundingEquation() const;
private:
	std::vector<std::shared_ptr<Tetrahedron>> tetraVector_;
//	std::vector<std::vector<std::shared_ptr<Tetrahedron>>> convexClusters_;
	std::vector<Tetrahedra> tetrahedras_;
	//


static size_t uniqueID;
};


}  // end namespace geom
#endif // TETRAHEDRON_HPP
