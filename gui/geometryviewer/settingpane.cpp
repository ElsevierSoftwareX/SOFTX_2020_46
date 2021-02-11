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
#include "settingpane.hpp"
#include "ui_settingpane.h"

#include <cmath>
#include <vtkTextProperty.h>

#include "cellpane.hpp"
#include "../subdialog/messagebox.hpp"
#include "../../core/geometry/cell/boundingbox.hpp"
#include "../../core/utils/message.hpp"
#include "../../core/utils/numeric_utils.hpp"

namespace {
//const double DEFAULT_NUM_PTS_CELL = 1000;
const double DEFAULT_NUM_SAMPLES = 5E+6; // 1Msamples total in default setting
}

const int SettingPane::DEFAULT_BB_SIZE = 50;



SettingPane::SettingPane(QWidget *parent, vtkSmartPointer<vtkRenderer> renderer) :
	QWidget(parent), ui(new Ui::SettingPane), renderer_(renderer),
	arrowAxesActor_(vtkSmartPointer<vtkAxesActor>::New()),
	cubeAxesActor_(vtkSmartPointer<vtkCubeAxesActor>::New()),
	auxPlaneActors_({vtkSmartPointer<vtkActor>::New(),
					vtkSmartPointer<vtkActor>::New(),
					vtkSmartPointer<vtkActor>::New()})
{
	ui->setupUi(this);
	GeometryViewerConfig defaultConfig;
//	ui->horizontalSliderRate->setValue(ui->spinBoxSampling->value());
	ui->lineEditPointsPerCell->setText(QString::number(defaultConfig.numPoints()));
	// ADVANCED設定値
	ui->doubleSpinBoxSmoothingFactor->setValue(defaultConfig.smoothingFactor());
//	ui->doubleSpinBoxSmoothingFactor->setDisabled(true);
	ui->spinBoxMemorySafetyFactor->setValue(defaultConfig.memoryUsagePercentLimit());

	// AXES
	cubeAxesActor_->SetCamera(renderer_->GetActiveCamera());
	cubeAxesActor_->GetTitleTextProperty(0)->SetColor(1.0, 0.0, 0.0);
	cubeAxesActor_->GetLabelTextProperty(0)->SetColor(1.0, 0.0, 0.0);
	cubeAxesActor_->GetTitleTextProperty(1)->SetColor(0.0, 1.0, 0.0);
	cubeAxesActor_->GetLabelTextProperty(1)->SetColor(0.0, 1.0, 0.0);
	cubeAxesActor_->GetTitleTextProperty(2)->SetColor(0.0, 0.0, 1.0);
	cubeAxesActor_->GetLabelTextProperty(2)->SetColor(0.0, 0.0, 1.0);
	cubeAxesActor_->DrawXGridlinesOn();
	cubeAxesActor_->DrawYGridlinesOn();
	cubeAxesActor_->DrawZGridlinesOn();
	cubeAxesActor_->SetGridLineLocation(cubeAxesActor_->VTK_GRID_LINES_FURTHEST);
	cubeAxesActor_->XAxisMinorTickVisibilityOff();
	cubeAxesActor_->YAxisMinorTickVisibilityOff();
	cubeAxesActor_->ZAxisMinorTickVisibilityOff();

	// arrowをレンダリング領域にフィットさせる場合はdoublespinboxは無効化する
	ui->doubleSpinBoxArrowLength->setDisabled(ui->checkBoxFitArrowToRenderingRegion->isChecked());

	// ↓ これ結局this->updatePaneへ繋がれているけど、SettingPaneでは現在の最大BBがわからないからGeometryViewerを経由する。
	connect(ui->pushButtonAutoCalc, &QPushButton::pressed, this, &SettingPane::requestAutoCalcRegion);
}

SettingPane::~SettingPane()
{
	delete ui;
}

void SettingPane::retranslate() {ui->retranslateUi(this);}

