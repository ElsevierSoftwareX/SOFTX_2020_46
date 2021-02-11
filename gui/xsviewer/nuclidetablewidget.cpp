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
#include "nuclidetablewidget.hpp"
#include "ui_nuclidetablewidget.h"


#include <QTreeWidgetItem>

#include "../core/utils/string_utils.hpp"
#include "../core/utils/message.hpp"
#include "../core/material/nuclide.hpp"
#include "../component/libacexs/libsrc/mt.hpp"

const int NuclideTableWidget::COLUMN_ZAID = 0;
const int NuclideTableWidget::COLUMN_MT = 1;
const int NuclideTableWidget::COLUMN_PLOT = 2;
const int NuclideTableWidget::COLUMN_FACTOR = 3;
const int NuclideTableWidget::COLUMN_DESC = 4;



NuclideTableWidget::NuclideTableWidget(phys::ParticleType pt, QWidget *parent) :
	QWidget(parent), ui(new Ui::NuclideTableWidget), ptype_(pt)
{
	qRegisterMetaType<phys::ParticleType>("phys::ParticleType ptype");
	ui->setupUi(this);
	connect(ui->treeWidgetNuclides, &QTreeWidget::itemChanged, this, &NuclideTableWidget::relayItemChanged);

	connect(ui->treeWidgetNuclides, &QTreeWidget::itemDoubleClicked, this, &NuclideTableWidget::handleDoubleClick);
}

NuclideTableWidget::~NuclideTableWidget()
{
	delete ui;
}


// FIXME ace:モノリシックな中性子断面積ファイルの読み取りにすごく時間がかかっている。
void NuclideTableWidget::setNuclides(const std::unordered_map<std::string, std::shared_ptr<const mat::Nuclide>> &nucMap)
{
	disconnect(ui->treeWidgetNuclides, &QTreeWidget::itemChanged, this, &NuclideTableWidget::relayItemChanged);

	std::vector<std::shared_ptr<const mat::Nuclide>> nucVec;
	nucVec.reserve((nucMap.size()));
	for(auto &zaidNucPair: nucMap) {
		nucVec.emplace_back(zaidNucPair.second);
	}

	std::sort(nucVec.begin(), nucVec.end(), mat::Nuclide::NuclidePLess);

	QList<QTreeWidgetItem*> topItemList;
	for(auto nuc: nucVec) {
		//mDebug() << "Adding ZAID====" << nuc->zaid();
		QTreeWidgetItem* topItem = new QTreeWidgetItem(ui->treeWidgetNuclides);
		QString zaid = QString::fromStdString(nuc->zaid());
		topItem->setText(COLUMN_ZAID, zaid);
		// NuclideのMT番号を取得
		// xsMapは ace::Reaction, CrossSectionのマップ
		ace::AceFile::XSmap_type xsMap = nuc->xsMap();
		// xsMapはunordereだけど表示時にはMT順に並べたい
		// ace::Reactionは実際はMT番号に等しい整数enumなのでstd::mapに入れればうまく並んでくれる。
		std::map<ace::Reaction, const ace::CrossSection*> tmpXsMap;
		for(auto &xsPair: xsMap) {
			tmpXsMap.emplace(xsPair.first, &xsPair.second);
		}
		for(auto &xsPair: tmpXsMap) {
			QString mtStr = QString::fromStdString(ace::mt::toMtString(xsPair.first));
			QTreeWidgetItem* childItem = new QTreeWidgetItem(topItem);
			childItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
			//childItem->setText(COLUMN_ZAID, zaid);
			childItem->setText(COLUMN_MT, mtStr);
			childItem->setText(COLUMN_FACTOR, "1");
			childItem->setText(COLUMN_DESC, QString::fromStdString(ace::mt::description(ace::mt::toNumber(xsPair.first))));
			childItem->setCheckState(COLUMN_PLOT, Qt::Unchecked);
		}

		topItemList << topItem;
	}

	ui->treeWidgetNuclides->addTopLevelItems(topItemList);
	for(int i = 0; i < ui->treeWidgetNuclides->columnCount(); ++i) ui->treeWidgetNuclides->resizeColumnToContents(i);

	// セットが終わったらitemChangedを中継するように再度connect
	connect(ui->treeWidgetNuclides, &QTreeWidget::itemChanged, this, &NuclideTableWidget::relayItemChanged);

}

void NuclideTableWidget::clear()
{
	ui->treeWidgetNuclides->clear();
}

void NuclideTableWidget::clearTableCheckBox()
{
	// テーブルのチェックボックス全てをオフにする
	assert(ui->treeWidgetNuclides->topLevelItemCount() != 0);
	for(int i = 0; i < ui->treeWidgetNuclides->topLevelItemCount(); ++i) {
		QTreeWidgetItem *topItem = ui->treeWidgetNuclides->topLevelItem(i);
		for(int index = 0; index < topItem->childCount(); ++index) {
			auto childItem = topItem->child(index);
			childItem->setCheckState(COLUMN_PLOT, Qt::Unchecked);
		}
	}
}

void NuclideTableWidget::retranslate()
{
	ui->retranslateUi(this);
}

int NuclideTableWidget::minRequiredWidth() const
{
//	int wid = QFontMetrics(QFont()).width("0")*20; // offsetを20文字ぶん与える
	int wid = QFontMetrics(QFont()).horizontalAdvance("0")*20; // offsetを20文字ぶん与える
	assert(COLUMN_PLOT < ui->treeWidgetNuclides->columnCount());
	for(int i = 0; i <= COLUMN_FACTOR; ++i) {
        //mDebug() << "i=" << i << "col width ===" << ui->treeWidgetNuclides->columnWidth(i);
		wid += 	ui->treeWidgetNuclides->columnWidth(i);
	}

	return wid;
}

std::vector<QTreeWidgetItem *> NuclideTableWidget::checkedItems() const
{
	std::vector<QTreeWidgetItem *> retvec;
	for(int i = 0; i < ui->treeWidgetNuclides->topLevelItemCount(); ++i) {
		auto toplevelItem = ui->treeWidgetNuclides->topLevelItem(i);
		for(int j = 0; j < toplevelItem->childCount(); ++j) {
			auto item = toplevelItem->child(j);
			if(item->checkState(COLUMN_PLOT) == Qt::Checked) retvec.emplace_back(std::move(item));
		}
	}
	return retvec;
}


void NuclideTableWidget::handleDoubleClick(QTreeWidgetItem *item, int column)
{
	// FACTOR列がダブルクリックされた場合はedit可能にする。
	if(column == COLUMN_FACTOR){
		item->setFlags(Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
	} else {
		item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		}
	}

	void NuclideTableWidget::relayItemChanged(QTreeWidgetItem *item, int column)
	{
		emit NuclideTableWidget::itemChanged(ptype_, item, column);
	}
