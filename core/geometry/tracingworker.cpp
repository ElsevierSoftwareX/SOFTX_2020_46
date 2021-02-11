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
#include "tracingworker.hpp"


OperationInfo TracingWorker::info() {
#ifdef ENABLE_GUI
	std::string title = QObject::tr("Progress").toStdString();
	std::string operatingText = QObject::tr("Tracing section ").toStdString();
	std::string cancelingText = QObject::tr("Waiting for the subthreads to finish ").toStdString();
	std::string cbuttonLabel = QObject::tr("Cancel").toStdString();
#else
	std::string title = "Progress";
	std::string operatingText = "Tracing section ";
	std::string cancelingText ="Waiting for the subthreads to finish ";
	std::string cbuttonLabel = "";
#endif
	return OperationInfo(title, operatingText, cancelingText, cbuttonLabel);
}

TracingWorker::result_type TracingWorker::collect(std::vector<TracingWorker::result_type> *results){
	/*・vector<img::TracingRayData>を集計する。*/
	return collectVector<result_type>(results);
}
