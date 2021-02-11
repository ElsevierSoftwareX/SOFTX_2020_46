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
#include "geometryviewer.hpp"
#include "ui_geometryviewer.h"

#include <bitset>
#include <chrono>
#include <cmath>
#include <future>
#include <list>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>
#include <QMouseEvent>
#include <QProgressDialog>
#include <QScreen>
#include <QTimer>

#include "vtkinclude.hpp"


#include "bbcalculator.hpp"
#include "cell3dexporter.hpp"
#include "cellexportdialog.hpp"
#include "custamtablewidget.hpp"
#include "namecomparefunc.hpp"
#include "polypainter.hpp"
#include "polyconstructor.hpp"
#include "customqvtkwidget.hpp"


#include "../globals.hpp"
#include "../../core/fielddata/fieldcolordata.hpp"
#include "../../core/geometry/geometry.hpp"
#include "../../core/geometry/cell/cell.hpp"
#include "../../core/material/material.hpp"
#include "../../core/utils/string_utils.hpp"
#include "../../core/utils/system_utils.hpp"
#include "../../core/utils/time_utils.hpp"
#include "../../core/utils/progress_utils.hpp"
#include "../subdialog/messagebox.hpp"
#include "../option/guiconfig.hpp"

namespace {

//const double OFFSET_FACTOR = 1.01;  // 陰関数サンプリングレンジはboundinbboxより少し大きくする
CameraInfo XPLUS_CAMERA_INFO  = CameraInfo(std::array<double, 9>{300, 0,  0,  0, 0, 0,  0, 0, 1});
CameraInfo XMINUS_CAMERA_INFO = CameraInfo(std::array<double, 9>{-300, 0, 0,  0, 0, 0,  0, 0, 1});
CameraInfo YPLUS_CAMERA_INFO  = CameraInfo(std::array<double, 9>{0, 300,  0,  0, 0, 0,  0, 0, 1});
CameraInfo YMINUS_CAMERA_INFO = CameraInfo(std::array<double, 9>{0, -300, 0,  0, 0, 0,  0, 0, 1});
CameraInfo ZPLUS_CAMERA_INFO  = CameraInfo(std::array<double, 9>{0, 0,  300,  0, 0, 0,  0, 1, 0});
CameraInfo ZMINUS_CAMERA_INFO = CameraInfo(std::array<double, 9>{0, 0, -300,  0, 0, 0,  0, 1, 0});
//const int SAMPLE_ELEMENT_SIZE_BYTE = 8;  // vtkで陰関数をサンプリングする時の返り値のデータサイズ
//const int INITIAL_RATE = 50;

// exportする時の倍率の最大最小
constexpr double MAX_FACTOR = 99999;
//constexpr double MIN_FACTOR = -1*MAX_FACTOR;
//constexpr int DEC_FACTOR = 4;


geom::BoundingBox DEFAULT_BB(-50, 50, -50, 50, -50, 50);
constexpr int SETTING_PANE_INDEX = 1;
}  // end anonymous namespace



GeometryViewer::GeometryViewer(QWidget *parent, const QString &tabText, const GuiConfig *gconf)
	: TabItem(parent, tabText, gconf), ui(new Ui::GeometryViewer), isFirstCall_(true)
{
	// 1．このクラス自体の初期化
	ui->setupUi(this);
	ui->toolBox->removeItem(0);
	// サンプリング密度に関するwidgetは隠す。そして将来的には削除
	ui->checkBoxAutoResolution->setHidden(true);

	// カメラボタン類のサイズ調整
	auto cameraButtonSize = ui->pushButtonZPlusCamera->size();
	auto sideLength = 0.8*std::min(cameraButtonSize.width(), cameraButtonSize.height());
	cameraButtonSize = QSize(sideLength, sideLength);
	ui->pushButtonXPlusCamera->setIconSize(cameraButtonSize);
	ui->pushButtonXMinusCamera->setIconSize(cameraButtonSize);
	ui->pushButtonYPlusCamera->setIconSize(cameraButtonSize);
	ui->pushButtonYMinusCamera->setIconSize(cameraButtonSize);
	ui->pushButtonZPlusCamera->setIconSize(cameraButtonSize);
	ui->pushButtonZMinusCamera->setIconSize(cameraButtonSize);
	ui->pushButtonShowGrid->setIconSize(cameraButtonSize);
	ui->pushButtonShowArrows->setIconSize(cameraButtonSize);
	ui->pushButtonEnableCellPicker->setIconSize(cameraButtonSize);
    ui->pushButtonEnableLegend->setIconSize(cameraButtonSize);
	// grid表示
	connect(ui->pushButtonShowGrid, &QPushButton::toggled,
			this, [=](bool on){settingPane_->showCubeAxes(on);});
	// 矢印表示
	connect(ui->pushButtonShowArrows, &QPushButton::toggled,
			this, [=](bool on){settingPane_->showArrows(on);});
	// picker無効にしたらピックしたセル情報は消す
	connect(ui->pushButtonEnableCellPicker, &QPushButton::toggled,
			this, [=](bool checked) {
                if(!checked) {
                    renderer_->RemoveActor2D(textActor_);
                    this->updateQVTKWidget(false);
            }});
//    connect(ui->pushButtonEnableLegend, &QPushButton::toggled, this,
//            [=](bool checked) {
//                if(!checked) {
//                    renderer_->RemoveActor2D(legendActor_);
//                    this->updateQVTKWidget(false);
//                }
//            });

	qvtkWidget_ = new CustomQVTKWidget(this);
	qvtkWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//	ui->verticalLayoutForView->addWidget(qvtkWidget_);
	ui->verticalLayoutForView->insertWidget(0, qvtkWidget_);
	auto layout = new QHBoxLayout;
	ui->verticalLayoutForView->addLayout(layout);
	ui->splitter->setOpaqueResize(false);
	qvtkWidget_->SetRenderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New());
	renderer_ = vtkSmartPointer<vtkRenderer>::New();
    qvtkWidget_->GetGenericRenderWindow()->AddRenderer(renderer_);
	renderer_->SetActiveCamera(renderer_->MakeCamera());
	cameraInfo_ = CameraInfo::fromCamera(renderer_->GetActiveCamera());
	qvtkWidget_->show();
	connect(qvtkWidget_, &CustomQVTKWidget::customMouseEvent, this, &GeometryViewer::handleQVTKMouseEvent);


	// 2．セルペインの初期化 Toolbox page1セルペイン
	cellPane_ = new CellPane(this, guiConfig_);
	ui->toolBox->addItem(cellPane_, tr("Cell"));
	// Toolbox page 1 Cell選択ペイン
	// チェックオンオフでremovingCellNames_に追加
	connect(cellPane_, &CellPane::cellNameAdded, this, [this](std::string cellName){
																removingCellNames_.erase(cellName);
																addingCellNames_.insert(cellName);
																});
	connect(cellPane_, &CellPane::cellNameRemoved, this, [this](std::string cellName){
																removingCellNames_.insert(cellName);
																addingCellNames_.erase(cellName);
																});

	// 3. セッティングペインの初期化 Toolbox page2 Settingペイン
	settingPane_ = new SettingPane(this, renderer_);
	ui->toolBox->addItem(settingPane_, tr("Setting"));
	geomConfig_ = settingPane_->getGeomConfig();
	connect(settingPane_, &SettingPane::requestAutoCalcRegion,
			this, [=](){settingPane_->updatePane(largestBB());});
	connect(settingPane_, &SettingPane::requestDisableAutoResolution,
			this, [=](){ui->checkBoxAutoResolution->setChecked(false);});
	connect(settingPane_, &SettingPane::requestDisableAutoRegion,
			this, [=](){ui->checkBoxAutoDrawingVolume->setChecked(false);});
	connect(settingPane_, &SettingPane::requestUpdateQVTKWidget,
			this, [=](){this->updateQVTKWidget(false);});

	// 3. 色ペインの初期化
	colorPane_ = new ColorPane(this);
	ui->toolBox->addItem(colorPane_, tr("Color"));


	// その他
    connect(qvtkWidget_, &CustomQVTKWidget::resized,
            this, &GeometryViewer::updateMaterialLegendPosition);
	ui->toolBox->setCurrentIndex(0);
	QTimer::singleShot(0, [=](){init();});
    qvtkWidget_->show();

}

