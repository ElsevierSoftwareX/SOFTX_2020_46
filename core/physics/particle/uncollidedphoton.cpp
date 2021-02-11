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
#include "uncollidedphoton.hpp"

#include <numeric>
#include "core/utils/message.hpp"


phys::UncollidedPhoton::UncollidedPhoton(double w,
										 const math::Point &p,
										 const math::Vector<3> &v,
										 const double &e,
										 const geom::Cell *c,
										 const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cellList,
										 double maxLength,
										 bool recordEvent,
										 bool guessStrict)
	: TracingParticle(w, p, v, e, c, cellList, maxLength, recordEvent, guessStrict)
{

}

void phys::UncollidedPhoton::trace() noexcept
{
	double decayFactor = 1;
	while(!expired()) {
		try {
			moveToBound();  // ここでtrackLengths_が更新される
			double macroXs = currentCell_->macroTotalXs(ptype_, energy_);
			mfpTrackLengths_.emplace_back(trackLengths_.back()*macroXs);
			// 指数減衰
			decayFactor = std::exp(-trackLengths_.back()*macroXs);
			mDebug() << "length=" << trackLengths_.back() << "totXs=" <<  macroXs << "factor=" << decayFactor;
			weight_ = weight_*decayFactor;


			if(recordEvent_) {
				events_.emplace_back(createEventRecord("Decay by collision",
													   std::string("new w=") +utils::toString(weight_) +
													   ",fac=" + utils::toString(decayFactor)) );
			}
		} catch (std::exception & e) {
			std::cerr << "Exception occured!" << std::endl;
			std::cerr << e.what();
			abort();
		}
		enterCellTr();
	}
}

double phys::UncollidedPhoton::uncollidedFlux() const
{
	double totalDistance = std::accumulate(trackLengths_.begin(), trackLengths_.end(), 0.0);
	assert(totalDistance > 0);
	if(totalDistance < math::EPS) {
		mWarning() << "total track length is zero. Particle tracking didn't executed or was invalid.";
		return 0;
	}
	// 立体角因子1/4piは粒子生成時にweight_に考慮済み
	return weight_/(totalDistance*totalDistance);
}
