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
#include "polyconstructor.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <future>
#include <thread>
#include <QApplication>
#include <QProgressDialog>

#include <vtkSampleFunction.h>
#include <vtkContourFilter.h>
#include <vtkMarchingCubes.h>
#include <vtkMapper.h>

#include <vtkFloatArray.h>  // 着色用テスト
#include <vtkPointData.h>
#include <vtkIdList.h>
#include <vtkCellData.h>

#include "core/fielddata/xyzmeshtallydata.hpp"
#include "core/fielddata/fieldcolordata.hpp"
#include "core/utils/message.hpp"
#include "core/utils/system_utils.hpp"
#include "colorpane.hpp"
#include "cellobject.hpp"

namespace {
// voxel体系であればlattice要素は単純であることが各日だが、燃料要素のように複雑形状の場合もあるので
// 一律に減らすのは良くない。advancedセッティングで変更可能にすべし。
constexpr double LATTICE_ELEM_REDUCTIO_FACTOR = 1.0; // latticeエレメントの解像度は過剰になりガチなのでこの因子(<1)を掛けて減らす
constexpr double OFFSET_FACTOR = 1.01;  // 陰関数サンプリングレンジはboundinbboxより少し大きくする

// このあたりはconfigのadvancedあたりに持っていきたい。
constexpr double MIN_SAMPLES_PER_DIRECTION = 5;
constexpr size_t  MIN_SAMPLE_POINTS = MIN_SAMPLES_PER_DIRECTION*MIN_SAMPLES_PER_DIRECTION*MIN_SAMPLES_PER_DIRECTION;
constexpr double MAX_SAMPLES_PER_DIRECTION = 1000;

constexpr double REF_REGION_VOLUME = 1e+8;
constexpr double CELL_VOLUME_FACTOR_PARAM = 0.5; // std::pow(cellvol, CELL_VOLUME_FACTOR_PARAM)でセル体積でのサンプリング点数補正因子を設定する。
// max cell volume factorは大きめでも良さそう。
constexpr double MAX_CELLVOLUME_FACTOR = 125;

}

/*
 * サンプル点数のチューニングは以下のsケースでチェックする。
 * 1. nmrilabo：セル体積補正が極端すぎないこと
 * 2. HPGe：自然な外見
 * 3．FNSDD：薄膜、屈曲部の描画ができること
 *
 *
 */

PolyConstructor::PolyConstructor(int numThreads, const std::vector<std::string> &cellNames,
								 //const ActorPairMap_type &amap,
								 const std::unordered_map<std::string, CellObject> &cellObjMap,
								 const GeometryViewerConfig& gconf)
	: numThreads_(numThreads),
	  cellNames_(cellNames),
//	  actorMap_(amap),
      currentSamplingRange_(gconf.region()),
	  cellObjMap_(cellObjMap),
	  geomConfig_(gconf)
{}


