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
#ifndef LICENSEDIALOG_HPP
#define LICENSEDIALOG_HPP

#include <QDialog>

namespace Ui {
class LicenseDialog;
}

class LicenseDialog : public QDialog
{
	Q_OBJECT

public:
	explicit LicenseDialog(QWidget *parent = 0);
	~LicenseDialog();

private:
	Ui::LicenseDialog *ui;
};

#endif // LICENSEDIALOG_HPP
