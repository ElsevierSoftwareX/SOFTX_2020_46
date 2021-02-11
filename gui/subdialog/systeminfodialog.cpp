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
#include "systeminfodialog.hpp"
#include "ui_systeminfodialog.h"

#include <QIcon>
#include <QPixmap>
#include <QStyle>
#include <QSysInfo>

#include "../../core/utils/system_utils.hpp"
#include "../guiutils.hpp"

SystemInfoDialog::SystemInfoDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SystemInfoDialog)
{
	ui->setupUi(this);

	ui->labelCPU->setText(QSysInfo::currentCpuArchitecture());
	ui->labelOS->setText(QSysInfo::prettyProductName());
	ui->labelMemTotal->setText(QString::number(utils::getTotalMemMB()) + " MB");
	ui->labelMemAvail->setText(QString::number(utils::getAvailMemMB()) + " MB");

	auto glinfo = gutils::getOpenGLInfo();
	ui->labelGLVender->setText(QString::fromStdString(glinfo.vendor));
	ui->labelGLRenderer->setText(QString::fromStdString(glinfo.renderer));
	ui->labelGLVersion->setText(QString::fromStdString(glinfo.versionStr));

	auto sz = QFontMetrics(QFont()).height();
	if((glinfo.isMesa && glinfo.version >= 3.0) || (!glinfo.isMesa && glinfo.version >= 2.0)) {
		QIcon icon = style()->standardIcon(QStyle::SP_DialogYesButton);
		ui->labelRequirementIcon->setPixmap(icon.pixmap(QSize(sz, sz)));
		ui->labelRequirementResult->setText("OK");
	} else {
		QIcon icon = style()->standardIcon(QStyle::SP_DialogNoButton);
		ui->labelRequirementIcon->setPixmap(icon.pixmap(QSize(sz, sz)));
		ui->labelRequirementResult->setText("NG: Not valid OpenGL versionr");
	}
}

SystemInfoDialog::~SystemInfoDialog()
{
	delete ui;
}
