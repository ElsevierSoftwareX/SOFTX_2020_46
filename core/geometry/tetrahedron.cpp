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
#include "tetrahedron.hpp"

#include <array>
#include <bitset>
#include "surface/surfacemap.hpp"
#include "surface/plane.hpp"
#include "core/utils/string_utils.hpp"

size_t geom::TetrahedronFactory::uniqueID = 0;


// 頂点IDから生成する面の名前を作る。
std::string generatePlaneName(const std::string &base, size_t n1, size_t n2, size_t n3)
{
	return base + "_" + std::to_string(n1) + "," + std::to_string(n2) + "," + std::to_string(n3);
}
// ここから四面体要素の構築
// 三角形要素を構築するためのnodeIDのトリオ。構築時に順序を保ったままnodeID最小のものがfirstに来るようにする。
struct VertexTrio
{
	VertexTrio(){;}
	VertexTrio(size_t id1, size_t id2, size_t id3)
	{
		setIDs(id1, id2, id3);
	}

	void setIDs(size_t id1, size_t id2, size_t id3)
	{
		// idに重複は無いものとする。
		size_t min_id = std::min({id1, id2, id3});
		if(id1 == min_id) {
			first = id1;
			second = id2;
			third = id3;
		} else if(id2 == min_id) {
			first = id2;
			second = id3;
			third = id1;
		} else {
			first = id3;
			second = id1;
			third = id2;
		}
	}

	size_t first;
	size_t second;
	size_t third;
};



// 重複をチェックしながら面を生成する。返り値は四面体内側を向いた構成面
const std::shared_ptr<const geom::Surface> createAndRegisterSurfaces(const std::string &base,  // 平面のベース名
									  size_t id0, size_t id1, size_t id2, // 面生成に使う頂点のID
									  const std::unordered_map<std::size_t, math::Point> &nodes,  // 頂点IDをキーにして頂点を格納したマップ
									 std::unordered_set<std::string> *createdSet, // 生成された面の名前を保存
									  geom::SurfaceMap *smap)  // 生成した面はsurfaceMapに追加する。生成しなければ追加しない
{
	VertexTrio trio(id0, id1, id2);
	std::string name = generatePlaneName(base, trio.first, trio.second, trio.third);
	std::string nameR = "-" + generatePlaneName(base, trio.first, trio.third, trio.second);
		// すでに裏面として生成済みかチェックする。
	if(createdSet->find(nameR) == createdSet->end()) {
		// 他の面の裏面として生成済みでなければ新たに生成・追加する。
		// equationを作成するのが楽なように要素内側を向くように生成する。
		createdSet->emplace(name);
		auto pl = std::make_shared<geom::Plane>(name, nodes.at(trio.first), nodes.at(trio.second),
										  nodes.at(trio.third), geom::Plane::NormalType::ClockWise);
		smap->registerSurface(pl->getID(), pl);
		// 面は必ず表裏ペアで作成するので片方のみが生成済みということはない。
		// 故にいま生成したpl面のウラ面が生成済みということはあり得ない。
		assert(createdSet->find(generatePlaneName(base, trio.first, trio.third, trio.second)) == createdSet->end());
		std::shared_ptr<geom::Surface> plr = pl->createReverse();
		smap->registerSurface(plr->getID(), plr);
		createdSet->emplace(plr->name());
		return pl;
	} else {
		return smap->at(nameR);
	}
}



