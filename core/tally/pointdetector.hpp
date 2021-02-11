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
#ifndef POINTDETECTOR_HPP
#define POINTDETECTOR_HPP

#include <list>
#include <unordered_map>

#include "pktally.hpp"
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"


namespace inp {
namespace phits {
class PhitsInputSection;
}
}

namespace tal {




class PointDetector : public PkTally
{
public:
	PointDetector(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
				  const inp::phits::PhitsInputSection &inputList);
	const std::vector<math::Point> &pointVectors() const {return detectionPoints_;}
	std::string toString() const;

private:

};


} // end namespace tal
#endif // POINTDETECTOR_HPP
