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
#ifndef TRACINGWORKER_HPP
#define TRACINGWORKER_HPP

#include <atomic>
#include <exception>
#include <unordered_map>

#include "core/utils/progress_utils.hpp"
#include "core/utils/workerinterface.hpp"
#include "core/image/tracingraydata.hpp"
#include "core/physics/particle/tracingparticle.hpp"
#include "cell/cell.hpp"
#include "core/math/nvector.hpp"


template<> struct WorkerTypeTraits<class TracingWorker> {
  typedef std::vector<img::TracingRayData> result_type;
};

// [startIndex, endIndex)の範囲のTracingを実行する
class TracingWorker: public WorkerInterface<TracingWorker>
{
public:
	typedef WorkerTypeTraits<TracingWorker>::result_type result_type;
	static OperationInfo info();
	static result_type collect(std::vector<result_type> *results);

	TracingWorker(const math::Point &origin,
				  const math::Vector<3> &scanDirUnitVec,
				  double scanLength,
				  const math::Vector<3> subScanDirUnitVec,
				  double subScanPitch,
				  bool recordEvent,
				  const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cellMap)
		: origin_(origin), scanDirUnitVec_(scanDirUnitVec), scanLength_(scanLength),
		  subScanDirUnitVec_(subScanDirUnitVec), subScanPitch_(subScanPitch), recordEvent_(recordEvent),
		  cellMap_(cellMap)
	{;}

	void impl_operation(size_t i, int threadNumber, result_type* resultRays)
	{
		(void) threadNumber;
		math::Point rayOrigin;
		// 境界と一致するのを防ぐために操作方向に0.001%のオフセットをつける
		rayOrigin = origin_ + (i+0.5)*subScanPitch_*subScanDirUnitVec_ - 0.00001*scanDirUnitVec_;
		// if(recordEvent) {
		//	mDebug() << "index=" << i << "start=" << rayOrigin << "dir=" << scanDirUnitVec;
		//	}
		phys::TracingParticle p1(1.0, rayOrigin, scanDirUnitVec_, 0, nullptr, cellMap_, scanLength_, recordEvent_, false);
		p1.trace();

		//if(recordEvent) {
		//	mDebug() << "index=" << i << "start=" << rayOrigin;
		//	mDebug() << "cells=" << p1.passedCells();
		//	mDebug() << "tlengths=" << p1.trackLengths();
		//	p1.dumpEvents(std::cout);

		//	if(p1.passedCells().size() != p1.trackLengths().size()) {
		//		mDebug() << "Size of cells_ and tracklengths_ is different";
		//		abort();
		//	}
		//}

		resultRays->emplace_back(img::TracingRayData(rayOrigin, i, p1.passedCells(), p1.trackLengths(),
													 geom::Cell::UNDEF_CELL_NAME, geom::Cell::UBOUND_CELL_NAME, geom::Cell::BOUND_CELL_NAME));

		if(recordEvent_) {
			mDebug() << "Recording EVENT!!!!";
			mDebug() << "i=" << i << "ray=" << p1.passedCells() << p1.trackLengths();
			p1.dumpEvents(std::cout);
		}

	}

private:
	math::Point origin_;
	math::Vector<3> scanDirUnitVec_;
	double scanLength_;
	math::Vector<3> subScanDirUnitVec_;
	double subScanPitch_;
	bool recordEvent_;
	const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cellMap_;
};


#endif // TRACINGWORKER_HPP