geom::Tetrahedron::Tetrahedron(const std::string &baseName,
								const std::array<size_t, 4> &ph,
							   const std::unordered_map<std::size_t, math::Point> &nodes,
							   std::unordered_set<std::string> *createdSet,
							   SurfaceMap* smap): groupID_(0)
{
//	for(size_t i = 0; i < ph.size(); ++i) {
//		points_[i] = nodes.at(ph[i]);
//	}

	// まず4面生成(4面とは限らないが)
	// 要素作成時に未作成の面はとりあえず法線が要素の内向きになるよう反時計回りに生成する。
	// Planeの法線はコンストラクタでどちら向きでも選べるがclockwiseが基本
	std::array<std::shared_ptr<const Surface>, 4> planes;
	planes[0] = createAndRegisterSurfaces(baseName, ph[0], ph[3], ph[1], nodes, createdSet, smap);  // p0p3p1面
	planes[1] = createAndRegisterSurfaces(baseName, ph[0], ph[1], ph[2], nodes, createdSet, smap);  // p0p1p2面
	planes[2] = createAndRegisterSurfaces(baseName, ph[0], ph[2], ph[3], nodes, createdSet, smap);  // p0p2p3面
	planes[3] = createAndRegisterSurfaces(baseName, ph[1], ph[3], ph[2], nodes, createdSet, smap);  // p1p3p2面


	surroundingEquation_ += "(";
	for(auto &pl: planes) {
		equation_ += pl->name() + " ";
		surroundingEquation_ += (pl->name().front() == '-') ? pl->name().substr(1) : "-" + pl->name();
		surroundingEquation_ += ":";
		neighborInfo_.emplace(pl, nullptr);
	}
	assert(neighborInfo_.size() == 4);
	surroundingEquation_.pop_back();
	surroundingEquation_ += ") ";
	equation_.pop_back();
}

void geom::Tetrahedron::dump() {
	mDebug() << "Tetra name=" << name_ << ", groupID=" << groupID_;
	for(auto &n: neighborInfo_) {
		if(n.second != nullptr) {
			mDebug() << "surface=" << n.first->name() << "neighbor==" << n.second->name();
		} else {
			mDebug() << "surface=" << n.first->name() << "neighbor== nullptr";
		}
	}
}

void geom::Tetrahedron::setNeighbor(const std::shared_ptr<const Surface> &sideSurface, std::shared_ptr<geom::Tetrahedron> neighbor)
{
//	mDebug() << "current neighbor surf=";
//	for(auto p: neighborInfo_) {
//		mDebug() << p.first->name();
//	}
//	mDebug() << "element=" << name_ << " is called setNeighbor, surf=" << sideSurface->name() << "arg2=" << neighbor->name();
	neighborInfo_[sideSurface] = neighbor;
	assert(neighborInfo_.size() == 4);
}

void geom::Tetrahedron::setSurface(const std::shared_ptr<const Surface> &surface)
{
	neighborInfo_[surface] =  nullptr;
	assert(neighborInfo_.size() == 4);
//	auto result = neighborInfo_.emplace(surface, nullptr);
//	assert(result.second);
}

void geom::Tetrahedron::receiveID(const std::shared_ptr<const Surface> &sideSurface, int newID)
{
	// IDが同じならID変更を伝搬させない（さもないと循環して無限ループになるして）
	if(newID == groupID_) return;
	groupID_ = newID;
	for(auto &neighborPair: neighborInfo_) {
		// ここはポインタ比較で良い。
		if(neighborPair.second && neighborPair.first != sideSurface) neighborPair.second->receiveID(neighborPair.first, groupID_);
	}
}

void geom::Tetrahedron::receiveNeighbor(std::shared_ptr<const Surface> &sideSurface,
										std::shared_ptr<geom::Tetrahedron> neighbor,
										const SurfaceMap &smap)
{
	//mDebug() << "element name=" << name_ << " received neighbor info, surf=" << sideSurface << "neighbor name=" << neighbor->name();
	receiveID(sideSurface, neighbor->groupID());
	neighborInfo_[sideSurface] = neighbor;
	assert(neighborInfo_.size() == 4);
	std::string reverseName = sideSurface->name().front() == '-' ? sideSurface->name().substr(1) : "-" + sideSurface->name();
	neighbor->setNeighbor(smap.at(reverseName), shared_from_this());
}

