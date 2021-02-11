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
#ifndef SURFACEMAP_HPP
#define SURFACEMAP_HPP

#include <stdexcept>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "core/math/nvector.hpp"
#include "core/utils/message.hpp"

namespace geom {


class Surface;

/*
 * Surfaceを格納するためのマップ
 * ・[int]あるいは.at(int)でアクセスする。O(1)必須
 * ・at(string)でのアクセスは遅くても良い。
 * ・負のインデックスを許容する
 * ・正のインデックスは表面、負のインデックスは裏面を格納する
 * ・要素数は表面数を高速に返せるようにする(交差判定に使うので)。裏面数、全体数は低速でも良い
 * ・全おもて面に逐次アクセス可能であること
 *
 * 非constメソッドの内registerSurfaceは内部で相互排他しているからスレッドセーフ
 * retisterTrSurfaceは内部でretisterSurface呼んでいるだけだからスレッドセーフ
 *
 * 当初内部ではvector<shared_ptr<const Surface>>を保持し、operator[]はそのままvectorにアクセス
 * するようにしていたが、surfaceIDは必ずしも連続しないので大量の歯抜けが出てメモリ消費が激しくなっていた。
 * ゆえにstd::unordered_set<sared_ptr<const Surface>>に切り替える。
 *
 */
class SurfaceMap
{
public:
//	typedef std::vector<int>::size_type size_type;
	typedef std::unordered_map<int, std::shared_ptr<Surface>>::size_type size_type;
    SurfaceMap(){}
	explicit SurfaceMap(const std::initializer_list<std::shared_ptr<Surface>> &iList);

	size_type size() const{return frontSurfaces_.size() + backSurfaces_.size();}
	// mapに index でsurfを登録する。NOTE surf->getID()で登録するのは自明だから第一引数不要では？
	void registerSurface(int index, const std::shared_ptr<const Surface> &surf);
    void registerTrSurface(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
						   const std::tuple<std::string, std::string, std::string> &surfTuple,
						   bool isGeneratedAutomatically = false);

	std::string frontSurfaceNames() const;
	std::string toString() const;

	// アクセス(高速であるべし)
	/* operator[], at()はconstのみのアクセスとする。登録はregisterSurfaceを必ず使う。
	 * おもて面裏面共にindexと要素アクセスindexは1個ずれる。
	 * (表裏の定義できないindex=0をvectorに保存しないため)
	 *
	 * 有効なshared_ptrが格納されていない場合は….at()では例外発生。
	 * 測度重視の所以外は.at()を使うこと。
	 */
	const std::shared_ptr<const Surface> &at(int index) const {
		//if(index = 0) throw std::out_of_range("SurfaceMap: Zero index access");
		return (index > 0) ? frontSurfaces_.at(index) : backSurfaces_.at(index);
	}
	const std::shared_ptr<const Surface> &at(const std::string &surfName) const {
		return at(getIndex(surfName));
	}




	// iteratorやrange-based forでアクセスする場合、どこも指さないshared_ptrが入っている可能性に留意する必要がある。
	std::unordered_map<int, const std::shared_ptr<const Surface>> &frontSurfaces() {return frontSurfaces_;}
	const std::unordered_map<int, const std::shared_ptr<const Surface>> &frontSurfaces() const {return frontSurfaces_;}
	std::unordered_map<int, const std::shared_ptr<const Surface>> &backSurfaces() {return backSurfaces_;}
	const std::unordered_map<int, const std::shared_ptr<const Surface>> &backSurfaces() const {return backSurfaces_;}

	// それほど高速でなくても良い
	// 検索
	bool hasSurfaceName(const std::string &targetName) const;
	// 削除
	void erase(int index);
	// string-index変換
	int getIndex(const std::string &name) const;
	// index-string変換
	std::string getName(int index) const;
	// string-index変換マップを返す
	const std::unordered_map<std::string, int> &nameIndexMap() const {return nameIndexMap_;}

	// polynomial.evaluate()に渡す用。indexに対応したsurfaceのisForward(pos)を呼び出す実際にはposはstd::bindして使う。
	bool operator () (int index, const math::Point& pos) const;

	void dumpFrontSufraceList(std::ostream &os) const;

// static
	// nameEquationのsurface名をsmapのgetIndex(name)で置き換える。
	static std::string makeIndexEquation(const std::string &nameEquation, const SurfaceMap &smap);


private:
	// おもて面、裏面を保存するvector
//	std::vector<std::shared_ptr<const Surface>> frontSurfaces_;
//	std::vector<std::shared_ptr<const Surface>> backSurfaces_;
	std::unordered_map<int, const std::shared_ptr<const Surface>> frontSurfaces_;
	std::unordered_map<int, const std::shared_ptr<const Surface>> backSurfaces_;
	// name->index変換テーブル
	std::unordered_map<std::string, int> nameIndexMap_;

	// operator()から参照する位置
//	math::Point position_;
};



std::ostream &operator<<(std::ostream &ost, const SurfaceMap &smap);



}  // end namespace geom
#endif // SURFACEMAP_HPP