GeometryViewer::~GeometryViewer() {delete ui;}



void GeometryViewer::init()
{
//	TabItem::init();
	//if(hasInitialized_) return;
	int paneWidth = std::max(cellPane_->minimumNoScrollWidth(), settingPane_->minimumNoScrollWidth());
	ui->splitter->setSizes(QList<int>{paneWidth, this->size().width() - paneWidth});
	TabItem::init();

}

void GeometryViewer::retranslate()
{
	ui->retranslateUi(this);
	cellPane_->retranslate();
	settingPane_->retranslate();
	colorPane_->retranslate();
}

void GeometryViewer::exportToRasterGraphics()
{
	try {
		ff::exportRenderWindowToRasterGraphics(qvtkWidget_->GetRenderWindow());
	} catch (std::exception &e) {
		mWarning() << e.what();
	}
}

void GeometryViewer::exportTo(ff::FORMAT3D fformat) {

	// まずダイアログでscaling factorを取得する。
	bool ok;
	auto resultPair = CellExportDialog::getExportingInfo(this, &ok);
//	factor = QInputDialog::getDouble(this, "", tr("Scaling factor:"), 1,
//									 MIN_FACTOR, MAX_FACTOR, DEC_FACTOR, &ok, Qt::WindowFlags());
	if(!ok) return;
	double factor = resultPair.first;
	bool unify = resultPair.second;

	QString dirName = QFileDialog::getExistingDirectory(this, tr("Select exporting directory"));
	if(dirName.isNull()) {
		return;
	} else if (dirName.isEmpty()) {
		dirName = ".";
	}
	// ここでFileDialogの消えるのが遅い。ネットではnativeを使うなとかあるけど不明
	QDir dir(dirName);


	std::vector<std::string> results;

	OperationInfo opInfo = Cell3DExporter::info();
	opInfo.numTargets = unify ? 1 : displayedCellNames_.size();
	opInfo.numThreads = guiConfig_->cuiConfig.numThread;

	//セルを一体化させてエクスポートする場合、並列処理は出来ない。
	// が、progress表示などを利用するためProceedOperationは利用する。
	// 第二以降の引数はファンクタのコンストラクタ引数
	results = ProceedOperation<Cell3DExporter>(opInfo, fformat, dir, cellObjMap_,
											   std::vector<std::string>(displayedCellNames_.cbegin(), displayedCellNames_.cend()),
											   factor, unify);





	if(results.size() != opInfo.numTargets) mWarning() << "Some cells were not exported.";
}


geom::BoundingBox GeometryViewer::largestBB() const
{
	std::unique_ptr<geom::BoundingBox> maxbb;
	for(const auto &cellObjPair: cellObjMap_) {
		const std::string & cellName = cellObjPair.second.cellName_;
		const geom::BoundingBox &bb = cellObjPair.second.bb_;
		if(cellPane_->isChecked(cellName)) {
			if(bb.isUniversal(false)) {
				mWarning() << "Bounding box has infinit extent, cell name ===" << cellName << bb.range();
			}
			//mDebug() << "bbMapのBB, name=" <<  bbPair.first << "range=" << bbPair.second.toInputString();
			if(!maxbb) {
				maxbb.reset(new geom::BoundingBox());
				*maxbb.get() = bb;
			} else {
				*maxbb = geom::BoundingBox::OR(*maxbb.get(), bb);
			}
		}
	}
	if(maxbb) {
		return *maxbb.get();
	} else {
		mWarning() << "Calculation of the largest bounding box failed, default =" << DEFAULT_BB.toInputString()  << " used.";
		return DEFAULT_BB;
	}
}


