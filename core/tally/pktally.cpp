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
#include "pktally.hpp"

#include <map>
#include <stdexcept>

#include "core/io/input/mcmode.hpp"
#include "pointdetector.hpp"

std::string tal::tallyTypeString(tal::TallyType tt)
{
	static std::map<tal::TallyType, std::string> ttmap
	{
		{TallyType::POINT, "point"},
		{TallyType::RING, "ring"}
	};
	return ttmap.at(tt);
}




std::vector<std::shared_ptr<tal::PkTally>>
	tal::PkTally::createTallies(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
								const inp::phits::PhitsInput &phitsInp,
								const conf::Config &config)
{
	namespace ip = inp::phits;
	std::vector<std::shared_ptr<tal::PkTally>> tallies;
	auto tallyInputLists = phitsInp.tallySections();
	if(tallyInputLists.empty()) {
		mWarning() << "No tally(t-point) section found.";
	} else {
		for(auto & tallyInput: tallyInputLists) {
			ip::PhitsInputSection tallySection(ip::toSectionString(ip::Section::T_POINT), tallyInput,
											   config.verbose, config.warnPhitsIncompatible);
			tallies.emplace_back(tal::PkTally::createTally(trMap, tallySection));
		}
	}
	return tallies;
}

std::shared_ptr<tal::PkTally> tal::PkTally::createTally(
		const std::unordered_map<size_t, math::Matrix<4>> &trMap,
		const inp::phits::PhitsInputSection &tallySection)
{
	namespace ip = inp::phits;
	const std::string name = tallySection.sectionName();
    if(name != ip::toSectionString(ip::Section::T_POINT)) throw std::invalid_argument("Tally section name should be t-point");

	return std::make_shared<PointDetector>(trMap, tallySection);
}

// Tallyクラスではタリー共通必須パラメータ,デフォルトパラメータの設定を行う。
// 、がそんなものは殆ど無い。mesh定義とpartくらい。
tal::PkTally::PkTally(const std::list<inp::DataLine> &dataList)
{
	if(dataList.empty()) {
		throw std::invalid_argument("Argument is empty");
	}
	parameterMap_["factor"] = "1.0";
	essentialParams_.emplace_back("part");
	exclusiveParams_.emplace_back(std::make_pair("point", "ring"));
	startLine_ = dataList.front();
}

void tal::PkTally::dump(std::ostream &os)
{
	os << "Tally title =" << title_ << std::endl;
	for(auto &td: talliedData) {
		os << "Point = " << td.detectionPoint << std::endl;
		for(size_t i = 0; i < td.fluxDataVec.size(); ++i) {
			UncollidedFluxData fd = td.fluxDataVec.at(i);
			os << "particle " << i << " uncollided flux =";
			os << std::scientific << fd.uncollidedFlux
			   << " cells =" << fd.passedCells << ", tl(mfp)=" << fd.mfpTrackLegths<< std::endl;
		}
	}
}

void tal::PkTally::checkEssentialParams()
{
	for(auto &param: essentialParams_) {
		if(parameterMap_.find(param) == parameterMap_.end()) {
			throw std::invalid_argument(std::string("Essential parameter ") + param + " is not set");
		}
	}
}

void tal::PkTally::checkExclusiveParams()
{
	for(auto &exPair: exclusiveParams_) {
		if(parameterMap_.find(exPair.first) != parameterMap_.end()
		&& parameterMap_.find(exPair.second) != parameterMap_.end()) {
			throw std::invalid_argument(std::string("Parameter ") + exPair.first
										+ " and " + exPair.second + " is exclusive");
		}
	}
}
	// NOTE タリーの記述で意味のないパラメータ入力をエラーにする。
