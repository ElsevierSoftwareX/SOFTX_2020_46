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
#ifndef MATERIAL_H
#define MATERIAL_H

#include <cassert>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "nuclide.hpp"
#include "core/physics/physconstants.hpp"

namespace inp {
class DataLine;
}

namespace ace {
class XsDir;
}

namespace mat {

/*
 * 密度の違う２つのMaterialは別々のインスタンスにするか？
 * 案１：別々のインスタンスにする
 *		利点：計算は速い
 *		欠点：密度の数だけmaterialインスタンスが生成されてメモリが使われる
 *
 * 案２：同じMaterialとして扱い、平均自由行程等を計算するときに引数で密度を与える
 *		利点：メモリ量削減
 *		欠点：速度は落ちる
 *
 * とりあえあず案2で行く。
 * 断面積データは結構大きいから最悪セル数分だけインスタンス作ることになるのはやばい。
 * 速度といってもたかだか掛け算と足し算数回くらいでしょ。
 *
 * Materialクラスは
 * 複数の{Nuclide,割合}ペアを持ち、
 * 密度とエネルギーから巨視的断面積平均自由行程を計算する。
 * 密度はcellが保持するのでこのクラスは保持しない。
 */


class Material
{
public:
	typedef int id_type;
	typedef std::unordered_multimap<phys::ParticleType, std::pair<std::shared_ptr<const Nuclide>, double>> nuclide_map_type;
	// 引数なしコンストラクタは"void"マテリアルを作成する
    Material():id_(Material::VOID_ID), name_(VOID_NAME){;}
    // 引数に文字列とIDだけをあたえた空のデータを生成する
    Material(const std::string &name, int id):id_(id), name_(name) {;}

	Material(const std::string &name, int id,
			 const nuclide_map_type &nuclides,
			 const std::string &bfName);

	// 平均自由行程を返す
	double mfp(phys::ParticleType ptype, double dens, double energy) const {return 1.0/macroTotalXs(ptype, dens, energy);}
	// 巨視的全断面積を返す
	double macroTotalXs(phys::ParticleType ptype, double dens, double energy) const;
	// 微視的全断面積を返す

	int id() const {return id_;}
	const std::string &name() const {return name_;}
	const std::string &bfName() const {return buildupFactorName_;}
	// 断面積と存在率(質量割合)のペアを格納している二重ベクトル
	const std::vector<std::vector<std::pair<std::shared_ptr<const Nuclide>, double>>> &nuclides() const {return nuclides_;}
	// 平均原子量(amu)
	double averageAtomicMass() const { return averageNuclideMassAmu_;}
private:
	// 一意に識別するための番号。シミュレーションを通して順に割り振られる。
	// id番号⇔文字列名 の対応関係はmap<string, int>で管理され、このマップは
	// Materialのstatic変数として保持され、インスタンス発生ごとに更新される。
	int id_;  //
	std::string name_;
	// 密度の値は直接保持しない。代わりに平均原子量を保持する。（原子数比と重量比の換算のため）
	double averageNuclideMassAmu_;
	/*
	 * 核種データは
	 * nuclides[ParticleType].at[index].first で核種データを
	 * nuclides[ParticleType].at[index].second でその核種の質量存在割合を
	 * 返す。 indexは材料構成核種のインデックス。1材料1核種ならindex=0のみ
	 *
	 * 核種の存在割合は粒子種によって変わり得るので注意。
	 * 例：光子は26000位置核種だが、中性子は26054, 26055等複数核種に別れるため。
	 */
	std::vector<std::vector<std::pair<std::shared_ptr<const Nuclide>, double>>> nuclides_;
	std::string buildupFactorName_;


// static
public:
	// 未定義,void領域,領域境界の材料名。シミュレーションには使わないが断面画像生成などに使う。
    static const char UNDEF_NAME[];// = "*M_undef*";  // 通常材料名とバッティングしないこと。
    static const char VOID_NAME[];// = "*M_void*";
    static const char BOUND_NAME[];// = "*M_bound*";  // 通常境界
    static const char UBOUND_NAME[];// = "*M_ubound*";  // 未定義セル境界
    static const char DOUBLE_NAME[];// = "*M_double*";  // 二重定義セル
    static const char OMITTED_NAME[];// = "*M_omitted*";  // 省略された領域
	// 未定義,void領域の材料ID。これらはユーザーが定義しないので組み込みで定義しておく。シミュレーションで使う。
	static constexpr int VOID_ID = 0;
	static constexpr int UNDEF_ID = -1;

    // matNameが予約済みマテリアル名の場合trueを返す。
    //static bool isReservedName(const std::string matName);
	static const std::shared_ptr<const Material> void_material_ptr();
	//
	static std::shared_ptr<const Material> createMaterial(const std::string &idStr,
														  std::size_t id,
														  const std::shared_ptr<const ace::XsDir> &xsdir,
														  const std::vector<phys::ParticleType> &ptypes,
                                                          const std::vector<std::string> &nuclideParams,
                                                          const std::unordered_map<std::string, std::string> &opts,
														  const std::string buildupFactorName);



};




}  // end namespace mat
#endif // MATERIAL_H