void GeometryViewer::removeAllActors()
{
	auto actors = renderer_->GetActors();// vtkRenderer
	actors->InitTraversal();
	while(actors->GetNumberOfItems() != 0) {
		renderer_->RemoveActor(actors->GetNextActor());
	}
	auto actors2D = renderer_->GetActors2D();
	actors2D->InitTraversal();
	while(actors2D->GetNumberOfItems() != 0) {
		renderer_->RemoveActor2D(actors2D->GetNextActor2D());
	}
}



// 外部からジオメトリデータを受け取った時の処理スロット
void GeometryViewer::updateSimulationDataX(std::shared_ptr<const Simulation> sim)
{
	namespace stc = std::chrono;
	if(!sim) return;
    this->clear();
	simulation_ = sim;
	std::vector<std::string> cellNameList;  // セル一覧チェックボックスを作るためのセル名リスト
//	std::vector<geom::BoundingBox> bbVec;

	// ### BoundingBox計算開始
	utils::SimpleTimer timer;
	timer.start();

	/*
	 * future版ではprogressが仕事をしないのと、
	 * メモリの開放が遅いために足りなくなる問題があるため、
	 * thread並列化にする。
	 */

	/*
	 * やること。
	 * cellNameListにセル名を追加
	 * functionMap_に陰関数を追加
	 * bbMap_にBBを追加
	 * する。
	 */

	// ここからBB計算
	OperationInfo operationInfo = BBCalculator::info();
	operationInfo.numTargets = simulation_->getGeometry()->cells().size();
    operationInfo.numThreads = static_cast<size_t>(guiConfig_->cuiConfig.numThread);
	operationInfo.sleepMsec = 50;

	assert(guiConfig_ != nullptr);
	// result_type は std::unordered_map<std::string, geom::BoundingBox>
	BBCalculator::result_type boundingBoxes
			= ProceedOperation<BBCalculator>(operationInfo, simulation_->getGeometry()->cells(), guiConfig_->cuiConfig);

	// ### BoundingBox計算終わり。

	if(boundingBoxes.empty()) {
		std::string message
				= tr("Boundingbox calculation was canceled. If you want to continue, reload the input file").toStdString();
		throw std::runtime_error(message);
	}

	// cellPair.firstはセル名、cellPair.secondはセルクラスへのshared_ptr
	for(auto const &cellPair: simulation_->getGeometry()->cells()) {
		//cellNames.emplace_back(cellPair.first);
		// 陰関数とboundingboxは不変なのでデータ受信時に生成しておく。

		const geom::BoundingBox &bb = boundingBoxes.at(cellPair.first);
		// 陰関数を計算するセルはBBが非emptyなセルだけ。
		if(!bb.empty()) {
			// 陰関数の生成コストはしれているので並列化しない
			cellObjMap_.emplace(cellPair.first, CellObject(cellPair.first, bb, cellPair.second->createImplicitFunction(), nullptr, 0, 0));
			cellNameList.emplace_back(cellPair.first);
		}
	}
	timer.stop();


	mDebug() << "Calculating BBs done in" << timer.msec() << "(ms)";
//	auto spTimes = timer.splitIntervalsMSec();
//	for(size_t i = 0; i < cellNameList.size(); ++i) {
//		mDebug() << "cell===" << cellNameList.at(i) << " bbcalc time(ms)===" << spTimes.at(i);
//	}


	// cellNameLessはセル名の数値/文字列/階層構造を考慮した比較関数
	timer.clear();
	timer.start();
	// コレひょっとしたら超絶時間掛かっているかも C++11ではO(NlogN) → そんなことなかった。
	std::sort(cellNameList.begin(), cellNameList.end(), &cellNameLess);

	timer.stop();
	mDebug() << "Sorting cell name list done in " << timer.msec() << "(ms)";

	// Pane, axisのアップデート
	mDebug() << "START updating cellPane.";
	timer.start();
	cellPane_->updateCellPane(simulation_, cellNameList);
	timer.stop();
	mDebug() << "END updating cell Pane in " << timer.msec() << "(ms)";

	settingPane_->updatePane(largestBB());

	geomConfig_ = settingPane_->getGeomConfig();
	// サイズ調整を再度試行
	init();
	// データ受取が完全に終わったらアップデートボタンをenableする。
	ui->pushButtonUpdateView->setEnabled(true);
	// すごく重い体系かもしれないので直ちに自動的に描画することは避けるが、
	// CubeAxeはボタンの状態に応じて更新する
	if(ui->pushButtonShowGrid->isChecked()) {
		settingPane_->showCubeAxes(true);
	}
	mDebug() << "update simulation data in geometryviewer done.";

}

void GeometryViewer::mouseReleaseEvent(QMouseEvent *ev)
{
	TabItem::mouseReleaseEvent(ev);
}

void GeometryViewer::mousePressEvent(QMouseEvent *ev)
{
    TabItem::mousePressEvent(ev);
//	if(event->type() == QEvent::MouseButtonPress && event->button() == Qt::RightButton) {
//		event->ignore();
//		mDebug() << "右クリックはmousePressEventでignoreしたやで";
//	}

}

