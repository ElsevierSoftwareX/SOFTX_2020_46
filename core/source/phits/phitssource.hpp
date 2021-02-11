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
#ifndef PHITSSOURCE_HPP
#define PHITSSOURCE_HPP

#include <array>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/physics/physconstants.hpp"
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/inputdata.hpp"
#include "core/io/input/phits/phitsinput.hpp"
#include "core/io/input/meshdata.hpp"
#include "core/geometry/cell/cell.hpp"
#include "core/option/config.hpp"

namespace phys {
class Particle;
class UncollidedPhoton;
}

namespace inp {
namespace phits {
class PhitsInputSection;
}
}



namespace src {

class Distribution;

/*
 * 空間分布はどう扱うか？空間は角度、エネルギーと違って次元が3つあり、
 * 空間分布必ずしもf(x, y, z)= A(x)B(y)C(z) のように変数分離形で書けるとは限らない。
 *
 * →
 * しかし、phitsに限って言えば ソースはxyz, rθz, rθφのどれかの座標系で
 * 変数分離形(球座標等では自由度2の)で書ける。
 * なのでPhitsSourceクラスの内部表現としては、空間分布は3つのDistributionで保持し、
 * pdfは合計5つのpdfの積とする。
 *
 *
 */


class PhitsSource
{
public:

	PhitsSource(const inp::DataLine &start,
				const std::unordered_map<std::string, std::string> paramMap,
				phys::ParticleType ptype,
				double factor,
				const math::Point &localOrg,
				const math::Vector<3> &refDir,
				std::shared_ptr<Distribution> eDist,
				std::shared_ptr<Distribution> aDist,
				std::array<std::shared_ptr<Distribution>, 3> sDists,
				const std::vector<std::shared_ptr<const geom::Cell>> cells,
				const inp::MeshData& emesh,
				const std::array<inp::MeshData, 3> spMeshes)
	{
		startLine_ = start;
		parameterMap_ = paramMap;
		particleType_ = ptype;
		factor_ = factor;
		localOrigin_ = localOrg;
		referenceDir_ = refDir;
		energyDistribution_ = eDist;
		angularDistribution_ = aDist;
		spatialDistributions_ = sDists;
		acceptableCells_ = cells;
		energyMesh_ = emesh;
		spatialMeshes_ = spMeshes;
	}


	virtual std::string toString() const;
	virtual math::Point toCartesian(double p1, double p2, double p3) const = 0;  // (p1, p2, p3)を(x,y,z)に変換する。

	// Phitsの粒子角度分布は自由度が1つしか無い。φ(azimthal)は一様分布が用いられる。
	// 故に分布はエネルギー1次元、粒子放出角度1次元、位置3次元となる。
	const std::shared_ptr<const Distribution> energyDistribution() const { return energyDistribution_;}
	const std::shared_ptr<const Distribution> angularDistribution() const { return angularDistribution_;}
	std::array<std::shared_ptr<Distribution>, 3> spatialDistributions() const { return spatialDistributions_;}

    // セッター
    void setFactor(double factor) {factor_ = factor;}

	// ゲッター
    double factor() const {return factor_;}
    phys::ParticleType particleType() const {return particleType_;}

    // 粒子の発生
	std::vector<std::shared_ptr<phys::UncollidedPhoton> > generateUncollideParticles(const math::Point &detPoint,
							   const std::unordered_map<std::string, std::shared_ptr<const geom::Cell> > &cellList,
							   bool recordEvent) const;
	std::vector<math::Point> sourcePoints() const;

	// InputDataから全てのソースを格納したvectorを作成して返す。
	static std::vector<std::shared_ptr<const PhitsSource>>
		createSources(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
					   const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cellMap,
					   const inp::phits::PhitsInput &phitsInp,
					   const conf::Config &config);

	// ソースセクションからマルチソースを生成するstatic関数
	static std::vector<std::shared_ptr<const PhitsSource>>
		createMultiSource(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
					  const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cellMap,
					  const std::list<inp::DataLine> &inputList,
					  bool verbose,
					  bool warnPhitcCompat);

	static const std::unordered_map<std::string, std::string> &defaultParameterMap();
	static const std::vector<std::string> &essentialKeywords();  // Phitsソースに必須のパラメータリスト
	static const std::vector<std::string> &acceptableParams();   // Phitsソースで入力して良いパラメータリスト
	static const std::vector<std::pair<std::string, std::string> > &exclusiveKeywords(); // 排他利用パラメータ
	static const std::vector<std::string> &acceptableExtendedParams();

protected:
	// 読み込み開始位置
	inp::DataLine startLine_;
	// 粒子種
	phys::ParticleType particleType_;
	// 定数倍因子
	double factor_;
	// 局所座標系の原点
	math::Point localOrigin_;
	// 粒子発生方向のreference方向
	math::Vector<3> referenceDir_;
    // ソースのエネルギー、角度、空間分布
	std::shared_ptr<Distribution> energyDistribution_;
	std::shared_ptr<Distribution> angularDistribution_;
    std::array<std::shared_ptr<Distribution>, 3> spatialDistributions_;
	// 許容セル。これがemptyでなければCookie-cutter-cellのような働きをする。
	std::vector<std::shared_ptr<const geom::Cell>> acceptableCells_;

	std::unordered_map<std::string, std::string> parameterMap_;  // 入力されたパラメータを記録しておくデバッグ以外で使わない。



	// 実際に無衝突光子を発生させるエネルギー、空間位置。角度は線源位置と検出点から求まるのでユーザー定義しない
	inp::MeshData energyMesh_;
	std::array<inp::MeshData, 3> spatialMeshes_;


private:

