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

#include "phitscylindersource.hpp"

#include <array>
#include <cmath>
#include <regex>
#include <sstream>
#include <unordered_map>

#include "phitsenergydistribution.hpp"
#include "phitsangulardistribution.hpp"
#include "phitssourcefunction.hpp"
#include "core/source/abstractdistribution.hpp"
#include "core/source/discretedistribution.hpp"
#include "core/source/multigroupdistribution.hpp"
#include "core/source/proportionaldistribution.hpp"
#include "core/formula/fortran/fortnode.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/meshdata.hpp"
#include "core/io/input/phits/phits_metacards.hpp"
#include "core/io/input/phits/phitsinputsection.hpp"
#include "core/math/nvector.hpp"
#include "core/utils/utils.hpp"
#include "core/utils/numeric_utils.hpp"

/*
 * TRCLの実装は下底中心とref方向をtransformして
 * 線源の空間分布そのものをTransrofmしておく
 */
src::PhitsCylinderSource::PhitsCylinderSource(const inp::DataLine &startLine,
											  const std::unordered_map<std::string, std::string> paramMap,
											  phys::ParticleType ptype,
											  double factor,
											  const math::Point localOrigin,
											  const math::Vector<3> &xdir,
											  const math::Vector<3> &ydir,
											  const math::Vector<3> &zdir,
											  const math::Vector<3> &refDir,
											  std::shared_ptr<src::Distribution> eDistro,
											  std::shared_ptr<src::Distribution> aDistro,
											  std::array<std::shared_ptr<src::Distribution>, 3> sDistro,
											  const std::vector<std::shared_ptr<const geom::Cell> > &acceptableCells,
											  const inp::MeshData &emesh,
											  const std::array<inp::MeshData, 3> spMeshes)
	:PhitsSource(startLine, paramMap, ptype, factor, localOrigin, refDir, eDistro, aDistro, sDistro, acceptableCells, emesh, spMeshes),
	  xdirection_(xdir), ydirection_(ydir), zdirection_(zdir)
{;}


std::string src::PhitsCylinderSource::toString() const
{
	std::stringstream ss;
	ss << "Center =" << baseCenter() << std::endl;
	return PhitsSource::toString() + ss.str();
}


/*
 * ローカル円筒座標系からグローバルxyz座標系への変換
 * ・transformMatrix_を使うか
 * ・ローカル原点を使うか
 * 2通りのやり方がある。とりあえず後者で。
 */
math::Point src::PhitsCylinderSource::toCartesian(double r, double th, double z) const
{
	//mDebug() << "localorigin=" << localOrigin_;
	auto rad = math::toRadians(th);
	return localOrigin_ + r*(xdirection_*std::cos(rad) + ydirection_*std::sin(rad)) + z*zdirection_;
}


/*
 * Phitsソースを作成。
 * sourceSectionはマルチソースを個別ソースに分割済みであること。
 */