// ペインの情報を更新する。
// regionのセットと
void SettingPane::updatePane(const geom::BoundingBox &maxbb)
{
	// 描画したいセルのBBが0というのは十分にあり得る。
	setRegionByBB(maxbb);
//    // ここで描画領域が確定したのでそれに合わせて分解能を設定する。
//    setAutomaticResolution();
}

void SettingPane::showCubeAxes(bool on)
{
	renderer_->RemoveActor(cubeAxesActor_);
	if(on) {
		std::array<double, 6> ranges;
		if(getRenderingRangeInput(true, &ranges)) {
			// Actorは一々newする必要はないはず。QVTKWidgetが更新されれば。
//			cubeAxesActor_ = vtkSmartPointer<vtkCubeAxesActor>::New();
//			initCubeAxesActor();
			cubeAxesActor_->SetBounds(ranges[0], ranges[1], ranges[2], ranges[3], ranges[4], ranges[5]);
			renderer_->AddActor(cubeAxesActor_);
		}
	}
	emit requestUpdateQVTKWidget(); // QVTKWidget更新関数が呼ばれないと描画には反映されない。
}

void SettingPane::showArrows(bool on)
{
	renderer_->RemoveActor(arrowAxesActor_);
	if(on){
		std::array<double, 3> arrowLengths{0, 0, 0};
		if(ui->checkBoxFitArrowToRenderingRegion->isChecked()) {
			std::array<double, 6> ranges;
			if(getRenderingRangeInput(true, &ranges)) {
				ranges[0] = ranges[2] = ranges[4] = 0;
				arrowLengths = decltype(arrowLengths){ranges[1] - ranges[0], ranges[3] - ranges[2], ranges[5] - ranges[4]};
			}
		} else {
			auto len = ui->doubleSpinBoxArrowLength->value();
			arrowLengths = decltype(arrowLengths){len, len, len};
		}

		if(arrowLengths[0] < 0 || arrowLengths[1] < 0 || arrowLengths[2] < 0) {
			// 矢印長さが負の場合描画領域にフィットさせない(できない)
			auto len = ui->doubleSpinBoxArrowLength->value();
			arrowLengths = decltype(arrowLengths){len, len, len};
			ui->checkBoxFitArrowToRenderingRegion->setChecked(false);
		}
		// ちゃんとQVTKWidgetがupdateされるのならActorは一々delte-and-newする必要は無いはず
		arrowAxesActor_->SetTotalLength(arrowLengths[0], arrowLengths[1], arrowLengths[2]);
		arrowAxesActor_->SetConeRadius(ui->doubleSpinBoxHeadRadius->value());
		renderer_->AddActor(arrowAxesActor_);
	}
    emit requestUpdateQVTKWidget();
}




#include <QSignalBlocker>
void SettingPane::setRenderingRegion(const std::array<double, 6> &arr)
{
	// GUIでRenderingRegionを変更する時以外はsignalをブロクする。
	QSignalBlocker blocker0(ui->lineEditRenderringRegionXmin);
	QSignalBlocker blocker1(ui->lineEditRenderringRegionXmax);
	QSignalBlocker blocker2(ui->lineEditRenderringRegionYmin);
	QSignalBlocker blocker3(ui->lineEditRenderringRegionYmax);
	QSignalBlocker blocker4(ui->lineEditRenderringRegionZmin);
	QSignalBlocker blocker5(ui->lineEditRenderringRegionZmax);
	ui->lineEditRenderringRegionXmin->setText(QString::number(arr[0]));
	ui->lineEditRenderringRegionXmax->setText(QString::number(arr[1]));
	ui->lineEditRenderringRegionYmin->setText(QString::number(arr[2]));
	ui->lineEditRenderringRegionYmax->setText(QString::number(arr[3]));
	ui->lineEditRenderringRegionZmin->setText(QString::number(arr[4]));
    ui->lineEditRenderringRegionZmax->setText(QString::number(arr[5]));
	Q_UNUSED(blocker0);
	Q_UNUSED(blocker1);
	Q_UNUSED(blocker2);
	Q_UNUSED(blocker3);
	Q_UNUSED(blocker4);
	Q_UNUSED(blocker5);
}