void geom::Tetrahedron::removeNeighbor(const std::shared_ptr<const geom::Surface> &surface)
{
//	mDebug() << "In removeNeighbor this=" << name() << "removing request against surface=" << surface->name();
//	this->dump();
	auto it = neighborInfo_.find(surface);
	assert(it != neighborInfo_.end());

	it->second = nullptr;
}

void geom::Tetrahedron::removeOtherSideNeighbor(const std::shared_ptr<const geom::Surface> &surface, const SurfaceMap &smap)
{
	for(auto &info: neighborInfo_) {
		if(info.second && info.second->isBackward(surface)) {
			info.second->removeNeighbor(smap.at(Surface::reverseName(info.first->name())));
		}
	}
}


// 四面体を定義している内側向きの面全てを返す
std::vector<std::shared_ptr<const geom::Surface> > geom::Tetrahedron::allSurfaces() const
{
	assert(neighborInfo_.size() == 4);
	std::vector<std::shared_ptr<const Surface>> retVec;
	for(auto it = neighborInfo_.cbegin(); it != neighborInfo_.cend(); ++it) {
		retVec.emplace_back(it->first);
	}
	return retVec;
}

// 隣接四面体と共有していない面を返す
std::vector<std::shared_ptr<const geom::Surface> > geom::Tetrahedron::outerSurfaces() const
{
	assert(neighborInfo_.size() == 4);
	std::vector<std::shared_ptr<const Surface>> retVec;
	for(auto it = neighborInfo_.cbegin(); it != neighborInfo_.cend(); ++it) {
		// gidが別なら別のクラスタなので外部扱いする。
		if(it->second == nullptr || it->second->groupID() != groupID_) {
			retVec.emplace_back(it->first);
		}
	}
	return retVec;
}



bool geom::Tetrahedron::isBackward(const std::shared_ptr<const Surface> &plane) const
{
	/*
	 * 数値誤差の関係で
	 * tet->isBackward(tet->outerSurfaces)とやってもtureが返るとは限らない。
	 * これはこまるので自分自身のplaneかどうかを判定して、その時は無条件にtrueを返す。
	 * それがいやなら自分自身の面にたいするisBackward適用は徹底して避ける必要がある。
	 */
	for(auto &info: neighborInfo_) {
		if(info.first == plane) return true;
	}

	//mDebug() << "\nTetra=" << name_ << "面=" << plane->name() << "の前後判定";
	for(auto &pt: points_) {
		if(!plane->isForward(pt)) {
			return false;  // 四面体定義面の外側を判定したいので裏返っている。
		} else {
		}
	}
	return true;
}




