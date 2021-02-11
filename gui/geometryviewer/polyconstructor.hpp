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
#ifndef POLYCONSTRUCTOR_HPP
#define POLYCONSTRUCTOR_HPP

#include <array>
#include <atomic>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>

#include <QProgressDialog>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>

#include "geometryviewerconfig.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/fielddata/fieldcolordata.hpp"
#include "core/utils/progress_utils.hpp"
#include "core/utils/workerinterface.hpp"
#include "cellobject.hpp"



template<>
struct WorkerTypeTraits<class PolyConstructor> {
	// セル名、polyData, サンプル点数,のタプルのベクトルを返す。
	typedef std::vector< std::tuple<std::string, vtkSmartPointer<vtkPolyData>, int, int> > result_type;
};

// ポリゴン生成クラス
class PolyConstructor: public WorkerInterface<PolyConstructor>
{
	// セル名と(actorスマポ、サンプル点数、)のマップ
	typedef std::unordered_map<std::string, std::pair<vtkSmartPointer<vtkActor>, int>> ActorPairMap_type;
	typedef std::unordered_map<std::string, geom::BoundingBox> BBMap_type;
	typedef std::array<double, 6> RangeArray_type;
	typedef std::unordered_map<std::string, vtkSmartPointer<vtkImplicitFunction>> ImplicitFuncMap_type;

public:
	typedef WorkerTypeTraits<PolyConstructor>::result_type result_type;

	PolyConstructor(int numThreads, const std::vector<std::string> &cellNames,
					const std::unordered_map<std::string, CellObject> &cellObjMap,
					const GeometryViewerConfig &gconf);

	void impl_operation(size_t i, int threadNumber, result_type *results);

	// vtkで陰関数をサンプリングする時の返り値のデータサイズ（格子点あたりのデータサイズbyte/ptsB）
	static const int SAMPLE_ELEMENT_SIZE_BYTE = 8;
	static OperationInfo defaultInfo();
	static result_type collect(std::vector<result_type> *resultVec);
private:
	// 実行スレッド数
	const int numThreads_;
	// 更新するセルの名前リスト
	const std::vector<std::string> &cellNames_;
//	// セル名をキーにしたvtkActorのマップ
//	const ActorPairMap_type &actorMap_;

	// 可視化領域[-x x, -y, y, -z, z]を格納したアレイ
	const std::array<double, 6> &currentSamplingRange_;

	// セル名とセルに関連したデータ一式を格納するマップ, BB, 陰関数を統合
	const std::unordered_map<std::string, CellObject> &cellObjMap_;

	// 可視化補助平面の情報クラス
	//const AuxPlaneInfo &auxPlaneInfo_;
	//const std::array<PlaneInfo, 3> auxPlaneInfo_;
	const GeometryViewerConfig geomConfig_;  	// 基準サンプリング点数はここに含まれる
};


#endif // POLYCONSTRUCTOR_HPP
