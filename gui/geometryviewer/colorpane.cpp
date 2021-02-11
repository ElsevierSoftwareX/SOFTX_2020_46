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
#include "colorpane.hpp"
#include "ui_colorpane.h"

#include <cassert>
#include <fstream>
#include <regex>
#include <sstream>

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>

#include "../../core/fielddata/xyzmeshtallydata.hpp"
#include "../../core/fielddata/fieldcolordata.hpp"
#include "../../core/utils/system_utils.hpp"
#include "../../core/utils/string_utils.hpp"
#include "../../core/utils/message.hpp"
#include "../subdialog/messagebox.hpp"

namespace {
const double DEFAULT_BAR_WIDTH = 0.1;
const double DEFAULT_BAR_HEIGHT = 0.8;

const double DEFAULT_VBAR_XPOS = 1 - 1.5*DEFAULT_BAR_WIDTH;
const double DEFAULT_VBAR_YPOS = DEFAULT_BAR_WIDTH;
const double DEFAULT_HBAR_XPOS = DEFAULT_BAR_WIDTH - 0.05;
const double DEFAULT_HBAR_YPOS = DEFAULT_BAR_WIDTH - 0.05;

}

ColorPane::ColorPane(QWidget *parent) :
	QWidget(parent),
    ui(new Ui::ColorPane), changed_(false),
    scalarBar_(vtkSmartPointer<vtkScalarBarActor>::New()),
    lookupTable_(vtkSmartPointer<vtkLookupTable>::New())
{
	ui->setupUi(this);
    //lookupTable_->SetTableRange (0, 0.05);
    lookupTable_->SetHueRange (0.667, 0);
    lookupTable_->SetSaturationRange (1, 1);
    lookupTable_->SetValueRange (1, 1);
    lookupTable_->Build();

    scalarBar_->SetLookupTable(lookupTable_);
	scalarBar_->SetWidth(DEFAULT_BAR_WIDTH);
	scalarBar_->SetHeight(DEFAULT_BAR_HEIGHT);


	ui->doubleSpinBoxBarWidth->setValue(scalarBar_->GetWidth());
	ui->doubleSpinBoxBarHeight->setValue(scalarBar_->GetHeight());

	// ファイルを有効化するかはcheckBoxUseFileにかかっている
	connect(ui->checkBoxUseFile, &QCheckBox::toggled, this, &ColorPane::enableWidgets);
	// 更新通知のchanged_変更はuiファイルの方でconnectする。（読み込み成功時のみこのファイルで対応。uiでは読み込み成功不成功はわからないから）
	// Legend関係はchanged_にカウントしない。legendはupdateボタン押すと必ず更新にするから。

	connect(ui->groupBoxLegend, &QGroupBox::toggled, this, &ColorPane::requestShowLegend);
}
ColorPane::~ColorPane() {delete ui;}

QString ColorPane::scalarBarTitle() const {return ui->lineEditLegendTitle->text();}
int ColorPane::numTics() const {return ui->spinBoxNumTics->value();}
bool ColorPane::useFieldDataChecked() const {return !currentDataAbsFilePath_.isEmpty();}
bool ColorPane::isScalarBarEnabled() const {return ui->groupBoxLegend->isChecked();}

std::shared_ptr<const fd::FieldColorData> ColorPane::fieldColorData() const
{
    if(!ui->checkBoxUseFile->isChecked()) return std::shared_ptr<const fd::FieldColorData>();

	std::shared_ptr<double> lower, upper;
	decltype(meshData_) meshData;
	bool isLog = false;
	bool useInterpolation = false;
	if(!currentDataAbsFilePath_.isEmpty()) {
		meshData = meshData_;
		isLog = ui->checkBoxLogScale->isChecked();
		useInterpolation = ui->checkBoxInterpolation->isChecked();

		QString lowerStr = ui->lineEditRangeMin->text();
		QString upperStr = ui->lineEditRangeMax->text();
		bool ok = true;

		if(!lowerStr.isEmpty()) {
			double lowerValue = lowerStr.toDouble(&ok);
			if(!ok) {
				mWarning() << "Field data's lower limit contains invalid character(s) str=" << lowerStr.toStdString();
			} else {
				lower = std::make_shared<double>(lowerValue);
			}
		}
		if(!upperStr.isEmpty()) {
			double upperValue = upperStr.toDouble(&ok);
			if(!ok) {
				mWarning() << "Field data's upper limit contains invalid character(s) str=" << upperStr.toStdString();
			} else {
				upper = std::make_shared<double>(upperValue);
				mDebug() << "user-defined upper bound===" << *upper.get();
			}
		}
        // ここからlookupTableの更新 FIXME ここでよいか？またこの部分は別関数が良い
        double tablelower = (lower) ? *lower.get() : meshData_->getMin();
        double tableupper = (upper)? *upper.get() : meshData_->getMax();
        // 下限と上限の大きさチェック
        if(tablelower >= tableupper) {
            mWarning() << "For field data color, lower bound is larger than upper bound. lower, upper="
                       << tablelower << tableupper;
            tablelower = meshData_->getMin();
            tableupper = meshData_->getMax();
            mWarning() << "Auto scale was used instead. low, up=" << tablelower << tableupper;
        }
        if(isLog) {
            if(tablelower < fd::FieldColorData::SMALL_FIELD_DATA) tablelower = fd::FieldColorData::SMALL_FIELD_DATA;
            tablelower = std::log10(tablelower);
            tableupper = std::log10(tableupper);
        }
        //mDebug() << "lookupTable, lower, upper====" << lower << upper;
        lookupTable_->SetTableRange (tablelower, tableupper);
        lookupTable_->Build();
	}
//    return std::make_shared<fd::FieldColorData>(meshData, std::make_pair(lower, upper), isLog, useInterpolation);
	return std::make_shared<fd::FieldColorData>(meshData, isLog, useInterpolation);
}