void geom::TetrahedronFactory::setUniqueID(const geom::SurfaceMap &smap, int ignoreID, std::vector<std::shared_ptr<Tetrahedron> > *tetraVec)
{
	// ここからgroupID分け
	// 面の所有者を保存するマップ。所有者はその面を作成した(=プラス面を保持する)四面体を表す。
	std::unordered_map<std::string, std::shared_ptr<Tetrahedron>> ownerMap;

//	mDebug() << "Enter setUniqueID!!!!!!!!!!! 現在のtetra情報は";
//	for(auto &tet: *tetraVec) {
//		tet->dump();
//	}

	for(auto &tet: *tetraVec) {
		std::bitset<4> hasNeighbors("0000");
		auto planes = tet->allSurfaces();
		assert(planes.size() <= 4);

		/*
		 * 隣接先4面体ポインタが非nullptrの場合向こう側に4面体が存在する。
		 * その上で面の所有者があるか、向こう側の4面体のgIDが無視指定でなければ隣接4面体ありとする。
		 */
		int index = 0;
		for(auto &plane: planes){
			std::shared_ptr<Tetrahedron> neighbor =tet->neighborInfo().at(plane);
			//mDebug() << "plane===" << plane->name() << "の反対側に4面体があるか探索中...";

//			if(ownerMap.find(plane->name()) == ownerMap.end()) || neighbor != nullptr && neighbor->groupID() != ignoreID)
			if(ownerMap.find(plane->name()) != ownerMap.end() &&
					(neighbor == nullptr ||	(neighbor != nullptr && neighbor->groupID() != ignoreID))) {
				// planeを介して四面体に隣接している場合相手側情報を受け取って伝搬させる
				hasNeighbors[index] = 1;
				tet->receiveNeighbor(plane, ownerMap.at(plane->name()), smap);
			} else{
//				mDebug() << "tet=" << tet->name() << "は"<< plane->name() << "の向こう側に隣接四面体を持たない";
//				mDebug() << "条件1 面の向こうはぬるぽ？" << (tet->neighborInfo().at(plane) == nullptr);
//				mDebug() << "条件2 ownermapに面が掲載されている？" << (ownerMap.find(plane->name()) != ownerMap.end());
				//mDebug() << "条件3 面の向こうのIDが無視指定ID?  ID=" << tet->neighborInfo().at(plane)->groupID();
				// (有効な)隣接四面体を持たない場合
				ownerMap[Surface::reverseName(plane->name())] = tet;  // owner登録は裏面。ウラ面に対する所有権を主張する
                // FIX(ME tetsimple+MSVCではここで落ちるbitset index outside range
				assert(static_cast<size_t>(index) < hasNeighbors.size());
                hasNeighbors[index] = 0;
				tet->setSurface(plane);
			}
            ++index;
		}
		if(hasNeighbors.none()) {
			//mDebug() << "tetra=" << tet->name() << "は孤立しているので新規ID=" << uniqueID << "を与える\n";
			tet->setGroupID(++uniqueID);
		}
//		mDebug() << "現時点でのownerMap=";
//		for(auto val: ownerMap) {
//			mDebug() << "surface=" << val.first << ", owner=" << val.second->name();
//		}
//		mDebug();
	}



//	mDebug() << "連結データ作成後のtetra情報は";
//	for(auto &tet: *tetraVec) {
//		tet->dump();
//	}
//	mDebug() << "DEBUG abort";
//	abort();
	using Ptet = const std::shared_ptr<Tetrahedron>&;
	std::sort(tetraVec->begin(), tetraVec->end(), [](Ptet tet1, Ptet tet2) {return tet1->groupID() < tet2->groupID();});
}


void geom::TetrahedronFactory::generateConvexTetrahedra()
{
	using Ptet = const std::shared_ptr<Tetrahedron>&;

//	mDebug() << "Enter genereateConvex tetraMap size=" << tetraMap_.size();
//	for(auto &tet: tetraMap_) {
//		mDebug() << "tet====" << tet->name() << "gid=" << tet->groupID();
//	}

	while(!tetraVector_.empty()) {
		// ここまででグループ分けした四面体集合ができた。次はこれらが凸になるまで分割を行う。
		// まずは同一ID部分の切り出し
		std::unordered_map<std::string, std::shared_ptr<Tetrahedron>> tetCluster;
		int id = tetraVector_.front()->groupID();
		auto sameIdItr = std::find_if(tetraVector_.begin(), tetraVector_.end(), [=](Ptet tet){return tet->groupID() != id;});

		for(auto it2 = tetraVector_.begin(); it2 != sameIdItr; ++it2) {
			tetCluster[(*it2)->name()] = *it2;
		}
		tetraVector_.erase(tetraVector_.begin(), sameIdItr);

//		mDebug() << "\nTetCluster作成。";
//		for(auto &tet: tetCluster) {
//			mDebug() << "暫定的tetClusterの四面体===" << tet.first << "gid=" << tet.second->groupID();
//			//tet.second->dump();
//		}
		assert(!tetCluster.empty());

		/*
		 * やはり隣接四面体を辿っていかないとろくな多面体ができない。
		 */
		while(!tetCluster.empty()) {
			for(auto it = tetCluster.begin(); it != tetCluster.end();) {
				//mDebug() << "\n最初にtetra=" << it->second->name() << "をセット";
				Tetrahedra tetrahedra(it->second);
				// tetrahedra構成要素をtetClusterから削除する


				for(auto elem: tetrahedra.elements()) {
					//mDebug() << "tetrahedra構成要素=" << elem->name() << "を元のクラスタから削除";
					it = tetCluster.find(elem->name());
					assert(it != tetCluster.end());
					it = tetCluster.erase(it);
				}
				tetrahedras_.emplace_back(tetrahedra);
			}
		}
	}

	//mDebug() << "tetrahedraの数=" << tetrahedras_.size();
}

