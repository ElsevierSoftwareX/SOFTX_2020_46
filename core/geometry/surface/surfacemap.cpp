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
#include "surfacemap.hpp"


#include <map>
#include <mutex>
#include <regex>
#include <sstream>

#include "surface.hpp"
#include "surface_utils.hpp"
#include "core/utils/message.hpp"
#include "core/utils/matrix_utils.hpp"
#include "core/utils/string_utils.hpp"

#include "core/io/input/cardcommon.hpp"

namespace {
bool isFrontName(const std::string &name)
{
	return (name.empty() || name.front() == '-') ? false : true;
}

}  // end anonymous namespace


bool geom::SurfaceMap::hasSurfaceName(const std::string &targetName) const
{
	//mDebug() << "Enter SurfaceMap::hasSurfaceName, targetName ===" << targetName;
	if(isFrontName(targetName)) {
		for(const auto &surfPair: frontSurfaces_) {
			/*
			 * FIXME マルチスレッドFiilingを実行するとここで落ちることがある。
			 * surfPair.second == falseなのにその後ループで書き出そうとすると
			 * 何故か割り当てられているおそらくデータアクセス順序で変わると思われる。
			 */
//			if(!surfPair.second) {
//				mDebug() << "unllocated pointer!!! id===" << surfPair.first;
//				for(const auto &sp: frontSurfaces_) {
//					mDebug() << "keys===" << sp.first << "surface===" << ((sp.second) ? sp.second->name(): "NOT allocated");
//				}
//				abort();
//			}

			if(surfPair.second->name() == targetName) return true;
		}
	} else {
		for(const auto &surfPair: backSurfaces_) {
			if(surfPair.second->name() == targetName) return true;
		}
	}
	return false;
}

// indexの要素を削除。
void geom::SurfaceMap::erase(int index)
{
	assert(index != 0);
	if(index > 0) {
		if(frontSurfaces_.erase(index) == 0) {
			throw std::invalid_argument("no surface is registered with id=" + std::to_string(index));
		}
	} else {
		if(backSurfaces_.erase(index) == 0) {
			throw std::invalid_argument("no surface is registered with id=" + std::to_string(index));
		}
	}
}

int geom::SurfaceMap::getIndex(const std::string &name) const
{
//	std::cerr << "名前" << name << "の面のindexを探索。" << std::endl;
//	for(auto &nipair: nameIndexMap_) {
//		std::cerr << nipair.first << ", " <<  nipair.second << std::endl;;
//	}
	if(nameIndexMap_.find(name) != nameIndexMap_.end()) {
//		std::cerr << "発見 =" << nameIndexMap_.at(name) << std::endl;
		return nameIndexMap_.at(name);
	} else {
//		std::cerr << "見つからず。 return 0" << std::endl;
		throw std::out_of_range(std::string("surface \"") + name + "\" not found in surfacemap");
	}
}



bool geom::SurfaceMap::operator ()(int index, const math::Point &pos) const
{
//	return this->operator[](index)->isForward(pos);
	return this->at(index)->isForward(pos);
}

void geom::SurfaceMap::dumpFrontSufraceList(std::ostream &os) const
{
	std::string name, str;
	int id;
	for(auto &fssPair: frontSurfaces_) {
		name = fssPair.second->name();
		id = this->getIndex(name);
		str = fssPair.second->toString();
		os  << "ID=" << id << ", " << str << std::endl;
	}
}



/*
 * Mapにsurfaceを登録する。
 * 内部ではvectorで保持するのでidが飛び飛びの場合飛んでいる箇所にはnullptrを入れる。
 * surfaceのIDは1から順に重複なしで割り振られることになっているので、
 * idが飛び飛びになった場合の処理はあくまでイレギュラーなものであり
 * 処理に時間がかかっても問題ない。
 *
 */
geom::SurfaceMap::SurfaceMap(const std::initializer_list<std::shared_ptr<geom::Surface>> &iList)
{
	for(auto &surf: iList) {
		registerSurface(surf->getID(), surf);
	}
}

// surface名でアクセスすればとりあえず間違うことが無いが、IDってなんで必要なんだっけ？？
// surfaceの一意ID(surf->id())使うなら引数のidいらん
// 経緯を忘れた。
// hash対応済み
void geom::SurfaceMap::registerSurface(int id, const std::shared_ptr<const geom::Surface> &surf)
{
	static std::mutex mtx; // C++11ではlocal static variable の初期化はthreadsafe

	std::lock_guard<std::mutex> lg(mtx);
	if(id > 0) {
		if(frontSurfaces_.find(id) != frontSurfaces_.end()) {
			//mDebug() << nameIndexMap_;
			std::stringstream ss;
			ss << "Registering surface, name=" << surf->name() << " id=" << id << ","
			   << " id is duplicated (former name=" << frontSurfaces_[id-1]->name()<<")";
			throw std::invalid_argument(ss.str());
		}
		frontSurfaces_.emplace(id, surf);
//		frontSurfaces_[id] = surf;

	} else if (id < 0) {
		if(backSurfaces_.find(id) != backSurfaces_.end()) {
			throw std::invalid_argument(std::string("id=") + std::to_string(id) + " is already registered.");
		}
//		backSurfaces_[id] = surf;
		backSurfaces_.emplace(id, surf);
	} else {
		throw std::out_of_range("SurfaceMap: Zero index registration");
	}
	nameIndexMap_[surf->name()] = id;
}