//void SettingPane::setAutomaticResolution()
//{
//	// 自動設定を適用すると、解像度の値は変わる。
//	// 今解像度の値が変わると、valueChangedがemitされ、
//	// valueChangedシグナルは解像度自動設定チェックボックスを外すように繋がれているため、
//	// 自動設定を適用すると解像度自動設定チェックが外れてしまう。
//	// これを解決するために、setAutomaticResolution()では、
//	// valueChangedを一旦ブロックして、変更後ブロック解除する
//	ui->spinBoxSampling->blockSignals(true);
//	ui->spinBoxRateMagnification->blockSignals(true);
//	try{
//		std::array<double, 6> arr;
//		if(this->getRenderingRangeInput(false, &arr)) {
//			ui->horizontalSliderRate->setValue(20);
//			double volume = (arr[1]-arr[0])*(arr[3]-arr[2])*(arr[5]-arr[4]);  // cm3
//			if(!utils::isSameDouble(0, volume)) {
//				// Num points = (slider*val/100)^3 * volume.  100 is cm/m
//				double val = 100/ui->horizontalSliderRate->value()*std::pow(DEFAULT_NUM_SAMPLES/volume, 0.33333);
//				//mDebug() << "val=" << val;
//				ui->spinBoxRateMagnification->setValue(std::max(1, static_cast<int>(val)));
//			}
//		}
//	} catch (...) {
//		// blockしたらなにがあっても必ずブロック解除しなければならない
//		ui->spinBoxSampling->setValue(ui->horizontalSliderRate->value());
//		ui->spinBoxRateMagnification->blockSignals(false);
//		ui->spinBoxSampling->blockSignals(false);
//		throw;  // ブロック解除したら再度投げる
//	}
//	ui->spinBoxSampling->setValue(ui->horizontalSliderRate->value());
//	ui->spinBoxRateMagnification->blockSignals(false);
//	ui->spinBoxSampling->blockSignals(false);
//}



void SettingPane::setRegionByBB(const geom::BoundingBox &maxbb)
{
	using BB = geom::BoundingBox;
	std::array<double, 6> range = maxbb.range();
	static const std::vector<std::string> dirStrs{"-x", "+x", "-y", "+y", "-z", "+z"};
	// BBに無限大(計算不能)な部分があった場合規定の最大値に丸める
	for(size_t i = 0; i < 6; ++i) {
		if(std::abs(range.at(i)) > 0.1*BB::MAX_EXTENT) {
			range.at(i) = std::pow(-1, (i+1)%2)*DEFAULT_BB_SIZE;
			mWarning() << "Estimation of rendering region in " << dirStrs.at(i)
					   << " direction failed , applied =" << range.at(i);
		}
	}
	this->setRenderingRegion(range);
}

GeometryViewerConfig SettingPane::getGeomConfig()
{
	std::array<double, 6> region;
	getRenderingRangeInput(false, &region);


	static auto func = [](QRadioButton *b1, QRadioButton* b2) {
		if(b1->isChecked()){
			return 1;
		} else if(b2->isChecked()) {
			return -1;
		} else {
			return 0;
		}
	};

	int cutx = func(ui->radioButtonXplaneCutPlus, ui->radioButtonXplaneCutMinus);
	int cuty = func(ui->radioButtonYplaneCutPlus, ui->radioButtonYplaneCutMinus);
	int cutz = func(ui->radioButtonZplaneCutPlus, ui->radioButtonZplaneCutMinus);
	math::Vector<3> xvec{1, 0, 0}, yvec{0, 1, 0}, zvec{0, 0, 1};
	std::array<PlaneInfo, 3> info{
		PlaneInfo (ui->checkBoxAuxXplane->isChecked(), xvec, ui->lineEditAuxXplanePos->text().toDouble(), cutx),
		PlaneInfo (ui->checkBoxAuxYplane->isChecked(), yvec, ui->lineEditAuxYplanePos->text().toDouble(), cuty),
		PlaneInfo (ui->checkBoxAuxZplane->isChecked(), zvec, ui->lineEditAuxZplanePos->text().toDouble(), cutz)
	};
	double smFactor = ui->doubleSpinBoxSmoothingFactor->value();
	int msFactor = ui->spinBoxMemorySafetyFactor->value();
	return GeometryViewerConfig(numberOfPoints(), region, info, smFactor, msFactor);
}