void GeometryViewer::clear()
{
	// 全アクター削除
	removeAllActors();

	simulation_.reset();
	cellPane_->clear();
	settingPane_->clear();
    colorPane_->clear();

	selectedActor_ = nullptr;
	textActor_ = nullptr;
	isFirstCall_ = true;

	removingCellNames_.clear();
	addingCellNames_.clear();
	displayedCellNames_.clear();
	cellObjMap_.clear();

	cameraInfo_ = ZPLUS_CAMERA_INFO;
	geomConfig_.clear();

	//初期化
//	renderer_->Clear();  //背景の消去

	renderer_->SetBackground(static_cast<double>(guiConfig_->bgColor3D.r)/255,
							 static_cast<double>(guiConfig_->bgColor3D.g)/255,
							 static_cast<double>(guiConfig_->bgColor3D.b)/255);

	qvtkWidget_->update();
	qvtkWidget_->GetRenderWindow()->Render();

	ui->pushButtonUpdateView->setEnabled(false);
}



void GeometryViewer::handleUpdateView()
{
//	if(ui->checkBoxAutoDrawingVolume->isChecked())
	updateView(false);
}

/*
 * 最初にupdateViewを実行する時は既にactiveカメラは生成済みではあるが、
 * デフォルトのpos=1,0,0とかそんな値が入っている。
 * よて最初は自動調整データをつかい、
 * あとはupdateButtonを押した時に保存したcameraInfoを設定する。
 *
 * doNotChangeRegion が trueなら描画領域を変更せず、ポリゴン生成もしない。
 */
