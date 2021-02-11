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
#ifndef TALLY_HPP
#define TALLY_HPP

#include <fstream>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "core/io/input/dataline.hpp"
#include "core/io/input/inputdata.hpp"
#include "core/io/input/phits/phitsinput.hpp"
#include "core/io/input/meshdata.hpp"
#include "core/io/input/phits/phitsinputsection.hpp"
#include "core/math/nmatrix.hpp"
#include "core/option/config.hpp"

namespace tal{

enum class TallyType{POINT, RING};
std::string tallyTypeString(TallyType tt);

// 粒子1個分の無衝突線束関連データ
struct UncollidedFluxData {
	UncollidedFluxData(double uf,
					   const std::vector<std::string> &cs,
					   const std::vector<double> &mfpTl)
		: uncollidedFlux(uf), passedCells(cs), mfpTrackLegths(mfpTl)
	{;}

	double uncollidedFlux;
	std::vector<std::string> passedCells;
	std::vector<double> mfpTrackLegths;
};

// タリー点1個あたりのデータをまとめた構造体
struct TallyPointData {
	TallyPointData(const math::Point &dp)
		: detectionPoint(dp)
	{;}
	void appendFluxData(const UncollidedFluxData &ufd)
	{
		fluxDataVec.emplace_back(ufd);
	}

	math::Point detectionPoint;
	std::vector<UncollidedFluxData> fluxDataVec;
};


// Point-Kernel tally
class PkTally
{
public:

	static std::vector<std::shared_ptr<PkTally>>
		createTallies(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
					  const inp::phits::PhitsInput &phitsInp,
					  const conf::Config &config);
	// tallyセクションからタリースマポを生成。 tallyは1セクション1タリーインスタンス
	static std::shared_ptr<PkTally>
		createTally(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
					const inp::phits::PhitsInputSection &tallySection);

	PkTally(const std::list<inp::DataLine> &dataList);
	//void addDose(std::size_t pindex, double dose);
	const std::vector<math::Point> &detectionPoints() const { return detectionPoints_;}
	// 線量を計算するための無衝突線束算結果とセル名、mfpをを受け取って線量を計算する。
	void appendTalliedData(const TallyPointData &tal) {talliedData.emplace_back(tal); }

	void dump(std::ostream& os);

protected:
	// Tallyの種類を記述する。point, ringn等
	TallyType type_;
	// 必須パラメータ
	std::list<std::string> essentialParams_;
	// 読み取ったパラメータを格納
	std::unordered_map<std::string, std::string> parameterMap_;
	// 排他利用パラメータpairのfirstとsecondは同時使用不可
	std::list<std::pair<std::string, std::string>> exclusiveParams_;

	// Eメッシュ（簡易コードでは使わない）
	inp::MeshData energyMesh_;

	// ここから実質使うデータ
	std::string title_;
	inp::DataLine startLine_;
	std::vector<math::Point> detectionPoints_;  // 検出点
	//std::vector<double> doses_;  // 線量
	std::vector<TallyPointData> talliedData;

	/*
	 * 線量は検出点ごとに計算するので、検出点、無衝突線束、線量等は
	 * 一つの構造体にまとめる
	 *
	 */

	// 必須パラメータが設定されているかチェック
	void checkEssentialParams();
	void checkExclusiveParams();
};


}  // end namespace tal

#endif // TALLY_HPP
