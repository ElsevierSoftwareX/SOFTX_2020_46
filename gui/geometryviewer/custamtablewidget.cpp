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
#include "custamtablewidget.hpp"

#include <cassert>
#include <string>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLayout>
#include <QList>
#include <QTableWidgetItem>

namespace {
//const int CHK_SPACING_SIZE = 3;
const int CHK_COL = 0;  // チェックボックスの列番号
const int CELL_COL = 1; // セル名の列番号
}

CustamTableWidget::CustamTableWidget(QWidget *parent)
	:QTableWidget(parent)
{

}

void CustamTableWidget::setCellNameItem(int row, QTableWidgetItem *item)
{
	std::string cellName = item->text().toStdString();
	if(cellRowMap_.find(cellName) != cellRowMap_.end()) {
		QString err = QString::fromStdString("Cell name =" + cellName + " is duplicated.");
        throw std::invalid_argument(err.toStdString());
	}
	cellRowMap_.emplace(cellName, row);
	QTableWidget::setItem(row, CELL_COL, item);
}

void CustamTableWidget::setCellCheckWidget(int rowNum, QCheckBox *box)
{
	defaultCheckData_.push_back(box->isChecked());

	auto layout = new QHBoxLayout;
	layout->addSpacerItem(new QSpacerItem(3, 0));
	layout->addWidget(box);
	QWidget * wid = new QWidget;
	wid->setLayout(layout);
	this->setCellWidget(rowNum, CHK_COL, wid);
}

QCheckBox *CustamTableWidget::getCheckBox(int row)
{
	auto widget = cellWidget(row, CHK_COL);
	return qobject_cast<QCheckBox*>(widget->layout()->itemAt(1)->widget());
}

void CustamTableWidget::clear()
{
	defaultCheckData_.clear();
	cellRowMap_.clear();
	QTableWidget::clearContents();
	QTableWidget::setRowCount(0);
}

void CustamTableWidget::setAllCheckBox(bool check)
{
	for(int i = 0; i < this->rowCount(); ++i) {
		this->getCheckBox(i)->setChecked(check);
	}
}

void CustamTableWidget::restoreDefaultCheckData()
{
	assert(defaultCheckData_.size() == this->rowCount());
	for(int i = 0; i < defaultCheckData_.size(); ++i) {
		getCheckBox(i)->setChecked(defaultCheckData_.at(i));
	}
}

void CustamTableWidget::keyPressEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_Space) {
		for(auto &item: this->selectedItems()) {
			int row = cellRowMap_.at(item->text().toStdString());
			auto checkBox = getCheckBox(row);
			checkBox->setChecked(!checkBox->isChecked());
		}
	}
	QTableWidget::keyPressEvent(e);
}