#include <bitset>
void GeometryViewer::updateView(bool doNotChangeRegion)
{
	namespace stc = std::chrono;
	// セルリストが空の状態でここに来ることがあってバグるの防止。
	if(!simulation_) return;

	// autofit設定の場合は領域更新
	if(!doNotChangeRegion && ui->checkBoxAutoDrawingVolume->isChecked()) {
		// ここで重要なのは描画領域自動設定を有効にしている場合、
		// 領域推定に失敗したらどうするか。
		// 1．チェックボックスは外す
		// 2．推定に失敗している方向をMessageBoxで表示し、
		// 3．描画はする。
		// 4．SettingPaneにフォーカスを移す
		auto maxbb = largestBB();
		std::array<double, 6> range = maxbb.range();
		std::bitset<6> estimationFailed("000000"); // 0:成功、1:失敗に対応で-x,+x,-y,+y,-z,+zの6方向

		static const std::vector<QString> dirStrs{"-x", "+x", "-y", "+y", "-z", "+z"};
		QString message = tr("Automatic estimation of drawing volume failed"
							  " for some reasons(ex. infinit cells or too complicated cells).\n");
		QString dirMessage = tr("Direction: ");
		QString bbMessage = tr("Applied default drawing volume = \n{");
		// BBに無限大(計算不能)な部分があった場合は規定の最大値に丸める
		for(size_t i = 0; i < 6; ++i) {
			if(std::abs(range.at(i)) >= 0.1*geom::BoundingBox::MAX_EXTENT) {
				estimationFailed.set(i);
				dirMessage += dirStrs.at(i) + ", ";
				range.at(i) = std::pow(-1, (i+1)%2)*SettingPane::DEFAULT_BB_SIZE;
			}
		}
		dirMessage = dirMessage.mid(0, dirMessage.size()-2) + "\n";  // 末尾の", "を消す。

		if(estimationFailed.any()) {
			ui->checkBoxAutoDrawingVolume->setChecked(false);
			// 適用するBBの文字列を作成する。
			std::stringstream ss;
			for(size_t i = 0; i < range.size(); ++i) {
				ss << range.at(i);
				if(i != range.size()-1) ss << ", ";
			}
			bbMessage += QString::fromStdString(ss.str() + "}\n");
			QString order = tr("Use manual setting in the setting pane.");
            GMessageBox::warning(Q_NULLPTR, tr("warning"), message + dirMessage + bbMessage + order, true);
			// セッティングペインにフォーカスを移す
			ui->toolBox->setCurrentIndex(SETTING_PANE_INDEX);
		}
		// SettingPaneで描画領域を変更したら、ui->checkBoxAutoDrawingVolumeのチェックを外れるように
		// signalが繋がれているので前後でブロックする。
		settingPane_->setRegionByBB(maxbb);
	}
	// ここまで領域自動チェックが入っている時の処理

	if(cameraInfo_.hasData) saveCameraState();
	ui->pushButtonUpdateView->setEnabled(false); // 連打できないように描画中はdisableする。
	/*
	 * ジオメトリ情報の変化は
	 * 1．可視セルの追加 (addingCellNames_ にセル名が格納されている)
	 * 2．可視セルの削除 (removingCellNAmes_にセル名が格納されている)
	 * 3．サンプリングレートの変更
	 * 4．レンダリング領域の変更
	 * で発生する。
	 * 1．2．については追加があった場合は名前がaddingCellNames_に、削除があった場合にはremovingCellNamesに
	 * セル名が格納されている。
	 * 3．4についてはui中の設定値を取得し、現在保持している設定値との比較により検知する。
	 */

	/*
	 * TODO 領域カットを伴わない補助平面追加時はポリゴン生成しないようにしたい。
	 * 問題は補助平面追加と補助平面カットで処理がことなること。
	 * (前者は平面actor追加なのに対し、後者は既存セルの再構築)
	 *
	 * isConfigChanged判定は
	 * 前回設定(geomConrig_)とcurrentConfigが補助平面以外の部分で差があればtrue
	 * カット付き補助平面の変更があればtrue
	 * カットなし補助平面は判定に使わない。
	 */

	auto currentConfig = settingPane_->getGeomConfig();
	bool isConfigChanged = geomConfig_.hasChangedExceptAuxPlanes(currentConfig);
	if(!isConfigChanged) isConfigChanged = geomConfig_.hasChangedCuttingPlanes(currentConfig);

	geomConfig_ = currentConfig;  // 設定はとりあえず上書きする。


	if(isFirstCall_ || isConfigChanged) {  // colorペインの変更はポリゴン構築に関係ない。
	//if(isFirstCall_ || isConfigChanged || colorPane_->isChanged()) {
		// 初回あるいは設定変更の場合全セル更新する
		removingCellNames_.clear();
		addingCellNames_ = cellPane_->getCheckedCellNames();

		// 全アクター削除
		displayedCellNames_.clear();
		//actorMap_.clear();
		for(auto &cellObjPair: cellObjMap_) {
			cellObjPair.second.actor_ = nullptr;
			cellObjPair.second.numRefPoints_ = 0;
			cellObjPair.second.numRealPoints_ = 0;
		}
		removeAllActors();

	} else {
	// 部分更新の場合
		// adding, removing セルどちらもアクター更新するのでrendererからアクターを削除する。そしてremovingはその後追加しない。
		// また、ここではレンダラからは削除するが、actor自体は削除しない。(サンプルレート不変で再追加された時再利用するため)
		// displayedCellNames_は3Dへエクスポートするセルを選択するために使っている。
		for(auto &cellName: addingCellNames_) {
			mDebug() << "addingCell =" << cellName;
			auto it0 = cellObjMap_.find(cellName);
			if(it0 != cellObjMap_.end() && it0->second.actor_ != nullptr){
				renderer_->RemoveActor(it0->second.actor_);
				if(displayedCellNames_.find(cellName) != displayedCellNames_.end())
					displayedCellNames_.erase(displayedCellNames_.find(cellName));
			}
		}
		for(auto &cellName: removingCellNames_) {
			mDebug() << "removingCell = " << cellName;
			auto it0 = cellObjMap_.find(cellName);
			if(it0 != cellObjMap_.end() && it0->second.actor_ != nullptr){
				renderer_->RemoveActor(it0->second.actor_);
				if(displayedCellNames_.find(cellName) != displayedCellNames_.end())
					displayedCellNames_.erase(displayedCellNames_.find(cellName));
			}
		}
	}


	// Grid, 補助平面、スカラバーは必ずコストが低いので毎回更新関数を呼ぶ
    if(ui->pushButtonEnableLegend->isChecked()) {
        showMaterialLegend(ui->pushButtonEnableLegend->isChecked());
    }
	// showGridButtonのチェック状態に応じてcubeAxesを描画。
	// この時showCubeAxesはqvtkwidgetのアップデートを要求するため
	// 結局二回qvtkwidgetがアップデートされて気持ち悪いが仕方ない。
	if(ui->pushButtonShowGrid->isChecked()) {
		settingPane_->showCubeAxes(true);
	}
	// 補助平面のアクター更新も低コストなのでとりあえず実行しておく
	// ここときの設定はsettingPane->getGeomConfig()に従う。
	settingPane_->updateAuxPlanes();

	// スカラバーは必ず更新関数を呼ぶ。更新するかはcolorPane次第。どうせ更新コストは安い。
	colorPane_->updateScalarBar(renderer_);
	// ############ fieldDataの取得とチェック
	std::shared_ptr<const fd::FieldColorData> fcData;
	try {
		fcData = colorPane_->fieldColorData();
	} catch (std::exception &e) {
		mWarning() << e.what();
		ui->pushButtonUpdateView->setEnabled(true);
		return;
	}

	// 補助平面、スカラバー、grid,矢印は個別にレンダラをアップデートしないので
	// そのままだとguiイベント発生までは描画されない。ゆえにここで更新をかける
	updateQVTKWidget(isFirstCall_);



	// 更新するセルが無い場合はリターン
	if(addingCellNames_.empty() && removingCellNames_.empty()) {
		if(colorPane_->isChanged()) updateViewColorOnly();
		ui->pushButtonUpdateView->setEnabled(true);
		return;
	}
	// ここから先でaddingCellNames_, removingCellNames_は使わないので消去
	removingCellNames_.clear();



	// ここまでで更新項目確定

	// ここからポリゴン生成
	OperationInfo operationInfo = PolyConstructor::defaultInfo();
	operationInfo.numTargets = addingCellNames_.size();
	// vtkPolyData作成はvtkのopenMPを使ったほうが速いので
	// vtkをopenMPビルドしてこのプログラムからはシーケンシャル実行した方が良さそう。
    operationInfo.numThreads = static_cast<size_t>(guiConfig_->cuiConfig.numThread);
	operationInfo.sleepMsec = 50;

	// addingCellNames_はstd::setでindex番号でランダムアクセスできず並列処理向きではないためvector化して処理する。
	utils::SimpleTimer timer;
	timer.start();

	// ポリゴンの生成
	// resultの型はstd::vector<std::tuple<std::string, vtkSmartPointer<vtkPolyData>, int>>
	// vector<tuple<セル名、polyData, refサンプル点数, 実サンプル点数>>
    std::vector< std::tuple<std::string, vtkSmartPointer<vtkPolyData>, int, int> >
            polyDataTuples = ProceedOperation<PolyConstructor>(operationInfo,
														   guiConfig_->cuiConfig.numThread,
														   std::vector<std::string>(addingCellNames_.cbegin(), addingCellNames_.cend()),
														   cellObjMap_, geomConfig_);

	// polyDataTuplesサイズが更新セル数と異なる場合、プログレスダイアログでキャンセルされているので、
	// その場合は何もせず、ボタンの状態だけ戻してリターン
	if(polyDataTuples.size() != addingCellNames_.size()){
		ui->pushButtonUpdateView->setEnabled(true);
		return;
	}



    /*
     *  ポリゴン構築後空きメモリが倍くらいないとエラーになるのは
     *    vtkRenderer::Render()を直に呼び出しているためだったので、ポリゴン構築後のメモリ量チェックは不要。
     *    (Window::RenderWindow()を呼び出すのが正しい。とヘッダに書いてあった)
     */
//    int totalUsedMem = 0;
//    for(const auto& polyTuple: polyDataTuples) {
//        if(std::get<1>(polyTuple) != nullptr) {
//            totalUsedMem += std::get<3>(polyTuple);  // 実際のポリゴンの頂点数
//        }
//    }
//    totalUsedMem *= static_cast<double>(PolyConstructor::SAMPLE_ELEMENT_SIZE_BYTE)/1048576.0;

//    mDebug() << "Used for polygons(MB)===" << totalUsedMem << ", Free===" << utils::getAvailMemMB();
//    if(totalUsedMem > static_cast<int>(utils::getAvailMemMB())) {  // DEBUG
//        ui->pushButtonUpdateView->setEnabled(true);
//        GMessageBox::critical(this, tr("critical"),
//                              tr("No enough memory to render polygons."""
//                                 "Reduce number of points per cells in the cell pane."), true);
//        return;
//    }







	// ポリゴン生成終了後直ちにdisplayedCellNames_は更新する。
	for(const auto& tup: polyDataTuples) {
		if(std::get<1>(tup) != nullptr) displayedCellNames_.insert(std::get<0>(tup));
	}



	// ポリゴンに(fielddataベース、あるいはパレットベースの)色をつける。
	// というか実際にはvtkActorのプロパティに色をつける。
	// 返り値はvectorで要素は アクタースマポとサンプル点数のペア

	auto actorTuples = ProceedOperation<PolyPainter>(operationInfo, polyDataTuples,
													 &cellObjMap_,
													 simulation_->getGeometry()->palette(),
													 fcData, colorPane_->lookupTable());
	// アクター更新がキャンセルされた場合もやはりボタンだけ直して何もせずリターン。
	if(actorTuples.size() != polyDataTuples.size()) {
		ui->pushButtonUpdateView->setEnabled(true);
		return;
	}





	// レンダラへのセットとマップ更新
	for(const auto& actorTuple: actorTuples) {
		if(std::get<1>(actorTuple) != nullptr) {
			// targetActor->GetProperty()->SetRepresentationToWireframe(); //これでワイアフレーム化できる
			// NOTE 三角形ポリゴンのエッジを表示する場合。
			//	targetActor->GetProperty()->EdgeVisibilityOn();
			//	targetActor->GetProperty()->SetEdgeColor(1,0,1);
			const std::string &cname = std::get<0>(actorTuple);
			auto tmpActor = std::get<1>(actorTuple);
			int nrefPoints = std::get<2>(actorTuple);
			int nrealPoints = std::get<3>(actorTuple);
			renderer_->AddActor(tmpActor);
			cellObjMap_.at(cname).actor_ = tmpActor;
			cellObjMap_.at(cname).numRefPoints_ = nrefPoints;
			cellObjMap_.at(cname).numRealPoints_ = nrealPoints;
		}
	}



    timer.stop();
    mDebug() << "Polygon generation ended in." << timer.msec() << "msec";







	// カメラ
	if(isFirstCall_){ 	// 初回のみの処理
		cameraInfo_ = CameraInfo::getAutoAdjustedCameraInfo(geomConfig_.region(), qvtkWidget_->size());
		CameraInfo::setToCamera(cameraInfo_, renderer_->GetActiveCamera());
	} else if(cameraInfo_.hasData) {
		//mDebug() << "restore from previous camera=" << cameraInfo_.toQString();
		CameraInfo::setToCamera(cameraInfo_, renderer_->GetActiveCamera());
	}


	timer.clear();
	timer.start();
	// Widgetのアップデート関数を呼ぶ
    updateQVTKWidget(isFirstCall_);

	timer.stop();
	mDebug() << "Renderer updated in " << timer.msec() << "msec.";

	// updateが終了後処理
	// キャンセルされた場合は事後処理しない。
	if(!polyDataTuples.empty()) {
		isFirstCall_ = false;
		addingCellNames_.clear();
		removingCellNames_.clear();
		colorPane_->updateChangedState(false); // colorPaneの変更が反映されたのでchanged_をfalseに戻す。
	}

	ui->pushButtonUpdateView->setEnabled(true);
	emit updateGeomViewFinished();
}


