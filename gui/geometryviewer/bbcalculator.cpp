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
#include "bbcalculator.hpp"

#include <QObject>
#include <QString>

#include "../../core/geometry/cell/cell.hpp"
#include "../../core/utils/progress_utils.hpp"

BBCalculator::result_type BBCalculator::collect(std::vector<BBCalculator::result_type> *resultVec)
{
	result_type results;
	for(auto &eachResult: *resultVec) {
//		results.insert(results.end(),
//					   std::make_move_iterator(eachResult.begin()),
//					   std::make_move_iterator(eachResult.end()));
		for(auto &elem: eachResult) results.emplace(std::move(elem));
	}
	return results;
}

OperationInfo BBCalculator::info()
{
	std::string title = QObject::tr("Progress").toStdString();
	std::string operatingText = QObject::tr("Calculating bounding boxes ").toStdString();
	std::string cancelingText = QObject::tr("Waiting for the subthreads to finish").toStdString();
	std::string cbuttonLabel = QObject::tr("Cancel").toStdString();
	return OperationInfo(title, operatingText, cancelingText, cbuttonLabel);
}

BBCalculator::BBCalculator(const std::unordered_map<std::string, std::shared_ptr<const geom::Cell> > &cmap,
						   const conf::Config &conf)
	: cuiConfig_(conf)
{
	cells_.reserve(cmap.size());
	for(const auto& cpair: cmap) {
		cells_.emplace_back(cpair.second);
	}
}

constexpr size_t DEFAULT_TIMEOUT_MSEC = 2000;

void BBCalculator::operator()(std::atomic_size_t *counter, std::atomic_bool *stopFlag, size_t threadNumber,
							  size_t startIndex, size_t endIndex,
							  BBCalculator::result_type *thisThreadResult,
							  std::exception_ptr *ep, bool quiet)
{

	(void) threadNumber;
	assert(startIndex <= endIndex);
	std::atomic_size_t localCounter(0);
	auto tID = std::this_thread::get_id();
	thisThreadResult->clear();
	thisThreadResult->reserve(endIndex - startIndex);
	try {
		for(size_t i = startIndex; i < endIndex; ++i) {
			thisThreadResult->emplace(cells_.at(i)->cellName(), cells_.at(i)->boundingBox(static_cast<size_t>(cuiConfig_.timeoutBB)));
			localCounter++;
			(*counter)++;
            if(stopFlag->load()) {
                if(!quiet) mDebug() << "tID =" << tID << ": BB calculation canceled.";
				// stopFlagがtrueになっている場合はprogressがキャンセルされているので
				// progressのためにcounterの帳尻を合わせる必要は無いが、念の為。
				std::atomic_fetch_add(counter, endIndex - startIndex - localCounter);
				return;
			}
		}
	} catch(...) {
		*ep = std::current_exception();
		try {
			std::rethrow_exception(*ep);
		} catch (std::exception &e) {
            if(!quiet) mDebug() << "worker exception. tID=" << tID << "what=" << e.what();
		}
		// 帳尻を合わせる。
		std::atomic_fetch_add(counter, endIndex - startIndex - localCounter);
	}
}