void ColorPane::clear()
{
	meshData_.reset();
	currentDataAbsFilePath_.clear();
	ui->labelFieldDataInfo->setText("");
    ui->labelFieldFileName->setText("");
	enableWidgets(false);
	ui->checkBoxUseFile->setChecked(false);
}


// meshtalファイルから適当なグリッドデータを作成
void ColorPane::calcXyzMeshData()
{
    try {
        meshData_ = fd::XyzMeshTallyData::createXyzMeshTallyData(currentDataAbsFilePath_.toStdString());
    } catch (std::exception &e) {
        std::stringstream ss;
        ss << currentDataAbsFilePath_.toStdString()
           << " is not a valid xyz mesh tally file. reason = " << e.what();
        GMessageBox::critical(this, tr("critical"), QString::fromStdString(ss.str()), true);
    }

}

// ファイル選択ボタン以外の有効化無効化
void ColorPane::enableWidgets(bool flag)
{
	ui->groupBoxUserDefinedRange->setDisabled(!flag);
	ui->groupBoxLegend->setDisabled(!flag);
	ui->checkBoxLogScale->setDisabled(!flag);
	ui->checkBoxInterpolation->setDisabled(!flag);
	ui->pushButtonClearFile->setDisabled(!flag);
	ui->labelDataInfo->setDisabled(!flag);
	ui->labelFieldDataInfo->setDisabled(!flag);
	ui->labelFieldFileName->setDisabled(!flag);
}

void ColorPane::selectFile()
{
	QString meshTalFile  = QFileDialog::getOpenFileName(this, tr("Select field (mesh tally) file"), currentDataAbsFilePath_);
	if(currentDataAbsFilePath_ == meshTalFile) return;
	currentDataAbsFilePath_ = meshTalFile;
	/*
	 * ファイル選択時にenableするのはcheckBoxUseFileだけ。
	 * checkBoxUseFileがチェックされたら他widgetをenableする。
	 * QFileDialogがキャンセルされた場合何もせずにリターンは↑でk実施済みなのでここで考える必要はない。
	 */
	if(currentDataAbsFilePath_.isEmpty()) {
		ui->checkBoxUseFile->setEnabled(false);
		return;
   } else {
		setChanged();
		ui->checkBoxUseFile->setEnabled(true);
		ui->checkBoxUseFile->setChecked(true);
		if(ui->checkBoxUseFile->isChecked()) enableWidgets(true);
   }

	if(utils::isRelativePath(currentDataAbsFilePath_.toStdString())) {
        ui->labelFieldFileName->setText(currentDataAbsFilePath_);
        currentDataAbsFilePath_ =  QFileInfo(QDir("."), currentDataAbsFilePath_).absoluteFilePath();
    } else {
        ui->labelFieldFileName->setText(QDir(".").relativeFilePath(currentDataAbsFilePath_));
    }

    calcXyzMeshData();

    ui->labelFieldDataInfo->setText(QString::fromStdString(meshData_->info()));
}

void ColorPane::setChanged() {changed_ = true;}
void ColorPane::updateChangedState(bool state) {changed_ = state;}

#include <vtkTextProperty.h>
bool ColorPane::updateScalarBar(vtkSmartPointer<vtkRenderer> renderer)
{
	//  SetUnconstrainedしないと horizontal時にフォントサイズがでかくなる。
	int fontSize = ui->spinBoxFontSize->value();
	if(fontSize > 0) {
		scalarBar_->GetLabelTextProperty()->SetFontSize(fontSize);
        scalarBar_->GetTitleTextProperty()->SetFontSize(fontSize);
        scalarBar_->SetUnconstrainedFontSize(true);  // fontsize固定
	}

    if(ui->checkBoxUseFile->isChecked() && !currentDataAbsFilePath_.isEmpty() && ui->groupBoxLegend->isChecked()) {
        renderer->RemoveActor2D(scalarBar_);
		scalarBar_->SetLookupTable(lookupTable_);
        auto titleStr = ui->lineEditLegendTitle->text().toStdString();
        if(ui->checkBoxLogScale->isChecked()) titleStr += " log10";
        scalarBar_->SetTitle(titleStr.c_str());
		scalarBar_->SetNumberOfLabels(ui->spinBoxNumTics->value());
		scalarBar_->SetWidth(ui->doubleSpinBoxBarWidth->value());
		scalarBar_->SetHeight(ui->doubleSpinBoxBarHeight->value());

		// TODO verticalでも幅を広げると短くなる。
		if(ui->radioButtonScalarBarV->isChecked()) {
            scalarBar_->SetOrientationToVertical();
			scalarBar_->GetPositionCoordinate()->SetValue(DEFAULT_VBAR_XPOS, DEFAULT_VBAR_YPOS);

        } else {
            scalarBar_->SetOrientationToHorizontal();
			scalarBar_->GetPositionCoordinate()->SetValue(DEFAULT_HBAR_XPOS, DEFAULT_HBAR_YPOS);
			scalarBar_->GetPosition2Coordinate()->SetValue(DEFAULT_HBAR_XPOS + scalarBar_->GetHeight(), DEFAULT_HBAR_YPOS + scalarBar_->GetWidth());
       }

        renderer->AddActor2D(scalarBar_);
		return true;
    } else {
        renderer->RemoveActor2D(scalarBar_);
		return false;
	}
}

void ColorPane::retranslate()
{
	ui->retranslateUi(this);
}


