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
#ifndef SYSTEMINFODIALOG_HPP
#define SYSTEMINFODIALOG_HPP

#include <QDialog>

namespace Ui {
class SystemInfoDialog;
}

class SystemInfoDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SystemInfoDialog(QWidget *parent = nullptr);
	~SystemInfoDialog();

private:
	Ui::SystemInfoDialog *ui;
};

#endif // SYSTEMINFODIALOG_HPP
