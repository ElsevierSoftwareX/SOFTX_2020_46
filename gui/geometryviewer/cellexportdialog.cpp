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
#include "cellexportdialog.hpp"
#include "ui_cellexportdialog.h"



std::pair<double, bool> CellExportDialog::getExportingInfo(QWidget *parent, bool *ok)
{
	CellExportDialog ced(parent);
	auto result = ced.exec();

	*ok = (result == QDialog::Accepted);
	return std::make_pair(ced.ui->doubleSpinBoxScalingFactor->value(),
						  ced.ui->checkBoxUnifyCells->isChecked());
}



CellExportDialog::CellExportDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CellExportDialog)
{
	ui->setupUi(this);
}

CellExportDialog::~CellExportDialog()
{
	delete ui;
}