size_t SettingPane::numberOfPoints()
{
	static const QString DEFAULT_MESSAGE = "\nDefault value("+ QString::number(GeometryViewerConfig().numPoints()) + ") was applied";
	bool ok = false;
	QString numStr = ui->lineEditPointsPerCell->text();
	double numPoints = numStr.toDouble(&ok);
	if(!ok) {
		QString message = "Number of sampling points should be a number, ex 10000 or 1e+4. input = "
							+ numStr + DEFAULT_MESSAGE;
		GMessageBox::warning(Q_NULLPTR, tr("warning"), message, true);
		numPoints = GeometryViewerConfig().numPoints();
		ui->lineEditPointsPerCell->setText(QString::number(numPoints));
	}

	if(numPoints < 1) {
		numPoints = GeometryViewerConfig().numPoints();
		QString message = "Number of sampling points should be > 1. current = "
				+ QString::number(numPoints) + DEFAULT_MESSAGE;
		GMessageBox::warning(Q_NULLPTR, tr("warning"), message, true);
	}
	if(numPoints > std::numeric_limits<size_t>::max()) {
		numPoints = GeometryViewerConfig().numPoints();
		QString message = "Number of sampling points is too large, should be < "
				+ QString::number(std::numeric_limits<size_t>::max())
				+ ". current = " + QString::number(numPoints) + DEFAULT_MESSAGE;
		GMessageBox::warning(Q_NULLPTR, tr("warning"), message, true);
	}
	return static_cast<size_t>(numPoints);
}

int SettingPane::minimumNoScrollWidth() const
{
	// 一番長いのはsliderの部分
	int padsize = 15;
	return ui->lineEditPointsPerCell->width() + padsize*2;
}


#include <vtkPlane.h>
#include <vtkSampleFunction.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