// ここでいうonlyとはpolygonを更新しない、という意味
void GeometryViewer::updateViewColorOnly()
{
	renderer_->SetBackground(static_cast<double>(guiConfig_->bgColor3D.r)/255,
							 static_cast<double>(guiConfig_->bgColor3D.g)/255,
							 static_cast<double>(guiConfig_->bgColor3D.b)/255);
	// スカラバーは必ず更新関数を呼ぶ。更新するかはcolorPane次第。どうせ更新コストは安い。
	colorPane_->updateScalarBar(renderer_);
    // legendも更新
    if(ui->pushButtonEnableLegend->isChecked()) this->showMaterialLegend(true);
	// ############ fieldDataの取得とチェック
	std::shared_ptr<const fd::FieldColorData> fcData;
	try {
		fcData = colorPane_->fieldColorData();
	} catch (std::exception &e) {
		mWarning() << e.what();
		ui->pushButtonUpdateView->setEnabled(true);
		return;
	}



	// ここからポリゴン着色
	OperationInfo operationInfo = PolyPainter::info();
	operationInfo.numTargets = displayedCellNames_.size();
    operationInfo.numThreads = static_cast<size_t>(guiConfig_->cuiConfig.numThread);
	operationInfo.sleepMsec = 50;

	/*
	 * 今はRenderingWorkerの結果を引数にしてActorの生成・色つけをしているが、
	 * polygonの新規生成をせずに色だけ変えるのはどうすればよいか。
	 * ・セル名、polyData,サンプル点数のタプルが必要
	 *		セル名、サンプル点数はactorMap_にある。
	 *		polydataはactorから取得できるか？
	 * できる。
	 * vtkSmartPointer<vtkPolyData> part1 = vtkPolyData::SafeDownCast(actor->GetMapper()->GetInputAsDataSet());
	 * こんな感じ。ただしactorにinputがセットされたmapperがセットされていなければならない。
	 * セットされていない時のactor->GetMapper()返り値は？ ソースみると0が返りそう。if(Mapper)みたいな箇所があるから。
	 */
	std::vector<std::tuple<std::string, vtkSmartPointer<vtkPolyData>, int, int>> polyDataTuples;
	polyDataTuples.reserve(displayedCellNames_.size());
	for(const auto& cellName: displayedCellNames_) {
		// displayedなセルなのでactorは生成済みのはず
		assert(cellObjMap_.find(cellName) != cellObjMap_.end());
		assert(cellObjMap_.at(cellName).actor_ != nullptr);

		// actorは色変え前のものは削除しないと二重に表示される。
		renderer_->RemoveActor(cellObjMap_.at(cellName).actor_);

		const vtkSmartPointer<vtkActor> &actor = cellObjMap_.at(cellName).actor_;
		// 表示中セルなのでactorは当然inputをもったmapperを持っているはず
		assert(actor->GetMapper() != nullptr);
		assert(actor->GetMapper()->GetInputAsDataSet() != nullptr);
		vtkSmartPointer<vtkPolyData> poly = vtkPolyData::SafeDownCast(actor->GetMapper()->GetInputAsDataSet());
		polyDataTuples.emplace_back(cellName, poly, cellObjMap_.at(cellName).numRefPoints_, cellObjMap_.at(cellName).numRealPoints_);
	}

	auto actorTuples = ProceedOperation<PolyPainter>(operationInfo, polyDataTuples, &cellObjMap_,
													simulation_->getGeometry()->palette(),
													fcData, colorPane_->lookupTable());

	// レンダラへのセット
	for(const auto& actorTuple: actorTuples) {
		if(std::get<1>(actorTuple) != nullptr) {
			// targetActor->GetProperty()->SetRepresentationToWireframe(); //これでワイアフレーム化できる
			// NOTE 三角形ポリゴンのエッジを表示する場合。
			//	targetActor->GetProperty()->EdgeVisibilityOn();
			//	targetActor->GetProperty()->SetEdgeColor(1,0,1);
			const std::string &tmpCellName = std::get<0>(actorTuple);
			auto tmpActor = std::get<1>(actorTuple);
			renderer_->AddActor(tmpActor);
			cellObjMap_.at(tmpCellName).actor_ = tmpActor;
			cellObjMap_.at(tmpCellName).numRefPoints_ = std::get<2>(actorTuple);
		}
	}

	updateQVTKWidget(isFirstCall_);

}




