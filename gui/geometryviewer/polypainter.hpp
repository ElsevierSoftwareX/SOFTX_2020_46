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
#ifndef POLYPAINTER_HPP
#define POLYPAINTER_HPP

#include <atomic>
#include <exception>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkIdList.h>
#include <vtkLookupTable.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include "cellobject.hpp"
#include "../core/fielddata/fieldcolordata.hpp"
#include "../core/fielddata/xyzmeshtallydata.hpp"
#include "../core/image/cellcolorpalette.hpp"
#include "../core/utils/progress_utils.hpp"
#include "../core/utils/workerinterface.hpp"

struct OperationInfo;  // 先行宣言


template<>
struct WorkerTypeTraits<class PolyPainter> {
	// セル名とvtkActorとrefサンプル点数,実サンプル点数のタプルをvectorに保存
  typedef std::vector<std::tuple<std::string, vtkSmartPointer<vtkActor>, int, int>> result_type;
};

class PolyPainter : public WorkerInterface<PolyPainter>
{
public:
	// return_typeは更新したセル名のvector
	typedef WorkerTypeTraits<PolyPainter>::result_type result_type;

	// tupleはセル名、plydata, refサンプル点数、実サンプル点数
	PolyPainter(const std::vector<std::tuple<std::string, vtkSmartPointer<vtkPolyData>, int, int>> &polyDataTuples,
				const std::unordered_map<std::string, CellObject> *cellObjMap,
				const img::CellColorPalette &palette,
				const std::shared_ptr<const fd::FieldColorData> &fcData,
				const vtkSmartPointer<vtkLookupTable> &lookupTable
				)
		: polyDataTuples_(polyDataTuples),
		  cellObjMap_(cellObjMap),
		  palette_(palette),
		 fcData_(fcData), lookupTable_(lookupTable)
    {
        lookupTable_->SetNanColor(1.0, 1.0, 1.0, 1.0);
    }
	void impl_operation(size_t i, int threadNumber, result_type *thisThreadResult);

	static OperationInfo info();
	static result_type collect(std::vector<result_type> *resultVec);

private:
	// 生成済みポリゴンデータ
	std::vector<std::tuple<std::string, vtkSmartPointer<vtkPolyData>, int, int>> polyDataTuples_;
	// actorマップ
//	const std::unordered_map<std::string, std::pair<vtkSmartPointer<vtkActor>, int>> *actorMap_;
	const std::unordered_map<std::string, CellObject> *cellObjMap_;

	// 着色用パレット(スカラデータが無い場合はこちらが適用される)
	const img::CellColorPalette &palette_;
	// 着色用のスカラデータ
	const std::shared_ptr<const fd::FieldColorData> fcData_;
	const vtkSmartPointer<vtkLookupTable> &lookupTable_;

};

#endif // POLYPAINTER_HPP