//i番目要素の処理の実装
// cellNames_のi番目のセルのポリゴンを作成してthisThreadResultへ
// 「cellName, polyData, サンプル点数」のタプルを追加する。
// Actor生成は着色と不可分なので、ここではcellObjectMapへの追加はしない。
void PolyConstructor::impl_operation(size_t i, int threadNumber, PolyConstructor::result_type *thisThreadResult)
{
	(void) threadNumber;
	const std::string &cellName = cellNames_.at(i);

	// この時点で更新対象actorはrendererから削除されているがactorMap_にはまだ残っている場合がある。
	// よってサンプル点数が変わっていなければデータ更新せず、polyDataへのスマートポインタを
	// shallow copyする。
//	if(actorMap_.find(cellName) != actorMap_.end()
//			&& std::abs(static_cast<double>(actorMap_.at(cellName).second - geomConfig_.numPoints())) < 0.001) {
	auto it0 = cellObjMap_.find(cellName);
	if(it0 != cellObjMap_.end() && it0->second.actor_ != nullptr
			&& std::abs(static_cast<double>(it0->second.numRefPoints_ - geomConfig_.numPoints())) < 0.001) {

		auto mapper = it0->second.actor_->GetMapper();
		vtkSmartPointer<vtkPolyData> poly = vtkPolyData::SafeDownCast(mapper->GetInputAsDataSet());
		thisThreadResult->emplace_back(std::make_tuple(cellName, poly, geomConfig_.numPoints(), it0->second.numRealPoints_));
		return;
	}




	// ########## ここからBondingBoxによる個別セル描画範囲の調整
	geom::BoundingBox bb = cellObjMap_.at(cellName).bb_;
	//mDebug() << "Start rendering cell ===" << cellName << ", initial calculated BB =" << bb.toInputString();

	// AABBは最後にモデル境界の直方体とANDを取って切り取る
	bb = geom::BoundingBox::AND(bb, geom::BoundingBox(currentSamplingRange_));
	// さらに補助平面でカットする
	bb = geom::BoundingBox::AND(bb, geomConfig_.getAuxPlanesBoundingBox());

	// ここでbbがemptyなら以降の処理は不要。std::make_tuple(cellName, polyData, baseRate_)してcontinueすべし
	if(bb.empty()) {
		// いうても結局複雑セルのサンプリング時間が支配的だから、ここで非描画セルの処理を多少省略してもあまり計算速度は上がらない。
		auto emptyPolyData = vtkSmartPointer<vtkPolyData>::New();
		thisThreadResult->emplace_back(std::make_tuple(cellName, emptyPolyData, geomConfig_.numPoints(), 0));
		return;
	}
	//mDebug() << "before expand BB=" << bb.toInputString();
	bb.expand(OFFSET_FACTOR);  // 大きさをOFFSET_FACTOR分だけ拡大する。
	//mDebug() << "Applied BB=" << bb.toInputString();

	auto sample = vtkSmartPointer<vtkSampleFunction>::New();
	sample->SetImplicitFunction(geomConfig_.cutImplicitFunction(cellObjMap_.at(cellName).implicitFunction_));
    sample->SetModelBounds(bb.range().data());
	sample->ComputeNormalsOff();
	/*
	 *  sample値のサイズは8バイトでsample要素数が多すぎるとメモリの割当に失敗する。
	 * 割当失敗時の挙動はvtkのビルドモードによって違って
	 * "VTKが" デバッグビルドならabort()で終了
	 * "VTKが" リリースビルドならbad_allocが投げられる。
	 *
	 * 外部ライブラリのビルドモードによって挙動が変わるのは避けたい。
	 * ・調べられる環境では空きメモリ量を調べて対応
	 * ・そうでない環境もあるのでVTKがreleaseであることを期待してtrycatchする。
	 */



	// ################################ ここから領域bdをつかってサンプリング点数dimを設定する。

	// baseRate:サンプリング密度、 bd = bb.range():サンプリング領域, dim:サンプリング数

	// ここからセルごとのサンプル点数ベース
	/*
	 * 方向別補正は最後に。
	 *
	 * d0*d1*d2 = N, d0:d1:d2 = x:y:zとすると d0=[N*y*z/(x*x)]^(1/3), d1=d0*y/x d2=d0*z/x
	 *
	 * x, y, zをどのように割り当てるか。考慮する要素は
	 *
	 * 1. そもそもの描画領域の大きさを考慮しても良さそう。描画領域が狭いときのほうがサンプル数は大きめで。
	 * 2．セルの大きさ：大きいセルは少し多くサンプリングしてサンプリング密度の非均一性を緩和する。
	 * 3．非等方性：アスペクト比が極端なセルは少し点数を増やしたほうが良い。(パイプ等が想定される)
	 * 4．辺の長さ：長手方向は少しサンプリング密度を落としても良い。短手方向は増やす必要がある。
	 * 5. 方向別最小サンプリング数の適用
	 */
	// len:描画領域のxyzサイズ（非負)
    auto bd = bb.range();
    std::array<double, 3> len{bd[1] - bd[0], bd[3] - bd[2], bd[5] - bd[4]};
    for(const auto& l: len) {
        (void)l;
        assert(l > 0);
    }

	double numPoints = static_cast<double>((std::max)(geomConfig_.numPoints(), MIN_SAMPLE_POINTS));

	// 1. 描画領域の体積による補正 → 廃止
	//numPoints *= regionVolumeFactor;

	// 2．セルの体積に応じてサンプリング点数を増やし、サンプリング密度の非均一性を緩和する
	double cellVolume = bb.volume();
	if(cellVolume < 1) cellVolume = 1;
	double cellVolumeFactor = (cellVolume <= 1) ? 1 : std::pow(cellVolume, geomConfig_.smoothingFactor());
	//double cellVolumeFactor = 1 + std::log(cellVolume);
	cellVolumeFactor = (std::min)(cellVolumeFactor, MAX_CELLVOLUME_FACTOR);
	numPoints *= cellVolumeFactor;

	// 3.アスペクト比補正
	// 対角線*辺の平均値^2/体積/√3を非等方性の指標とする。
	double avelen = 0.3333*(len[0]+len[1]+len[2]);
	double aspectRatio = 0.57735*std::sqrt(len[0]*len[0] + len[1]*len[1] + len[2]*len[2])*avelen*avelen
			/(len[0]*len[1]*len[2]);
	// そのまま乗数にすると過激すぎるのでlogで緩和する。
	// 露骨にアスペクト比で補正を掛けると、中空カプセルみたいな薄くてaspect比の小さいセルがやばくなる
	double aspectCorrectionFactor = std::pow(aspectRatio, 0.4);
	numPoints *= aspectCorrectionFactor;

	// 4. 辺の長さによる非均一性サンプリング。平方根で重み付けをすることで線形より穏やかな重み付け
	std::array<double, 3> w {
		len[0] < 1 ? len[0] : std::pow(len[0], 0.6),
		len[1] < 1 ? len[1] : std::pow(len[1], 0.6),
		len[2] < 1 ? len[2] : std::pow(len[2], 0.6)
	};
	assert(w[0] > 0 && std::abs(w[0]) > 1e-10);
	std::array<double, 3> dim;
	dim[0] = std::pow(numPoints*(w[0]*w[0])/(w[1]*w[2]), 0.3333333);
	dim[1] = dim[0]*w[1]/w[0];
	dim[2] = dim[0]*w[2]/w[0];

	// LATTICE要素の解像度は落とす
	if(cellName.find("[") != std::string::npos) {
		for(size_t i = 0; i < dim.size(); ++i) dim[i] *= LATTICE_ELEM_REDUCTIO_FACTOR;
	}

	mDebug() << "\ncell === " << cellName << ", Num points ===" << numPoints;
	mDebug() << "BB size(cm) ===" << len << ", vol(cm3)===" << len[0]*len[1]*len[2];
	mDebug() << "samples ===" << dim << ", sample density(pts/cm)" << dim[0]/len[0] <<dim[1]/len[1] <<dim[2]/len[2];
	mDebug() << "Correction (cellVol aspect) ==="
			  << cellVolumeFactor << aspectCorrectionFactor;


	// 5. 最小サンプリング数の設定
	// ある方向のサンプルレートが2以下になるとその方向のサンプル点不足で描画できなくなるので下限を設ける。
	for(size_t i = 0; i < dim.size(); ++i){
		if(dim[i] < MIN_SAMPLES_PER_DIRECTION ) {
			mDebug() << "cell===" << cellName << "i=" << i << "dim===" << dim[i]
						<< "is lower than minimum=" << MIN_SAMPLES_PER_DIRECTION;
			dim[i] = MIN_SAMPLES_PER_DIRECTION;
		}
	}

	// 6.最大サンプリング数の設定
	// 異常なサイズのセルのサンプル数を制限する
	for(size_t i = 0; i < dim.size(); ++i){
		if(dim[i] > MAX_SAMPLES_PER_DIRECTION ) {
			mDebug() << "cell===" << cellName << "i=" << i << "dim===" << dim[i]
						<< "is larger than maximum=" << MAX_SAMPLES_PER_DIRECTION;
			dim[i] = MAX_SAMPLES_PER_DIRECTION;
		}
	}


	static const unsigned long totalPhysMemMB = utils::getTotalMemMB();
	// vtkのコンパイルフラグによってはbad_allocの発生は期待できないので
	// メモリが割り当て可能かチェックする。
	// NOTE vtk setsampleDimensionsの内部でのvalueTypeをpublicに知ることはできないだろうか？
	unsigned long requiredMemMB = dim[0]*dim[1]*dim[2]*SAMPLE_ELEMENT_SIZE_BYTE/1048576.0;
	mDebug() << "Samples =(" << dim[0] << dim[1] << dim[2]
			 << "), required(MB) = " << requiredMemMB << ", available =" << utils::getAvailMemMB();
	unsigned long freeMemMB = utils::getAvailMemMB();

	/*
	 * メモリ使用量の制限
	 * 1．(このアプリに限らず)システムのメモリ使用量が限界値を超えた場合
	 * 2. マーチングキューブ法の要求するメモリ量が空きメモリ量/スレッド数を超えた場合
	 * 例外を発生させてポリゴン生成を止める。
	 */
    constexpr double MEM_SAFETY_FACTOR = 1.1;
    double realNumThreads = (numThreads_ == 0) ? 1 : numThreads_;
	if(100*static_cast<double>(totalPhysMemMB - freeMemMB)/totalPhysMemMB > geomConfig_.memoryUsagePercentLimit()) {
		std::stringstream ss;
		ss<< "Not enough free memory, free(MB) = " << freeMemMB << ", total(MB) = " << totalPhysMemMB
		  << ", limit(%) = " << geomConfig_.memoryUsagePercentLimit() << ". "
		  << "Reduce number of points per cell. ";
		throw std::invalid_argument(ss.str());
    } else if(MEM_SAFETY_FACTOR*requiredMemMB > freeMemMB/realNumThreads && freeMemMB > 0) {
		std::stringstream ss;
        ss<< "Not enough free memory, required = "
          << MEM_SAFETY_FACTOR*requiredMemMB << " (MB), available(per thread)="
          << freeMemMB/realNumThreads << " (MB). "
		  << "Reduce number of points per cell. ";
		throw std::invalid_argument(ss.str());
	}

	// Update()でbad_allocが投げられるかはビルドよるので注意。(debugビルドでは投げられない)
	// しかもこれはおそらく文書化された仕様ではないから将来的に変更される可能性はある。
	// ゆえにここのbad_alloc発生はあまり信用しないこと。
	try{
		sample->SetSampleDimensions(dim[0], dim[1], dim[2]);
		sample->Update();  // XXX ここが一番重い処理。XXX
	} catch (std::bad_alloc &ba) {
		(void) ba;
        throw std::runtime_error(std::string(" Memory allocation failed. status = ") + ba.what()
                                 +  " Too much sampling point for cell = " + cellName);
	}



	// contour
	// データがstructuredならvtkContourFilterよりvtkMarchingCubesを使ったほうが微妙に速いらしい。
//	auto surface = vtkSmartPointer<vtkContourFilter>::New();
	auto surface = vtkSmartPointer<vtkMarchingCubes>::New();
	surface->SetInputConnection(sample->GetOutputPort());
	surface->SetValue(0, 0.0);
	surface->Update();
	auto polyData = vtkSmartPointer<vtkPolyData>::New();
	polyData->DeepCopy(surface->GetOutput());

//	thisThreadResult->emplace_back(std::make_tuple(cellName, polyData, dim[0]*dim[1]*dim[2]));
	thisThreadResult->emplace_back(std::make_tuple(cellName,
												   polyData,
												   geomConfig_.numPoints(),
												   static_cast<int>(dim[0]*dim[1]*dim[2])));
}





OperationInfo PolyConstructor::defaultInfo()
{
	std::string title = QObject::tr("Progress").toStdString();
	std::string operatingText = QObject::tr("Creating polygons ").toStdString();
	std::string cancelingText = QObject::tr("Waiting for the subthreads to finish").toStdString();
	std::string cbuttonLabel = QObject::tr("Cancel").toStdString();
	return OperationInfo(title, operatingText, cancelingText, cbuttonLabel);
}

PolyConstructor::result_type PolyConstructor::collect(std::vector<typename PolyConstructor::result_type> *resultVec)
{

	auto retvec = collectVector<result_type>(resultVec);
	return retvec;
}
