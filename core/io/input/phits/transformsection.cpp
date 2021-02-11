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
#include "transformsection.hpp"

#include "core/io/input/mcmode.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include <regex>

inp::phits::TransformSection::TransformSection(const std::string &name,
											   const std::list<DataLine>& trDataLines, bool verbose, bool warnPhitsCompat)
	:PhitsInputSection(name, trDataLines, verbose, warnPhitsCompat)
{
    sectionName_ = inp::phits::toSectionString(Section::TRANSFORM);
	std::smatch sm;
    for(auto& element: input_) {
		try {
			// ここからTRカードパターン検索
			if(std::regex_search(element.data, sm, comm::TrCard::regex())) {
				comm::TrCard trCard(element.data);
				auto trNum = trCard.idNumber();
				if(trCards_.find(trNum) != trCards_.end()) {
					throw std::invalid_argument(std::string("Multiple definition of TR") + utils::toString(trNum));
				}
				trCards_[trNum] = std::make_shared<comm::TrCard>(trCard);
			} else {
				// TransformSectionで非TRカードが出たら警告して無視。
				std::cerr << "Warning! non-TR card found, ignored. "
						  << element.pos() << " " << element.data << std::endl;
			}
		} catch (std::exception& e) {
            throw std::invalid_argument(element.pos() + " " +  e.what());
		}
	}
}

std::unordered_map<size_t, math::Matrix<4> > inp::phits::TransformSection::tranformMatrixMap() const
{
	std::unordered_map<size_t, math::Matrix<4> > retMap;
	for(auto it = trCards_.begin(); it != trCards_.end(); it++) {
		// firstはカード番号  secondはshared_ptr<TrCard>
		retMap[it->first] = it->second->trMatrix();
	}
	return retMap;
}