	// ソース一つを作成。あとでfactor調整できるようにここでは非constにしている。
	static std::unique_ptr<PhitsSource> createSingleSource(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
													 const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cellMap,
													 const inp::phits::PhitsInputSection &inputSection);
	bool isInAcceptedCells(const math::Point pos) const;
};


/*
 * ソースを作る。
 * ソースの方向余弦分布		g(μ)
 * 方向余弦の基準となる方向	refDir
 * ソース発生位置の分布    p(x, y, z) or p(r, θ, φ)
 * エネルギー分布          f(E)
 *
 * point kernel コードで粒子を発生させる場合、
 * ・方向：線源から計算点方向に発生させ、その時の確率p(μ)が計算結果に必要
 * ・エネルギー：最終的にエネルギー積分∫g(E)dEする。
 *		離散E線源：g(E) = Σp_i δ(E-E_i)   各E_i点で発生させる。発生確率∫g(E)dE=p_iが計算結果に必要
 *		連続E線源：代表的なE点で発生させる。発生確率g(E)と幅ΔEが必要
 *
 * phitsでは位置依存エネルギー分布は扱わないのでその点は楽。
 *
 * 通常のMCでは確率に応じて粒子を発生させるが、無衝突線束計算粒子は角度やエネルギーを指定して
 * 発生させ、発生後にソースから粒子の発生確率を取得する。
 *
 * Source::getPdf(energy, point, direction) = f(E)g(μ)p(xyz)/2π
 */

/*
 * エネルギーの読み取り。
 * ほぼ全ての入力で群数neが入力されるので、それを使う。
 * 内部表現としては
 *	・ne個要素のvector<double>を保持
 *	・あとは適当な補間でE分布を表現
 * 現時点ではバイアスは不要なので警告を出してバイアス無視する。
 *
 * エネルギー分布はneとその後の自由形式部分を読み切らないと終わりがわからないので
 * 逐次読み取りして判定するしか無い。読み取ってから切り取って処理ルーチンに
 * 渡しても良いが二度手間にはなる。
 *
 */

/*
 * ※ソースのメッシュタイプ記述とタリーのメッシュ記述は似ているようで違うから注意。
 * エネルギーメッシュの指定は基本3系等(任意連続分布[1,21]、任意離散分布[8]、関数定義[2, 3, 5])でEバイアス版と波長入力版がある。
 *
 * ●e-type = 1: 群積分値による任意分布
 *	ne=で点数を与え、
 *  群下限エネルギー1  群1積分値
 *  群下限エネルギー2  群2積分値
 *       …              …
 *  最上群上限エネルギー
 *  で入力。データは 「2*ne + 1個」
 *
 *
 * ●e-type = 21:群微分値による任意分布
 *	type1の積分値を微分値にしたもの。
 *	neが正ならMeV、負ならレサジー幅で規格化
 *
 *
 * ●e-type = 8: 離散分布
 *	ne = エネルギー点数
 *	データ入力は自由形式で
 *  エネルギー1 発生確率1 エネルギー2 発生確率2
 *  エネルギー3 発生確率3 … エネルギーne  発生確率ne
 * のように 2*ne個入力する。
 *
 *
 * ●e-type = 2: ガウス分布
 *	eg0 = 中心
 *  eg1 = 半値幅
 *	eg2 = カットオフ下限値
 *	eg3 = カットオフ上限値
 *
 *
 * ●e-type = 3: マクスウェル分布
 *	nm=で分点を与える。
 *	et0 = 温度パラメータT(MeV)
 *	et1 = カットオフ下限
 *	et2 = カットオフ上限
 *
 *
 * ●e-type = 5: 任意関数
 *	nm = 分点
 *	f(x) FORTRAN形式で書いた関数 xはエネルギー(MeV/u)で f(x) = x**2 などのように記述
 *  eg1 = カットオフ最小値
 *	eg2 = カットオフ最大地
 *
 *
 *
 *
 *
 * e-type = 4, 7, 24: それぞれtype(1, 3, 21)を各Ebin粒子数が均等(あるいはユーザー定義数)になるようにバイアスを掛けたもの
 * e-type = 9: type8にバイアスを掛けて粒子数均等化したもの
 * e-type =11, 12, 13, 14, 15, 16, 31, 34: それぞれ(1, 2, 3, 4, 5, 6, 21, 24)のエネルギーを波長で入力にした版
 *
 *
 * ＋3するとバイアス版(例外は7。3のバイアス版)
 * +10すると波長入力版
 * 例外は9(8+1)
 *
 */


/*
 * 角度分布の読み取り.
 * 大きく分けて2系統(群データ、任意関数)
 * 群データ：1, 4, 11, 14
 * 任意関数：5, 6, 15, 16
 *
 *
 * ●a-type = 1
 *	na=で点数を与え、
 *  群下限角度余弦1  群1積分値
 *  群下限角度余弦2  群2積分値
 *       …              …
 *  最上群上限角度余弦
 *  で入力。データは 「2*ne + 1」個
 *
 *
 * ●a-type = 5: 任意関数
 *	nn = 分点数
 *	g(x) FORTRAN形式で書いた関数 xは角度で f(x) = x**2 などのように記述
 *  ag1 = カットオフ最小値
 *	ag2 = カットオフ最大地
 *
 *
 * a-type = 4,6  それぞれtype(1, 5)を各Abin粒子数が均等(あるいはユーザー定義数)になるようにバイアスを掛けたもの
 * a-type = 11, 14, 15 それぞれ(1, 4, 5)の角度をcosでなくdegreeで入力した版
 *
 * ＋10するとdegree入力
 */


} // end namespace src

#endif // PHITSSOURCE_HPP
