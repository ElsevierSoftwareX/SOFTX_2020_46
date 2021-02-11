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
#include "pixelmergingworker.hpp"



OperationInfo PixelMergingWorker::info()
{
#ifdef ENABLE_GUI
	std::string title = QObject::tr("Progress").toStdString();
	std::string operatingText = QObject::tr("Merging ray data ").toStdString();
	std::string cancelingText = QObject::tr("Waiting for the subthreads to finish").toStdString();
	std::string cbuttonLabel = QObject::tr("Cancel").toStdString();
#else
	std::string title = "Progress";
	std::string operatingText = "Merging ray data ";
	std::string cancelingText ="Waiting for the subthreads to finish";
	std::string cbuttonLabel = "";
#endif
	return OperationInfo(title, operatingText, cancelingText, cbuttonLabel);
}

PixelMergingWorker::result_type PixelMergingWorker::collect(std::vector<PixelMergingWorker::result_type> *results)
{
	/*
	 * PixelMergingWorkerでは元の全体配列をvertical方向に分割しているので、
	 * collect関数ではv方向に連結していく。
	 *
	 * result_type はimg::PixelArray
	 */
	result_type pixelArray;
	for(auto &parray: *results) {
        pixelArray.hMoveConcat(&parray);
	}
    return pixelArray;
}