/*
 * TRCLで新たに生成されたサーフェイスを生成してsurfacemapに登録する。
 * 第一引数はTrMap(TRCLではTR番号指定の場合があるため)
 * 第二引数は新たに生成するsurface情報を含んだタプル。
 * タプルは
 * <0> TRされる元のsurface名
 * <1> TRCLしているセル名
 * <2> TRCLの引数文字列
 *
 * 例外が発生するのは... getIndex(name)のところ。
 */
void geom::SurfaceMap::registerTrSurface(const std::unordered_map<size_t, math::Matrix<4> > &trMap,
                                         const std::tuple<std::string, std::string, std::string> &surfTuple,
                                         bool isGeneratedAutomatically)
{
    // FIXME ここでもとのsurfaceに適用されているTRが取得できていない

    (void) isGeneratedAutomatically;
	static std::mutex mtx;
	std::lock_guard<std::mutex> lg(mtx);
	std::string oldName = std::get<0>(surfTuple), trCell = std::get<1>(surfTuple), trStr = std::get<2>(surfTuple);
//	std::string newInputString =this->at(oldName)->toInputString() + trStr;
    // 新しいsurface名が一意になるように オリジナルsurf名とTR実行セル名から新しいsurf名を作成している。
    std::string newName = inp::getTransformedSurfaceName(trCell, oldName);

    // 既に同名のsurfaceが登録済みの場合は新規にsurface生成・登録はしない。
    if(hasSurfaceName(newName)) return;

    // trStrには"trsf=(0 0 0) trsf=(6)”みたいな感じになっているのでこれを
    // generateTransformMatrixが受け入れられる単一TR文字列 "0 0 0,6" に変換する必要がある。
    // (0 0 0),(6)はOKか→ だめ。
    auto trStr2 = trStr;
    utils::tolower(&trStr2);
    std::replace(trStr2.begin(), trStr2.end(), '=', ' ');
    utils::trim(&trStr2);
    if(trStr2.size() > 4 && trStr2.substr(0, 4) == "trsf") {
        trStr2 = trStr2.substr(4);
    }
    const std::regex reg("trsf", std::regex_constants::icase);
    const std::string fmt =",";
    trStr2 = std::regex_replace(trStr2, reg, fmt);
    // TRSFでは入れ子は無いので()は全削除で良い
    const std::regex parreg(R"(\(|\))");
    trStr2 = std::regex_replace(trStr2, parreg, "");
    auto matrix = utils::generateTransformMatrix(trMap, trStr2);
    //Surface::deepCopy(newName)を実装して、SurfaceCardを経由しないようにした。
    std::shared_ptr<Surface> newSurface = this->at(oldName)->makeDeepCopy(newName);
    newSurface->transform(matrix);

//    /*
//     * registerTrSurfaceは
//     * ・明示的TRCLの掛かっているセルで使われている面を登録する場合と
//     * ・暗黙のTRCLが適用される格子要素セルで使われている面を登録する場合
//     * に使われる。後者のセル・面は自動生成なのでfortran数式やijmr表現は含まれないのでチェック
//     * しないようにして高速化できる。
//     */
//    // 自動生成されたsurfaceCardはfortran数式とijmr展開を含まないので省略できる。
//    inp::SurfaceCard newCard = inp::SurfaceCard::fromString(newInputString, false, isGeneratedAutomatically, isGeneratedAutomatically);
//    mDebug() << "正しいTR文字列は===" << newCard.trStr;
//    auto matrix = utils::generateTransformMatrix(trMap, newCard.trStr);
//    newCard.name = newName;
//    // TR前のsurface作成時にwarnCompatしているだろうからここでは最後の引数はfalse
//    auto newSurface = geom::createSurface(newCard.name, newCard.symbol, newCard.params, newCard.paramMap, matrix, false);


    //	mDebug() << "TR生成サーフェイスカード=" << newCard.name << " " << newCard.symbol << ", " << newCard.params;
//	mDebug() << "TRstring=" << trStr;
//	mDebug() << "TRmatrix=" << matrix;
//	mDebug() << surface->toString();
	//mDebug() << "surfName=" << surface->name() << "surfID=" << surface->getID();

    this->registerSurface(newSurface->getID(), newSurface);
	// 裏面も作って登録する
    std::shared_ptr<geom::Surface> revSurf(newSurface->createReverse());
	this->registerSurface(revSurf->getID(), revSurf);
}

std::string geom::SurfaceMap::frontSurfaceNames() const
{
	std::stringstream ss;
	ss << "{";
	for(const auto &frontSurfPair: frontSurfaces_) {
		ss << frontSurfPair.second->name() << "(" << frontSurfPair.second->getID() << ") ";
	}
	ss << "}";
	return ss.str();
}