void GeometryViewer::saveCameraState()
{
	// まだ一度も描画していない場合camera位置はまだ不定であるという問題があるのでチェックが必要。
	if(renderer_->IsActiveCameraCreated() == 1) {
		cameraInfo_ = CameraInfo::fromCamera(renderer_->GetActiveCamera());
		//mDebug() << "recorded current camera =" << QString::fromStdString(cameraInfo_.toString());
	}
}

void GeometryViewer::setCameraInfo(const CameraInfo &cinfo)
{
	//mDebug() << "Enter restoreDefaultCamera!!!!!!!!!!!!!!!";
	cameraInfo_ = cinfo;
	CameraInfo::setToCamera(cameraInfo_, renderer_->GetActiveCamera());
	renderer_->ResetCamera();
	qvtkWidget_->GetRenderWindow()->Render();  // これでも解決しない
	// ズームしてからこれを呼ぶと何故かevent発生まで暗いまま
}

void GeometryViewer::setZPlusCamera() {setCameraInfo(ZPLUS_CAMERA_INFO);}
void GeometryViewer::setZMinusCamera() {setCameraInfo(ZMINUS_CAMERA_INFO);}
void GeometryViewer::setYPlusCamera() {setCameraInfo(YPLUS_CAMERA_INFO);}
void GeometryViewer::setYMinusCamera() {setCameraInfo(YMINUS_CAMERA_INFO);}
void GeometryViewer::setXPlusCamera() {setCameraInfo(XPLUS_CAMERA_INFO);}
void GeometryViewer::setXMinusCamera() {setCameraInfo(XMINUS_CAMERA_INFO);}


// pointerから値コピーでarrayにデータを保存
template <class T, unsigned int N>
std::array<T, N> getArray(T* arg)
{
	std::array<T, N> retArray;
	for(size_t i = 0; i < N; ++i) retArray[i] = arg[i];
	return retArray;
}


