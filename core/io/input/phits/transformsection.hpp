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
#ifndef TRANSFORMSECTION_HPP
#define TRANSFORMSECTION_HPP

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include "core/math/nmatrix.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/common/trcard.hpp"
#include "phitsinputsection.hpp"

namespace inp {
namespace phits {

class TransformSection: public PhitsInputSection
{
	template <class KEY, class T>	using map_type =std::unordered_map<KEY, std::shared_ptr<T>>;
public:
    TransformSection(const std::string &name,
					 const std::list<DataLine>& trDataLines,
					 bool verbose, bool warnPhitsCompat);

	// 変換行列マップ(mcnpに非依存形式)を返す TRカード数
	std::unordered_map<size_t, math::Matrix<4>> tranformMatrixMap() const;

private:
	map_type<size_t, comm::TrCard> trCards_;
};


}
}

#endif // TRANSFORMSECTION_HPP