std::string geom::SurfaceMap::toString() const
{
	std::stringstream ss;
	ss << "front {" << std::endl;
	for(const auto &frontPair: frontSurfaces_) {
		ss << "front =" << frontPair.second->toString() << std::endl;
	}
	ss << "}" << std::endl;
	ss << "back {" << std::endl;
	for(const auto &backPair: backSurfaces_) {
		ss << backPair.second->toString() << std::endl;
	}
	ss << "}" << std::endl;
	return ss.str();
}

// nameEquationのsurface名をsmapのgetIndex(name)で置き換える。
std::string geom::SurfaceMap::makeIndexEquation(const std::string &nameEquation, const geom::SurfaceMap &smap)
{
	//mDebug() << "name equation=" << nameEquation;
	std::smatch sm;
	std::regex surfaceNamePattern = Surface::getSurfaceNamePattern();
	auto it = nameEquation.cbegin() ;
	auto end = nameEquation.cend() ;
	std::map<std::string, int> replacingNameAndIndexes;  // nameと対応するindexを格納するマップ
	while(std::regex_search(it, end, sm, surfaceNamePattern)) {
		//std::cout << "matched=\"" << sm.str(1) << "\" replacing to \"" << smap.getIndex(sm.str(1)) << "\"" << std::endl;
		std::string name = sm[2];
		if(name.front() == '+') name = name.substr(1);
		// 置換元は＋を取ったらマッチしなくなるが、getIndexでは+を取らないと駄目。
		replacingNameAndIndexes[sm[2]] = smap.getIndex(name);
		it = sm[0].second;
	}

	//mDebug() << "replacing table=\n" << replacingNameAndIndexes;

	std::string indexEquation = nameEquation;

	for(auto &nameIndexPair: replacingNameAndIndexes) {
		// 置換するindexは＊で囲む。これは置換後のindexが他のsurface名にマッチしてしまうのを防ぐ
		std::string indexStr = std::string("*") + std::to_string(nameIndexPair.second) + "*";
		std::string target = nameIndexPair.first;
		// surface名中のピリオド,+が正規表現にならないようにエスケープ
		target = std::regex_replace(target, std::regex(R"(\.)"), std::string("\\."));
		target = std::regex_replace(target, std::regex(R"(\+)"), std::string("\\+"));


		target = std::string("([^0-9a-zA-Z*]|^)(") + target + std::string(")([^0-9a-zA-Z*]|$)");
		//std::cout << "Replacing =\"" << target << "\" by \"" << indexStr << "\" from=\"" << indexEquation << "\"" << std::endl;
		std::regex namePattern(target);		// 1→5 へ変換する場合、 12→52と変換されないように注意する必要がある。


		// std::regex_replaceだと他の名前の一部にマッチして置換してしまう
		// if(std::regex_search(.... だequation内に複数この該当がある場合に対応できない
		// while(...)だとindexStr自体がnamepatternに含まれる場合終わらなくなる。

		std::string retStr;
		while(std::regex_search(indexEquation, sm, namePattern)) {
			// sm[n].firstがマッチ最初文字、secondが最終文字の次
//			for(size_t i = 0; i < sm.size(); ++i) {
//				mDebug() << "sm" << i << "=" << sm.str(i);
//			}

			retStr += std::string(indexEquation.cbegin(), sm[2].first) + indexStr;
			// マッチした位置が文字列最後端ならsm[2].secondは.indexEquation.cend()になるのでretStr取得後にbreak;
			if(sm[2].second == indexEquation.cend()) {
				indexEquation = "";
				break;
			}

			indexEquation = std::string(sm[2].second, indexEquation.cend());

			//mDebug() << "indexEq=" << indexEquation;
		}
		retStr += indexEquation;
		indexEquation = retStr;
		//mDebug() << "retstr=" << retStr;
		//indexEquation = std::regex_replace(indexEquation, namePattern, indexStr) ;
	}
	// 余計な"*"を削除
	auto itrNewEnd = std::remove_if(indexEquation.begin(),indexEquation.end()
									,[](char ch) { return ch == '*'; });

	indexEquation.erase(itrNewEnd, indexEquation.end());
	//mDebug() << "indexEq =" << indexEquation;
	//return retStr;
	return indexEquation;
}




std::ostream &geom::operator<<(std::ostream &ost, const geom::SurfaceMap &smap)
{
//	ost << smap.frontSurfaceNames();
//	return ost;
	//ost << "enter operatorr<<" << std::endl;
	Surface::map_type::size_type sz = 0;
	for(auto it = smap.frontSurfaces().begin(); it != smap.frontSurfaces().end(); it++) {
		sz++;
		if(it->second) ost << (it->second)->toString() << std::endl;;
		//if(sz != smap.frontSurfaces().size()) ost << std::endl;
	}
//	sz = 0;
//	for(auto it = smap.backSurfaces().begin(); it != smap.backSurfaces().end(); it++) {
//		sz++;
//		if(*it) ost << (*it)->toString() << std::endl;

//		//if(sz != smap.backSurfaces().size()) ost << std::endl;
//	}
	return ost ;
}
