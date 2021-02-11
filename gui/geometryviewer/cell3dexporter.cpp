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
#include "cell3dexporter.hpp"

#include <QFileInfo>
#include <QString>

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkMapper.h>

#include "fileconverter.hpp"
#include "../../core/utils/message.hpp"
#include "../../core/utils/progress_utils.hpp"
#include "../../core/utils/system_utils.hpp"

namespace {

constexpr char UNIFIED_CELL_NAME[] = "unified";

}  // end anonymous namespace


OperationInfo Cell3DExporter::info()
{
	std::string title = QObject::tr("Progress").toStdString();
	std::string operatingText = QObject::tr("exporting cells in 3D format ").toStdString();
	std::string cancelingText = QObject::tr("Waiting for the subthreads to finish").toStdString();
	std::string cbuttonLabel = QObject::tr("Cancel").toStdString();
	return OperationInfo(title, operatingText, cancelingText, cbuttonLabel);
}

Cell3DExporter::result_type Cell3DExporter::collect(std::vector<Cell3DExporter::result_type> *resultVec)
{
	result_type results;
	// 結果の回収.move_iteratorを使う。
	for(auto &eachResult: *resultVec) {
		results.insert(results.end(),
					   std::make_move_iterator(eachResult.begin()),
					   std::make_move_iterator(eachResult.end()));
	}
	return results;
}

Cell3DExporter::Cell3DExporter(const ff::FORMAT3D &format, const QDir &dir,
							   const std::unordered_map<std::string, CellObject> &cellObjMap,
							   const std::vector<std::string> &cellNames,
							   double factor, bool unify)
	: format3D_(format), dir_(dir), cellObjMap_(cellObjMap),
	  targetCellNames_(cellNames), exFactor_(factor), unify_(unify)
{;}





// Workerの記述はこのstartIndex-endIndexループ以外の部分は共通化できるのではないか。
//  → WorkerInterfaceを継承してoperator()の代わりに1要素分の処理をimpl_operationで実装すれば良い。
// が、unify処理もこのクラスで実施したいのでそれは不可
void Cell3DExporter::operator()(std::atomic_size_t *counter, std::atomic_bool *stopFlag, size_t threadNumber,
								size_t startIndex, size_t endIndex,
								Cell3DExporter::result_type *thisThreadResult, std::exception_ptr *ep, bool quiet)
{
	(void) threadNumber;
    (void) quiet;
	assert(startIndex <= endIndex);
	auto tID = std::this_thread::get_id();
	size_t localCounter = 0;
	try {
		if(unify_) {
			// 一体化版は全セルを一体化させてエクスポートする。
			auto appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();
			for(const auto& cellName: targetCellNames_) {
				// vtkAppendPolyData::AddInputDataの引数はvtkDataObject*だから多分そのままvtkPolyData*で行け
				if(stopFlag->load()) return;
				appendFilter->AddInputData(vtkPolyData::SafeDownCast(cellObjMap_.at(cellName).actor_->GetMapper()->GetInput()));
				++localCounter;
				++(*counter);
			}

			// Remove any duplicate points.
			auto cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
			cleanFilter->SetInputConnection(appendFilter->GetOutputPort());
			cleanFilter->Update();

			//Create a mapper and actor
			auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
			mapper->SetInputConnection(cleanFilter->GetOutputPort());

			auto polys = vtkPolyData::SafeDownCast(mapper->GetInput());
			QFileInfo finfo(dir_, QString(UNIFIED_CELL_NAME));
			std::string outputFileName = finfo.absoluteFilePath().toStdString();
			ff::exportFile(format3D_, utils::utf8ToSystemEncoding(outputFileName), polys, exFactor_, true);
			thisThreadResult->emplace_back(UNIFIED_CELL_NAME);

		} else {
			// 個別セル版
			for(size_t i = startIndex; i < endIndex; ++i) {
				// 正味の処理はここだけ。
				const std::string &cellName = targetCellNames_.at(i);
				QFileInfo finfo(dir_, QString::fromStdString(utils::toValidEachFileName(cellName)));
				auto polyData = vtkPolyData::SafeDownCast(cellObjMap_.at(cellName).actor_->GetMapper()->GetInput());
				//std::string outputFileName = ff::get3DFileName(finfo.absoluteFilePath().toStdString(), fformat);
				std::string outputFileName = finfo.absoluteFilePath().toStdString();
				if(stopFlag->load()) return;
				ff::exportFile(format3D_, utils::utf8ToSystemEncoding(outputFileName), polyData, exFactor_, true);

				++localCounter;
				++(*counter);
				thisThreadResult->emplace_back(cellName);
			}
		}
	} catch (...) {
		/*
		 * 例外が発生したら
		 * 1. メインスレッドに伝えるためにepに代入
		 * 2．counterを残り処理分だけ増加（counterを監視しているprogressが正常にループを抜けるように。ループを抜けないとjoinできない）
		 * 3．リターン
		 */
		*ep = std::current_exception();
		mDebug() << "worker exception. tID=" << tID << ",what=" << what(*ep);
		// counterの帳尻合わせ. localCounter個の処理が終わったところでエラー発生したので、
		// 残りの数だけcounterを増やして帳尻を合わせる。
		std::atomic_fetch_add(counter, endIndex - startIndex - localCounter);
	}

	mDebug() << "cell exporting finished. tID=" << tID;
}
