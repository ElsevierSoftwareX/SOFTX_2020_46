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
#ifndef PHITSCYLINDERSOURCE_HPP
#define PHITSCYLINDERSOURCE_HPP


#include <list>
#include <memory>
#include <unordered_map>

#include "phitssource.hpp"
#include "core/math/nvector.hpp"
#include "core/utils/container_utils.hpp"

namespace inp {
class DataLine;
namespace phits {
class PhitsInputSection;
}
}

namespace src {



class PhitsCylinderSource : public PhitsSource
{
public:
	PhitsCylinderSource(const inp::DataLine &startLine,
						const std::unordered_map<std::string, std::string> paramMap,
						phys::ParticleType pType,
						double factor,
						const math::Point localOrigin,
						const math::Vector<3> &xdir,
						const math::Vector<3> &ydir,
						const math::Vector<3> &zdir,
						const math::Vector<3> &refDir,
						std::shared_ptr<Distribution> eDistro,
						std::shared_ptr<Distribution> aDistro,
						std::array<std::shared_ptr<Distribution>, 3> sDistro,
						const std::vector<std::shared_ptr<const geom::Cell>> &acceptableCells,
						const inp::MeshData& emesh,
						const std::array<inp::MeshData, 3> spMeshes
						);

	// override必須な仮想関数
	std::string toString() const override;
	math::Point toCartesian(double r, double th, double z) const override;

	const math::Point &baseCenter() const {return localOrigin_;}
	static std::unique_ptr<PhitsCylinderSource>
		create(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
			  const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cellMap,
			  const inp::phits::PhitsInputSection &sourceSection, bool hasEnergyDistribution);

private:
	// 円筒下底中心座標を局所座標原点に採る
	//math::Point baseCenter_;
    // 円筒軸方向とreference方向は同じなので別途定義はしない。(refDirは基底で定義済み)
	math::Vector<3> xdirection_;  // xdirectionをθ=0方向とする
	math::Vector<3> ydirection_;  // ydireはxdirとrefDirから必要に応じて生成できるが、計算効率のためメンバ化
	math::Vector<3> zdirection_;


	static const std::vector<std::string> &acceptableExtendedKeywords();
	static const std::vector<std::string> &essentialExtendedKeywords();
	static const std::unordered_map<std::string, std::string> &defaultParameterMap();
	static const std::vector<std::pair<std::string, std::string>> &exclusiveKeywords();
	static const std::vector<std::string> &essentialKeywords();
};

}  // end namespace src
#endif // PHITSCYLINDERSOURCE_HPP
