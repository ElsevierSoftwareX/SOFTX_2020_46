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
#include "polypainter.hpp"

#include <QObject>

#include <vtkLookupTable.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkScalarsToColors.h>


#include "../../core/utils/progress_utils.hpp"

/*
 * Actorの更新と着色を行う。
 * polyDataTuplesに（名前が）含まれているセルは全て着色対象。
 * 前回の着色方法を保持していないので、現状似合うようにmapperは全てnewで新規作成する。
 */
void PolyPainter::impl_operation(size_t i, int threadNumber, PolyPainter::result_type *thisThreadResult)
{
	(void) threadNumber;
    //auto tID = std::this_thread::get_id();


//	// Debug用
//	if (i >= polyDataTuples_.size()) {
//		mDebug() << "index ===" << i << "is out of range for polyDataTuples_, size===" << polyDataTuples_.size() << "tID===" << tID;
//	}
	const std::string &cellName = std::get<0>(polyDataTuples_.at(i));
	vtkSmartPointer<vtkPolyData> &polyData = std::get<1>(polyDataTuples_.at(i));


	// ポリゴンデータが未更新ならpolyDataはnullptrであることに注意。

	// 着色用fielddataを作成
	if(fcData_ && polyData != nullptr) {
		size_t numPolys = polyData->GetNumberOfPolys();
		auto  scalars = vtkSmartPointer<vtkFloatArray>::New();
		scalars->SetNumberOfValues(numPolys);
		scalars->SetName("flux");
		polyData->GetPolys()->InitTraversal();
		auto idList = vtkSmartPointer<vtkIdList>::New();
		double *center;
		double x, y, z, factor;
		vtkIdType cellId;
		while(polyData->GetPolys()->GetNextCell(idList)) {
			x = 0; y = 0; z = 0;
			factor = 1.0/idList->GetNumberOfIds();
			// idListにはセル(三角形ポリゴン)の頂点IDが入っている。のでそこから座標を取得する。
			for(vtkIdType i = 0; i < idList->GetNumberOfIds(); ++i) {
				center = polyData->GetPoint(idList->GetId(i));
				x += center[0];
				y += center[1];
				z += center[2];
			}
			x *= factor;
			y *= factor;
			z *= factor;
			cellId = polyData->GetPolys()->GetTraversalLocation(idList->GetNumberOfIds());
			// ポリゴン中心のxyz位置がわかったのでその位置でのfieldデータを取得する。
            double val = fcData_->meshData_->getValue(x, y, z, fcData_->interpolation_, fcData_->isLog_);
			if(std::abs(val) <= fd::FieldColorData::SMALL_FIELD_DATA) {val = fd::FieldColorData::SMALL_FIELD_DATA;}
			if(fcData_->isLog_) val = std::log10(val);
			scalars->SetValue(cellId/4, val);
		}
		//	polyData->GetPointData()->SetScalars(scalars); // うまくいかない
		polyData->GetCellData()->SetScalars(scalars);  // これでfieldデータのセット完了
	}

	vtkSmartPointer<vtkActor> targetActor = nullptr;
	auto it0 = cellObjMap_->find(cellName);
//	if(polyData == nullptr && actorMap_->find(cellName) != actorMap_->end()) {
	if(polyData == nullptr && it0 != cellObjMap_->end() && it0->second.actor_ != nullptr) {

		// ポリゴンデータを更新しておらず、actorMapにエントリがある場合はそれを使う
		targetActor = it0->second.actor_;
	} else {
		targetActor = vtkSmartPointer<vtkActor>::New();
	}

	// 前回の着色モード(単色 or fielddata)は記憶されていないのでmapperは毎回更新する。
	auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData(polyData);
	if(fcData_) {
		mapper->ScalarVisibilityOn();
		mapper->SetLookupTable(lookupTable_);
		mapper->SetScalarModeToUseCellData(); // スカラーモードの選択
		mapper->SetColorModeToMapScalars();  // カラーモードの選択。 lookupテーブルを参照する。
        auto rang = mapper->GetLookupTable()->GetRange();
        // 一見意味がなさそうだが、明示的にsetScalarRangeを呼ばないとレンジはセットされない。(lookuptableがセット済みでも)
        mapper->SetScalarRange(rang[0], rang[1]);

	} else {
		mapper->ScalarVisibilityOff();
	}
	targetActor->SetMapper(mapper);


	// ここまででtargetActorが確定する。
    auto color = palette_.getColorByCellName(cellName);
    // NOTE セルのActorへのvtkPropertyによる着色はここ。
	// ちなみにScalarVisibilityがonならここで色をセットしても反映されないので
	if(!fcData_ && targetActor != nullptr) {
        targetActor->GetProperty()->SetColor(static_cast<double>(color->r)/255,
                                             static_cast<double>(color->g)/255,
                                             static_cast<double>(color->b)/255);
	}

	int numRefPoints = std::get<2>(polyDataTuples_.at(i));
	int numRealPoints = std::get<3>(polyDataTuples_.at(i));
	thisThreadResult->emplace_back(std::make_tuple(cellName, targetActor, numRefPoints, numRealPoints));

}

OperationInfo PolyPainter::info()
{
	std::string title = QObject::tr("Progress").toStdString();
	std::string operatingText = QObject::tr("Updating polygon color ").toStdString();
	std::string cancelingText = QObject::tr("Waiting for the subthreads to finish").toStdString();
	std::string cbuttonLabel = QObject::tr("Cancel").toStdString();
	return OperationInfo(title, operatingText, cancelingText, cbuttonLabel);
}

PolyPainter::result_type PolyPainter::collect(std::vector<PolyPainter::result_type> *resultVec)
{
	return collectVector<PolyPainter::result_type>(resultVec);
}
