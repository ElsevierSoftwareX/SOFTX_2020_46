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
#ifndef NUCLIDE_HPP
#define NUCLIDE_HPP

#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef NO_ACEXS
#include "component/libacexs/libsrc/acefile.hpp"
#include "component/libacexs/libsrc/mt.hpp"
#endif
namespace mat {


/*
 * nuclide(核種の断面積データ)はサイズが大きいので重複して生成しない。
 * createNuclide(string zaid)ではその核種が生成済みならその核種へのスマートポインタを返し、
 * そうでない場合はmake_shared()で生成する。ようなプール方式にする。
 */
// 個別の核種データ
class Nuclide {
public:
	Nuclide(){;}
#ifndef NO_ACEXS
	Nuclide(const std::string &zaidx, double awr, const ace::AceFile::XSmap_type &xsmap);
    const ace::AceFile::XSmap_type &xsMap() const {return xsMap_;}
    double getTotalXs(double energy) const;
    const std::vector<double> &totalXsEpoints() const {return totalXs_.epoints;}
#endif


	std::string toString() const;
	// 異種核データ同核種データを混ぜる可能性があるためzaidには拡張子が含まれている。
	const std::string &zaid() const {return zaidx_;}
	double awr() const {return awr_;}

private:
	std::string zaidx_;  // 核種名 (ZAID/SZAX)
	double awr_;  // 原子質量(中性子を1にした単位)

#ifndef NO_ACEXS
    ace::AceFile::XSmap_type xsMap_;  // 断面積テーブル
	ace::CrossSection totalXs_;  // 全断面積は使用頻度高いので別に保持する。
#endif




// static
public:
	static std::shared_ptr<const Nuclide> createNuclide(const std::string &filename, double awr,
									   const std::string &zaidx, std::size_t startline);

    /*
     * nuclideプールの初期化メソッドを実装しないとGUI時に違うファイルを再読込したとき
     * 無制限に核種が増大してしまう。だけど実際はあまりないシチュエーションだし、
     * 、同一ファイル何回も開きがちであることを考えると毎回は初期化しない
     * 一定サイズを超えたときのみ
     */

	static bool ZaidLess(const std::string &zaid1, const std::string &zaid2);
	static bool NuclideLess(const mat::Nuclide& nuc1, const mat::Nuclide &nuc2);
	static bool NuclidePLess(const std::shared_ptr<const mat::Nuclide> &nuc1, const std::shared_ptr<const mat::Nuclide> &nuc2);

    static void clearPool() {nuclidePool.clear();}
    static size_t poolSize() {return nuclidePool.size();}
private:
	static std::mutex poolMtx;
	static std::unordered_map<std::string, std::shared_ptr<const Nuclide>> nuclidePool;
};

std::ostream &operator << (std::ostream &os, const Nuclide &nuc);
std::ostream &operator << (std::ostream &os, const std::pair<std::shared_ptr<const Nuclide>, double> &nucPair);




}  // end namespace mat
#endif // NUCLIDE_HPP