//#include <vtkTransform.h>
void GeometryViewer::updateQVTKWidget(bool doResetCamera)
{
	/*
	 *  vtkRenderer::ResetCameraを呼ぶと可視物体が表示される。
	 * 但し可視領域が再計算され、カメラ位置も変わる。
	 * https://public.kitware.com/pipermail/vtkusers/2015-August/091998.html
	 * It resets the camera so that all visible objects in the renderer are displayed,
	 * so it will overwrite the position that you set.
	 */
	// (主に初回の描画では)カメラ自動調整で強制描画
	if(doResetCamera) renderer_->ResetCamera();

//	auto camera = renderer_->GetActiveCamera();
//	auto p = getArray<double, 3>(camera->GetPosition());
//	auto f = getArray<double, 3>(camera->GetFocalPoint());
//	auto c = getArray<double, 2>(camera->GetClippingRange());
//	auto vu = getArray<double, 3>(camera->GetViewUp());
//	double ag = camera->GetViewAngle();

//	camera = renderer_->GetActiveCamera();
//	renderer_->GetActiveCamera()->SetPosition(p[0], p[1], p[2]);  // 始点tの移動。
//	renderer_->GetActiveCamera()->SetFocalPoint(f[0], f[1], f[2]);
//	renderer_->GetActiveCamera()->SetViewUp(vu[0], vu[1], vu[2]);
//	renderer_->GetActiveCamera()->SetViewAngle(ag);


    // vtkRenderer::Render()はソース見ると直接呼ばずに
    // RenderWindow::Render()呼べ、みたいなこと書いてあるから不要。
    //
    //renderer_->Render();
//    qvtkWidget_->GetInteractor()->Render();
//    qvtkWidget_->GetInteractor()->ReInitialize();
	qvtkWidget_->update();
    qvtkWidget_->GetRenderWindow()->Render();


}



constexpr int MATERIAL_LEGENT_FONT_PT = 25;

void GeometryViewer::showMaterialLegend(bool checked)
{
    /*
     * 1.■が表示されない問題
     * フォントを再度ペインから選択可能なようにして、デフォルトをarialあたりにする。
     * utils::fontFilePath(const QFont &font)でフォントファイルを選択する。
     * SetFontFamily(VTK_FONT_FILE)が必要なのを忘れていた。
     *
     * mat-name-colorセクション対応
     */

    for(auto& lactor: legendActors_)if(lactor) renderer_->RemoveActor(lactor);
    legendActors_.clear();
    if(checked) {
        const img::CellColorPalette &palette = simulation_->getGeometry()->palette();
        std::vector<std::shared_ptr<const img::MaterialColorData>> matColDataVec;
        for(const auto& matColData: palette.materialColorDataList()) {
            //mDebug() << "Matname===" << matColData->matName();
            if(img::MaterialColorData::isUserDefinedColor(*matColData.get())) {
                matColDataVec.emplace_back(matColData);
            }
        }
        // 判例の表示順序が安定するように、ここでソートをかける。
        std::sort(matColDataVec.begin(), matColDataVec.end(),
                  [](const std::shared_ptr<const img::MaterialColorData> &mcd1,
                     const std::shared_ptr<const img::MaterialColorData> &mcd2) {
                    return mcd1->matName() < mcd2->matName();
                });


        for(size_t i = 0; i < matColDataVec.size(); ++i) {
            const std::shared_ptr<const img::MaterialColorData> &matColData = matColDataVec.at(i);
            const std::shared_ptr<const img::Color> &col = matColData->color();
            std::string legendString;
            legendString += std::string("\U000025A0 ") + matColData->matName() + " " + matColData->aliasName();
            //legendString += std::string(" ■ ") + matColData->matName() + " " + matColData->aliasName();

            // 環境ごとにU+25A0を表示可能なフォントファイルを返すような関数を作りたい。
            // VTKの組み込みでない普通のarial相当ならだいたいどのような環境にもあって
            // U+25A0を表示可能と思われる。
            // QFontMetrics(QFont("Helvetica")).inFont(QChar(0x25a)) は表示されなくてもtrueを返すからだめ。
            // QFontMetrics(QFont("Helvetica")).width(QChar(0x25a)) は表示されなくても16が返るからだめ。
            // 結論：コンフィグメニューで設定可能ななUIフォントを使う。
            std::string fontFile = utils::fontFilePath(guiConfig_->uiFont);
            auto actor = vtkSmartPointer<vtkTextActor>::New();
            actor->GetTextProperty()->SetFontFile(fontFile.c_str());
            actor->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
            /*
             *  vtkTextProperty::SetVerticalJustifitionToTop()はバグっていて機能しない。
             * 従ってvtkTextActor::SetPosition(int, int)を使用することになるが、
             * 上寄せをしようとするとresizeイベントごとに位置更新が必要になる。
             */
            int fsz = static_cast<int>(std::round(matColData->printSize()*MATERIAL_LEGENT_FONT_PT));
            actor->GetTextProperty()->SetFontSize(fsz);
            actor->GetTextProperty()->SetColor(col->rf(), col->gf(), col->bf());
            actor->SetInput(legendString.c_str());
            legendActors_.emplace_back(actor);
        }
        for(auto &actor: legendActors_) renderer_->AddActor(actor);
    } else {
        legendActors_.clear();
    }
    this->updateMaterialLegendPosition();
    this->updateQVTKWidget(false);
}

void GeometryViewer::updateMaterialLegendPosition()
{
    // actorの指定する座標系はQVTKOpenGLWidgetの左下を原点とする座標系。
    // ただし指定するのはレジェンドの左下隅の点であることに注意。
    int xpos, ypos;
    QSize sz = qvtkWidget_->size();
    constexpr int OFFSET = 20;
    xpos = 10;
    ypos = sz.height() - OFFSET;
    for(const auto& actor:legendActors_) {
        ypos -= actor->GetTextProperty()->GetFontSize();
    }

    // (xpos, ypos)が基準点なのでそこから上に表示していく
    for(int i = static_cast<int>(legendActors_.size())-1 ; i >= 0; --i) {
        legendActors_.at(static_cast<size_t>(i))->SetPosition(xpos, ypos);
        ypos += legendActors_.at(static_cast<size_t>(i))->GetTextProperty()->GetFontSize();
    }
}