std::unique_ptr<src::PhitsCylinderSource>
src::PhitsCylinderSource::create(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
								const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cellMap,
								const inp::phits::PhitsInputSection &sourceSection,
								bool hasEnergyDistribution)
{
	using Vec = math::Vector<3>;
	using dVec = std::vector<double>;
	auto sourceInput = sourceSection.input();
	if(sourceInput.empty()) return std::unique_ptr<src::PhitsCylinderSource>();
	auto startPos = sourceInput.front().pos();
	std::shared_ptr<Distribution> energyDistribution, angularDistribution;
	std::vector<std::shared_ptr<const geom::Cell>>acceptableCells;
	// パラメータ読み取り。a-type, e-typeは単なるパラメータではなくサブセクション的な物を持つので別に取得する。
	double factor = 0;
	auto parameterMap = src::phits::readPhitsParameters(
											sourceInput,
											PhitsCylinderSource::defaultParameterMap(),
											PhitsSource::acceptableParams(),
											PhitsCylinderSource::essentialKeywords(),
											PhitsCylinderSource::exclusiveKeywords(),
											cellMap,
											&factor,
											&energyDistribution, &angularDistribution, &acceptableCells,
											hasEnergyDistribution);

	std::array<std::shared_ptr<Distribution>, 3> spatialDistributions;
	// r方向は(r1, r0)間で均一。但し形状因子が効くので分布は∝rのように1乗に∝。 あと、(r0, r1)では無いことに注意。
	auto r1 = fort::eq(parameterMap.at("r1")), r0 = fort::eq(parameterMap.at("r0"));
	spatialDistributions[0] = utils::isSameDouble(r1, r0) ? Distribution::makeDiscreteDistribution(dVec{r0}, dVec{1.0})
															: Distribution::makeProportionalDistribution(std::make_pair(r1, r0), 1);
	// theta方向は0, 2π間で均一
	spatialDistributions[1] = Distribution::makeProportionalDistribution(std::make_pair(0.0, 360), 0);
	// z方向は均一 (z0, z1)で均一
	auto z0 = fort::eq(parameterMap.at("z0")), z1 = fort::eq(parameterMap.at("z1"));
	spatialDistributions[2] = utils::isSameDouble(z0, z1) ? Distribution::makeDiscreteDistribution(dVec{z0}, dVec{1.0})
														  : Distribution::makeProportionalDistribution(std::make_pair(z0, z1), 0);

	// phitsではz0>z1でも構わないが、小さいほうが円筒の底として扱われる。
	math::Point origin = math::Point{fort::eq(parameterMap["x0"]),	fort::eq(parameterMap["y0"]), std::min(z0, z1)};

	/*
	 * TRCL処理は、baseCenterとreference方向の変換だけではすまない。
	 * 粒子発生時の座標も変換される。粒子発生座標取得はtoCartesian関数を通して実行されるので、
	 * toCartesianにもアフィン変換の結果が適用される必要がある。
	 *
	 * NOTE ソースの統一的記述としてソースローカル座標の基底をまとめて親クラスに移すか？
	 *      円筒ソースではrefdirとz方向の基底が一致しているがこれは偶々。
	 */
	auto referenceDir = Vec{0, 0, 1};  // phitsの円柱ソースのデフォルトreference方向はz軸正
	std::array<Vec, 3> bases {Vec{1, 0, 0}, Vec{0, 1, 0}, Vec{0, 0, 1}};
	if(parameterMap.find("trcl") != parameterMap.end() && !parameterMap.at("trcl").empty()) {
		try {
			auto matrix = utils::generateTransformMatrix(trMap, parameterMap.at("trcl"));
			// 円筒下底中心はアフィン変換して、他の方向ベクトルは回転変換のみ適用する。
			math::affineTransform<3>(&origin, matrix);
			auto rotMat = matrix.rotationMatrix();
			referenceDir = referenceDir* rotMat;
			for(auto &base: bases) base = base*rotMat;
		} catch (std::exception &e) {
            throw std::invalid_argument(startPos + " " + e.what() + ", TRCL parameter is invalid(card input or TR number is wrong).");
		}
	}

/*
 *  拡張コマンドの実装
 * ・エネルギーと位置の代表点を設定しなければならない。
 * ・離散分布の場合は代表点は設定しない。(離散点をそのまま使う。)
 */
	inp::MeshData energyMesh;
	std::array<inp::MeshData, 3> spatialMeshes;
	std::array<std::string, 3> spatialChars{"r", "a", "z"};  // spatialMeshes[i]に対応する*-typeの文字

	if(!sourceSection.extension().empty()) {
		src::phits::readPhitsExtendedParameters(sourceSection.extension(),
												PhitsSource::acceptableExtendedParams(),
												PhitsCylinderSource::essentialExtendedKeywords(),
												energyDistribution,
												spatialDistributions,
												spatialChars,
												&energyMesh, &spatialMeshes);
	}
//	mDebug() << "R-distribution =" << spatialDistributions_.at(0)->toString();
//	mDebug() << "Th-distribution =" << spatialDistributions_.at(1)->toString();
//	mDebug() << "Z-distribution =" << spatialDistributions_.at(2)->toString();

	if(sourceSection.extension().empty()) {
		mWarning(sourceSection.input().front().pos()) << "No source extension found.";
		return std::unique_ptr<src::PhitsCylinderSource>();
	}
	// ここでざっくりした入力データから内容を作ってコンストラクタに渡す。コンストラクタは論理的に意味のわかる値を受け付けるようにする。
	return std::unique_ptr<src::PhitsCylinderSource>(
			new PhitsCylinderSource(sourceInput.front(),
									parameterMap,
									phys::strToParticleType(parameterMap.at("proj")),  // 粒子
									factor,  // factor
									origin, // baseCenter
									bases[0], bases[1], bases[2],  // xyz方向基底ベクトル
									referenceDir,  // refDir
									energyDistribution,  // エネルギー分布
									angularDistribution, // 粒子角度分布
									spatialDistributions, // 空間分布
									acceptableCells,      // 線源セル
									energyMesh,          // 拡張入力エネルギー離散化メッシュ
									spatialMeshes        //  拡張入力空間離散化メッシュ
									)
			);

}

const std::vector<std::string> &src::PhitsCylinderSource::acceptableExtendedKeywords()
{
	static const std::vector<std::string> acParams{"e-type", "r-type", "a-type", "z-type"};
	return acParams;
}

const std::vector<std::string> &src::PhitsCylinderSource::essentialExtendedKeywords()
{
	static const std::vector<std::string> essentialVec{"e-type", "r-type", "a-type", "z-type"};
	return essentialVec;
}

const std::unordered_map<std::string, std::string> &src::PhitsCylinderSource::defaultParameterMap() {
	static auto initFunc = []() {
		std::unordered_map<std::string, std::string> map1(PhitsSource::defaultParameterMap());
		map1.emplace("x0", "0");
		map1.emplace("y0", "0");
		map1.emplace("r1", "0");
		map1.emplace("phi", "");
		map1.emplace("dom", "0");
		return map1;
	};
	static std::unordered_map<std::string, std::string> defMap = initFunc();
	return defMap;
}

const std::vector<std::string> &src::PhitsCylinderSource::essentialKeywords() {
	static std::vector<std::string> keywords = utils::makeConcatVector(PhitsSource::essentialKeywords(),
																	   std::vector<std::string> {"z0", "z1", "r0", "dir"});
	return keywords;
}

const std::vector<std::pair<std::string, std::string> > &src::PhitsCylinderSource::exclusiveKeywords() {
	static const std::vector<std::pair<std::string, std::string>> keywords = PhitsSource::exclusiveKeywords();
	return keywords;
}