int geom::Tetrahedra::uniqueID = 0;

geom::Tetrahedra::Tetrahedra(const std::shared_ptr<geom::Tetrahedron> &tet)
{
	//mDebug() << "Tetrahedraのコンストラクト 最初の要素=" << tet->name();
	// tetrahedraに追加した四面体要素はgidを暫定的に負にする。
	tet->setGroupID(-1*tet->groupID());
	elements_.emplace_back(tet);

	for(size_t i = 0; i < elements_.size(); ++i){
		auto lastElem = elements_.at(i);
		for(auto &info: lastElem->neighborInfo()) {
			// 凸多面体要素候補として追加する条件は 1.そもそもヌルポでない。2．追加済みでない。3．gIDが正である
			if(info.second && std::find(elements_.begin(), elements_.end(), info.second) == elements_.end()
					&& info.second->groupID() >= 0){
				elements_.emplace_back(info.second);
				elements_.back()->setGroupID(-1*elements_.back()->groupID());
				//mDebug() << "elem==" << info.second->name() << "を凸多面体要素候補として追加。convex?=" << isConvex();
				if(!isConvex()) {
					// 追加した結果凹になったので削除してリターン
					//mDebug() << "凹になってしまったので要素=" << elements_.back()->name() << "を削除";
					elements_.back()->setGroupID(-1*elements_.back()->groupID());
					elements_.pop_back();
				}
			}
		}
		if(!isConvex()) break;
	}
	// 新たに負の一意なIDを振り直す
	--uniqueID;
	for(auto &elem: elements_) {
		elem->setGroupID(uniqueID);
	}
//	mDebug() << "作成した凸多面体の要素=";
//	for(auto elem: elements_) {
//		mDebug() << "Name=" << elem->name() << "gid=" << elem->groupID();
//	}
}

bool geom::Tetrahedra::isConvex() const
{
	for(int i = 0; i < static_cast<int>(elements_.size()); ++i) {
		for(auto surf: elements_.at(i)->outerSurfaces()) {
			for(auto it = elements_.begin(); it != elements_.end(); ++it) {
				if(!(*it)->isBackward(surf)) return false;
			}
		}
	}
	return true;
}



std::string geom::TetrahedronFactory::surroundingEquation() const
{
	if(tetrahedras_.empty()) return "";

	std::string equation;
	for(auto &tetras: tetrahedras_) {
		equation += "(" + tetras.outerEquation() + ") ";
	}
	equation.pop_back();
	return equation;

//	for(size_t i = 0; i < convexClusters_.size(); ++i)
//	{
////	size_t i = 1;
//		equation += "(";
//		for(auto &tet: convexClusters_.at(i)) {
//			for(auto &surf: tet->outerSurfaces()) {
//				equation += Surface::reverseName(surf->name()) + ":";
//			}
//		}
//		equation.pop_back();
//		equation += ") ";
//	}
//	equation.pop_back();
//	return equation;
}



std::string geom::Tetrahedra::outerEquation() const
{
	if(elements_.empty()) return "";
	std::string equation;
	for(auto &elem: elements_) {
		for(auto &surf: elem->outerSurfaces()) {
			equation += Surface::reverseName(surf->name()) + ":";
		}
	}
	equation.pop_back();
	return equation;
}
