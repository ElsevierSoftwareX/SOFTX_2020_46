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
#include "pointdetector.hpp"


#include <regex>

#include "core/io/input/phits/phits_metacards.hpp"
#include "core/io/input/phits/phitsinputsection.hpp"
#include "core/io/input/phits/phitsinputsubsection.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/meshdata.hpp"
#include "core/source/phits/phitssource.hpp"
#include "core/utils/utils.hpp"
#include "core/math/constants.hpp"

tal::PointDetector::PointDetector(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
								  const  inp::phits::PhitsInputSection &inputList)
	:PkTally(inputList.input())
{
	namespace ip = inp::phits;
	std::smatch sm;
	ip::PhitsInputSubsection subsection;
	// 最初に標準phits入力を読み取る
	auto normalInput = inputList.input();
	for(auto it = normalInput.cbegin(); it != normalInput.cend(); ++it) {
		if(std::regex_search(it->data, sm, inp::phits::parameterPattern())) {
			auto paramValuePair = inp::phits::getParameterPair(it->data);
			auto paramName = paramValuePair.first, paramValue = paramValuePair.second;
			parameterMap_[paramName] = paramValue;


			// サブセクションを入力するパラメータ
			if(paramName == "point" || paramName == "ring") {
				// point/ringの後はパラメータ行ではなくサブセクションが続く
				++it;
				subsection = ip::PhitsInputSubsection(it, normalInput.cend(),
													 utils::stringTo<size_t>(paramValue));
				//mDebug() << "Tally subsection=" <<  subsection.toString();
				if(it != normalInput.cend() && !std::regex_search(it->data, sm, inp::phits::parameterPattern())) {
                    throw std::invalid_argument(it->pos() + " Number of ring/point and number of subsection entries are not different.");
				}
				--it;
			} else if(paramName == "e-type") {
				// e-typeの後もパラメータ行ではなくメッシュサブセクションが続く
				++it;
				energyMesh_ = inp::MeshData::fromPhitsInput(it, normalInput, inp::MeshData::KIND::E, paramValue);
			}
		} else {
			std::cerr << "Error:" << it->pos() << " \"" << it->data << "\" is not a valid tally parameter." << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}

	// チェック
	PkTally::checkEssentialParams();
	PkTally::checkExclusiveParams();
	type_ = (parameterMap_.find("point") != parameterMap_.end())
					? TallyType::POINT: TallyType::RING;

	// #### Point detectorの場合
	if(parameterMap_.find("point") != parameterMap_.end()) {
		for(auto &title: {"x", "y", "z"}) {
			if(!subsection.hasTitle(title)) {
				std::cerr << "Error: In tally section started from " << startLine_.pos() << std::endl;
				std::cerr << "Subsection should have entries named \"" << title << "\"" << std::endl;
				std::exit(EXIT_FAILURE);
			}
		}
		// 検出点を取得
		auto xvec = utils::stringVectorTo<double>(subsection.getValueVector("x"));
		auto yvec = utils::stringVectorTo<double>(subsection.getValueVector("y"));
		auto zvec = utils::stringVectorTo<double>(subsection.getValueVector("z"));
		for(size_t i = 0; i < subsection.numberOfParameters(); ++i) {
			// point の場合points_には1要素vectorを追加
			detectionPoints_.emplace_back(math::Point{xvec.at(i), yvec.at(i), zvec.at(i)});
		}
		//doses_.resize(detectionPoints_.size());
		//for(auto it = doses_.begin(); it != doses_.end(); ++it) *it = 0;

	// ### Ring detectorの場合
	} else if(parameterMap_.find("ring") != parameterMap_.end()) {
		std::cerr << "Error: Ring detector is not implemented yet." << std::endl;
		std::exit(EXIT_FAILURE);
	}

	// TRCLがある場合は座標変換
	if(parameterMap_.find("trcl") != parameterMap_.end()) {
		std::string trclStr = parameterMap_.at("trcl");
		try {
			auto matrix = utils::generateTransformMatrix(trMap, trclStr);
			for(auto &pt: detectionPoints_) {
				math::affineTransform<3>(&pt, matrix);
			}
		} catch (std::exception &e) {
			std::cerr << "Error: In tally section start from " << inputList.input().front().pos() << std::endl;
			std::cerr << e.what() << std::endl;
			std::cerr << "TRCL parameter is invalid(card input or TR number is wrong?)." << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}
}

std::string tal::PointDetector::toString() const
{
	std::stringstream ss;
	if(type_ == TallyType::POINT) {
		ss << tal::tallyTypeString(type_) << " detector tally, number of points = "<< detectionPoints_.size();
		for(size_t i = 0; i < detectionPoints_.size(); ++i) {
			ss << std::endl;
			ss << "Detector" << i << " point = " << detectionPoints_.at(i);
		}
		return ss.str();
	} else {
		std::cerr << "ProgramError: type of PD class should be point or ring." << std::endl;
		std::exit(EXIT_FAILURE);
	}
}
