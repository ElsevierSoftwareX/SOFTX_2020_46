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
#ifndef UNCOLLIDEDPHOTON_HPP
#define UNCOLLIDEDPHOTON_HPP

#include <memory>
#include <unordered_map>

#include "tracingparticle.hpp"
#include "core/math/nvector.hpp"
#include "core/physics/physconstants.hpp"

namespace geom {
class Cell;
}

namespace phys{

class UncollidedPhoton : public phys::TracingParticle
{
public:
	UncollidedPhoton(double w,
					 const math::Point &p,
					 const math::Vector<3> &v,
					 const double& e,
					 const geom::Cell *c,  // ナマポ版
					 const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>>& cellList,
					 double maxLength,
					 bool recordEvent = false,
					 bool guessStrict = false);

	void trace() noexcept;
	// trace後にuncollideFluxを計算する。
	double uncollidedFlux() const;
	std::vector<double> mfpTrackLengths() const {return mfpTrackLengths_;}  // MFP化したtrackLengths()を返す

private:
	const ParticleType ptype_ = ParticleType::PHOTON;
	std::vector<double> mfpTrackLengths_;
};


}  // end namespace phys
#endif // UNCOLLIDEDPHOTON_HPP
