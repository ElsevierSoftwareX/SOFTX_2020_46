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
#ifndef CELLEXPORTDIALOG_HPP
#define CELLEXPORTDIALOG_HPP

#include <utility>
#include <QDialog>

namespace Ui {
class CellExportDialog;
}

class CellExportDialog : public QDialog
{
	Q_OBJECT

public:
	static std::pair<double, bool> getExportingInfo(QWidget *parent, bool *ok);

	explicit CellExportDialog(QWidget *parent = nullptr);
	~CellExportDialog();

private:
	Ui::CellExportDialog *ui;

};

#endif // CELLEXPORTDIALOG_HPP
