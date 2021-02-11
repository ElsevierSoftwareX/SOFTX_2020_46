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
#ifndef BBCALCULATOR_HPP
#define BBCALCULATOR_HPP

#include <atomic>
#include <exception>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "../../core/option/config.hpp"

namespace geom{
class BoundingBox;
class Cell;
}

struct OperationInfo;



class BBCalculator {
public:
//	typedef std::vector<geom::BoundingBox> result_type;
	typedef std::unordered_map<std::string, geom::BoundingBox> result_type;
	static result_type collect(std::vector<result_type> *resultVec);
	static OperationInfo info();

	BBCalculator(const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cmap,
				 const conf::Config &conf);
	void operator()(std::atomic_size_t *counter, std::atomic_bool *stopFlag, size_t threadNumber,
					size_t startIndex, size_t endIndex,
                    result_type *thisThreadResult, std::exception_ptr *ep, bool quiet);
private:
	std::vector<std::shared_ptr<const geom::Cell>> cells_;
	conf::Config cuiConfig_;


};

#endif // BBCALCULATOR_HPP