void SettingPane::updateAuxPlanes()
{
	/*
	 * 疑問1：addしていないactorをremoveしようとしたらどうなるか
	 * → vtkViewPort::RemoveViewPropと実質同じ
	 *	→ "Remove a prop from the list of props.Does nothing if the prop is not already present."
	 *     と書かれているのでremoveしようとしてもなにも起こらないだけ。
	 *
	 * 疑問2：addしたままactorを操作してよいか（＝反映されるか？)
	 *
	 */
	for(const auto& planeActor: auxPlaneActors_) {
		renderer_->RemoveActor(planeActor);
	}

	auto tmpGeomConfig = this->getGeomConfig();
	auto bb = geom::BoundingBox(tmpGeomConfig.region());
	bb.expand(2);
	using Vec = math::Vector<3>;
    std::vector<Vec> normals{Vec{1, 0, 0}, Vec{0, 1, 0}, Vec{0, 0, 1}};
    std::vector<const QLineEdit*> auxLineEdits{ui->lineEditAuxXplanePos, ui->lineEditAuxYplanePos, ui->lineEditAuxZplanePos};
	std::vector<QCheckBox*> auxCheckBoxes{ui->checkBoxAuxXplane, ui->checkBoxAuxYplane, ui->checkBoxAuxZplane};
    std::vector<QRadioButton*> auxNocutChecks{ui->radioButtonXplaneNoCut, ui->radioButtonYplaneNoCut, ui->radioButtonZplaneNoCut};
    math::Point regionCenter = bb.center();
	for(size_t i = 0; i < auxPlaneActors_.size(); ++i) {
		if(!auxCheckBoxes.at(i)->isChecked() || !auxNocutChecks.at(i)->isChecked()) continue;
		auto plane = vtkSmartPointer<vtkPlane>::New();
        auto center = regionCenter;
        bool ok = false;
        double pos = auxLineEdits.at(i)->text().toDouble(&ok);
        if(!ok) {
            QString err = "Entry for the aux plane position is not a number. str = "
                    + auxLineEdits.at(i)->text();
            mWarning() << err;
            GMessageBox::warning(Q_NULLPTR, tr("warning"), err, true);
            continue;
        }
        center[i] = pos;
        plane->SetOrigin(center.x(), center.y(), center.z());
		plane->SetNormal(normals.at(i).x(), normals.at(i).y(), normals.at(i).z());
		auto sample = vtkSmartPointer<vtkSampleFunction>::New();
		//sample->SetImplicitFunction(functionMap_.at(*it));
		sample->SetImplicitFunction(plane);
		double *bd = bb.range().data();
		sample->SetModelBounds(bd);
		sample->ComputeNormalsOff();
		sample->SetSampleDimensions(10, 10, 10);
		sample->Update();
		// contour
		auto surface = vtkSmartPointer<vtkContourFilter>::New();
		surface->SetInputConnection(sample->GetOutputPort());
		surface->SetValue(0, 0.0);
		surface->Update();
		auto polyData = vtkSmartPointer<vtkPolyData>::New();
		polyData->DeepCopy(surface->GetOutput());
		// mapper
		auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		mapper->SetInputData(polyData);
		mapper->ScalarVisibilityOff();
		// actor
		auto actor = vtkSmartPointer<vtkActor>::New();
		actor->SetMapper(mapper);
		actor->GetProperty()->SetOpacity(0.85);
		auxPlaneActors_.at(i) = actor;
		renderer_->AddActor(auxPlaneActors_.at(i));
	}

//	for(const auto &planeActor: auxPlaneActors_) {
//		renderer_->AddActor(planeActor);
//		// NOTE actor->GetProperty()->SetRepresentationToWireframe(); これを実行するとワイヤーフレーム表示になる。
//		// ・renderer_->AddActor(pinfo.createActor(bb));
//	}
}

void SettingPane::clear()
{

	renderer_->RemoveActor(arrowAxesActor_);
	renderer_->RemoveActor(cubeAxesActor_);
    for(const auto& pa: auxPlaneActors_) {
        renderer_->RemoveActor(pa);
    }
    ui->checkBoxAuxXplane->setChecked(false);
    ui->checkBoxAuxYplane->setChecked(false);
    ui->checkBoxAuxZplane->setChecked(false);
    ui->radioButtonXplaneCutPlus->setChecked(false);
    ui->radioButtonYplaneCutPlus->setChecked(false);
    ui->radioButtonZplaneCutPlus->setChecked(false);
    ui->radioButtonXplaneCutMinus->setChecked(false);
    ui->radioButtonYplaneCutMinus->setChecked(false);
    ui->radioButtonZplaneCutMinus->setChecked(false);
    ui->radioButtonXplaneNoCut->setChecked(true);
    ui->radioButtonYplaneNoCut->setChecked(true);
    ui->radioButtonZplaneNoCut->setChecked(true);

	GeometryViewerConfig gconf;
	ui->lineEditPointsPerCell->setText(QString::number(gconf.numPoints()));
	ui->spinBoxMemorySafetyFactor->setValue(gconf.memoryUsagePercentLimit());
	ui->doubleSpinBoxSmoothingFactor->setValue(gconf.smoothingFactor());
	auto range = gconf.region();
	ui->lineEditRenderringRegionXmin->setText(QString::number(range.at(0)));
	ui->lineEditRenderringRegionXmax->setText(QString::number(range.at(1)));
	ui->lineEditRenderringRegionYmin->setText(QString::number(range.at(2)));
	ui->lineEditRenderringRegionYmax->setText(QString::number(range.at(3)));
	ui->lineEditRenderringRegionZmin->setText(QString::number(range.at(4)));
	ui->lineEditRenderringRegionZmax->setText(QString::number(range.at(5)));

}



void SettingPane::setEnableArrowConfig(bool state)
{
	if(!ui->checkBoxFitArrowToRenderingRegion->isChecked()) {
		ui->labelArrowLength->setEnabled(state);
		ui->doubleSpinBoxArrowLength->setEnabled(state);
	}
	ui->doubleSpinBoxHeadRadius->setEnabled(state);
	ui->labelArrowHeadRadius->setEnabled(state);
	ui->checkBoxFitArrowToRenderingRegion->setEnabled(state);
}

void SettingPane::emitDisableAutoResolution() {	emit requestDisableAutoResolution();}

void SettingPane::emitDisableAutoRegion() {	emit requestDisableAutoRegion();}

/*!
 * LineEdit群に入力されている文字列から描画領域データを取得する。
 * LineEdit群に数値化できない文字列が入っている場合取得に失敗しfalseを返す。
 * @param[in] (warn) 失敗時MessageBoxで警告を出すフラグ
 * @param[out] (rangeArray) 取得結果を格納するstd::array<double,6>*
 * @return 領域データの取得に成功したらtrue
 */
bool SettingPane::getRenderingRangeInput(bool warn, std::array<double, 6> *rangeArray) const
{
	std::array<bool, 6> valueOks;
	std::array<std::string, 6> dirStrs{"xmin", "xmax", "ymin", "ymax", "zmin", "zmax"};
	std::array<QLineEdit*, 6> rangeEdits {
		ui->lineEditRenderringRegionXmin, ui->lineEditRenderringRegionXmax,
		ui->lineEditRenderringRegionYmin, ui->lineEditRenderringRegionYmax,
		ui->lineEditRenderringRegionZmin, ui->lineEditRenderringRegionZmax,
	};
	for(size_t i = 0; i < valueOks.size(); ++i) {
		rangeArray->at(i) = rangeEdits.at(i)->text().toDouble(&valueOks[i]);
		if(!valueOks.at(i)) {
			rangeArray->at(i) = -1*std::pow(-1, i)*DEFAULT_BB_SIZE;
			QString message = tr("Input for rendering region is invalid.")
					+ " input = " + rangeEdits.at(i)->text()
					+ " applied = " + QString::number(rangeArray->at(i));
			if(warn) GMessageBox::warning(Q_NULLPTR, tr("Warning"), message, true);
			// lineEditにも反映させる
			rangeEdits.at(i)->setText(QString::number(rangeArray->at(i)));
		}
	}
	// xmin,xmaxの大小関係も要チェック
	for(size_t i = 0; i < valueOks.size(); i+=2) {
		if(rangeArray->at(i) > rangeArray->at(i+1)) {
			double newMin = -1*std::pow(-1, i)*DEFAULT_BB_SIZE, newMax = std::pow(-1, i)*DEFAULT_BB_SIZE;
			QString message = tr("Upper limit is smaller than lower one. Default values applied\n")
					+ "(lower, upper) = (" + QString::number(rangeArray->at(i))
					+ ", " + QString::number(rangeArray->at(i+1)) + "),  "
					+ "applied = (" + QString::number(newMin) + ", " + QString::number(newMax) + ")";
			if(warn) GMessageBox::warning(Q_NULLPTR, tr("Warning"), message, true);
			rangeArray->at(i) = newMin;
			rangeArray->at(i+1) = newMax;
		}
	}
	assert(rangeArray->at(0) < rangeArray->at(1));
	assert(rangeArray->at(2) < rangeArray->at(3));
	assert(rangeArray->at(4) < rangeArray->at(5));

	// 結局入力エラーはデフォルト値適用するようにしたので常にtrueが返ることにした。つまり返り値いらないんじゃね？
	return true;

//	if(std::count(valueOks.begin(), valueOks.end(), false) != 0) {
//        if(warn) GMessageBox::warning(Q_NULLPTR, tr("Warning"), tr("Input for rendering region is invalid"), true);
//		*rangeArray = std::array<double, 6>{0, 0, 0, 0, 0, 0};
//		return false;
//	} else {
//		*rangeArray = std::array<double, 6>{xmin, xmax, ymin, ymax, zmin, zmax};
//		return true;
//	}
}
