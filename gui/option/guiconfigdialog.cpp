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
#include "guiconfigdialog.hpp"
#include "ui_guiconfigdialog.h"


#include <algorithm>
#include <unordered_map>

#include <QColorDialog>
#include <QFileDialog>
#include <QFontMetrics>
#include <QDebug>
#include <QString>
#include <QTableWidgetItem>

#include "../../core/utils/message.hpp"
#include "guiconfig.hpp"
#include "../guiutils.hpp"

namespace {

constexpr int MAT_COL_NUMBER = 0;
constexpr int COLOR_COL_NUMBER = 3;
constexpr int ALIAS_COL_NUMBER = 1;
constexpr int FSCALE_COL_NUMBER = 2;

// constexpr int NAME_COL_NUMBER = 2;
// constexpr int PRINTNAME_COL_NUMBER = 3;  // 使わない。
//constexpr int COLUMN_COUNT = 2;
//constexpr int INITIAL_NUM_ROWS = 3;
}




GuiConfigDialog::GuiConfigDialog(QWidget *parent, const GuiConfig &gconf,
                                 const std::map<std::string, img::MaterialColorData> &defaultColorMap) :
    QDialog(parent), ui(new Ui::GuiConfigDialog), defaultColorMap_(defaultColorMap)
{
    ui->setupUi(this);
	ui->pushButtonGetXsdir->setFixedWidth(2*QFontMetrics(gconf.uiFont).horizontalAdvance(".."));

	// 初期化・コネクト
	// ########## color
	// bg
	// material
	connect(ui->tableWidgetColor, &QTableWidget::itemPressed,
			this, &GuiConfigDialog::handleItemPressed);
	ui->tableWidgetColor->horizontalHeader()->setStretchLastSection(true); // フィットさせる


	setConfigToGui(gconf); // guiに設定を反映させる
}

GuiConfigDialog::~GuiConfigDialog() { delete ui;}

GuiConfig GuiConfigDialog::getCurrentConfig() const
{
	GuiConfig gconf;
	gconf.cuiConfig.numThread = ui->spinBoxThreads->value();
	QString modeStr = ui->comboBoxMcMode->itemText(ui->comboBoxMcMode->currentIndex());
	gconf.cuiConfig.mode = inp::stringToMcMode(modeStr.toStdString());
	gconf.cuiConfig.xsdir = ui->lineEditXsdir->text().toStdString();
	gconf.cuiConfig.noXs = !ui->checkBoxReadXs->isChecked();
	gconf.cuiConfig.verbose = ui->checkBoxVerbose->isChecked();

	gconf.uiFont = this->uiFont_;
	gconf.editorFont = this->editorFont_;

	auto qcol = gutils::getWidgetColor(ui->pushButtonBackgroundColor);
	gconf.bgColor3D = img::Color(qcol.red(), qcol.green(), qcol.blue(), 1.0);
	// ################ COLOR
	gconf.cuiConfig.colorMap.clear();
	for(int rowIndex = 0; rowIndex < ui->tableWidgetColor->rowCount(); ++rowIndex) {
		auto matItem = ui->tableWidgetColor->item(rowIndex, MAT_COL_NUMBER);
        auto colItem = ui->tableWidgetColor->item(rowIndex, COLOR_COL_NUMBER);
        auto aliasItem = ui->tableWidgetColor->item(rowIndex, ALIAS_COL_NUMBER);
        auto fscaleItem = ui->tableWidgetColor->item(rowIndex, FSCALE_COL_NUMBER);
        if(!matItem || matItem->text().isEmpty()) {
			mWarning() << "Material name is empty in color table, row index =" << rowIndex << ", Ignored.";
			continue;
        } else if (!colItem) {
            mWarning() << "Color is not set, row index =" << rowIndex << ", Ignored.";
            continue;
		} else {
            QColor qc = colItem->background().color();
			auto mName = matItem->text().toStdString();
			auto col = std::make_shared<const img::Color>(qc.red(), qc.green(), qc.blue(), qc.alpha());
            std::string aliasName = aliasItem ? aliasItem->text().toStdString() : "";
            QString scaleStr = (fscaleItem) ? fscaleItem->text() : "";
            bool ok = true;
            double fontScale = scaleStr.toDouble(&ok);
            if(!ok) fontScale = 1.0;
            gconf.cuiConfig.colorMap.emplace(mName, img::MaterialColorData(mName, aliasName, fontScale, col));
		}
	}

	// ################ advanced
	gconf.cuiConfig.timeoutBB = ui->spinBoxBBTimeout->value();
    return gconf;
}



void GuiConfigDialog::setConfigToGui(const GuiConfig &gconf)
{
	uiFont_ = gconf.uiFont;
	editorFont_ = gconf.editorFont;
	// ########################  General
	//ui->checkBoxIntegerName->setChecked(gconf.cuiConfig.useIntegerName);
	// thread
	ui->spinBoxThreads->setValue(gconf.cuiConfig.numThread);
	// Mc mode
	std::unordered_map<McMode, int> comboMap;
	int index = 0;
	for(const auto&mode: McModeList) {
		comboMap.emplace(mode, index++);
		ui->comboBoxMcMode->addItem(QString::fromStdString(inp::getModeString(mode)),
									static_cast<int>(mode));
	}
	ui->comboBoxMcMode->setCurrentIndex(comboMap.at(gconf.cuiConfig.mode));

	// xsdir, noXs, verbose
	ui->lineEditXsdir->setText(QString::fromStdString(gconf.cuiConfig.xsdir));
	ui->checkBoxReadXs->setChecked(!gconf.cuiConfig.noXs);
	ui->checkBoxVerbose->setChecked(gconf.cuiConfig.verbose);

	// font
	ui->pushButtonUiFont->setText(gutils::simpleFontString(gconf.uiFont.toString()));
	ui->pushButtonEditorFont->setText(gutils::simpleFontString(gconf.editorFont.toString()));


	// #######################  Color
	// BG
	QColor bgColor(gconf.bgColor3D.r, gconf.bgColor3D.g, gconf.bgColor3D.b);
	gutils::setWidgetColor(bgColor, ui->pushButtonBackgroundColor);
	// material

	/*
	 * cuiConfigはmatNameColのマップを持っている
	 *
	 */
    // 色テーブルは一旦すべて削除する。
    for(int i = ui->tableWidgetColor->rowCount()-1; i >= 0; --i) {
        ui->tableWidgetColor->removeRow(i);
    }
    QFontMetrics fontMs = QFontMetrics(this->uiFont_);
    // 列幅の調整。列幅は列タイトルと中身のサイズの大きい方とする。
    int matNameWidth = fontMs.horizontalAdvance(ui->tableWidgetColor->horizontalHeaderItem(MAT_COL_NUMBER)->text());
    int aliasWidth = fontMs.horizontalAdvance(ui->tableWidgetColor->horizontalHeaderItem(ALIAS_COL_NUMBER)->text());
    int fsWidth = fontMs.horizontalAdvance(ui->tableWidgetColor->horizontalHeaderItem(FSCALE_COL_NUMBER)->text());
    QString buff;
    for(const auto &colorMapPair: gconf.cuiConfig.colorMap) {
        buff = QString::fromStdString(colorMapPair.second.matName());
        matNameWidth = std::max(matNameWidth, fontMs.horizontalAdvance(buff));
        buff = QString::fromStdString(colorMapPair.second.aliasName());
        aliasWidth = std::max(aliasWidth, fontMs.horizontalAdvance(buff));
        buff = QString::number(colorMapPair.second.printSize());
        fsWidth = std::max(fsWidth, fontMs.horizontalAdvance(buff));
    }
    mDebug() << "mat,alias,fs width===" << matNameWidth << aliasWidth << fsWidth;
    constexpr int OFFSET = 10;
    constexpr int MAX_COL_WIDTH = 200;
    matNameWidth = std::min(matNameWidth, MAX_COL_WIDTH);
    aliasWidth = std::min(aliasWidth, MAX_COL_WIDTH);
    fsWidth = std::min(fsWidth, MAX_COL_WIDTH);
    ui->tableWidgetColor->setColumnWidth(MAT_COL_NUMBER, matNameWidth + OFFSET);
    ui->tableWidgetColor->setColumnWidth(ALIAS_COL_NUMBER, aliasWidth + OFFSET);
    ui->tableWidgetColor->setColumnWidth(FSCALE_COL_NUMBER, fsWidth + OFFSET);

	for(const auto &colorMapPair: gconf.cuiConfig.colorMap) {
		int rowNumber = ui->tableWidgetColor->rowCount();
		ui->tableWidgetColor->insertRow(rowNumber);

		// material名
		const std::string &matName = colorMapPair.first;
		auto itemMatname = new QTableWidgetItem(QString::fromStdString(matName));
        itemMatname->setFlags(itemMatname->flags() ^ Qt::ItemIsEditable ^Qt::ItemIsSelectable);
		ui->tableWidgetColor->setItem(rowNumber, MAT_COL_NUMBER, itemMatname);

        const img::MaterialColorData &matColData =colorMapPair.second;
		// 色
        auto color = matColData.color();
		QColor qcol(color->r, color->g, color->b, 255);
		QTableWidgetItem *itemColor = new QTableWidgetItem("");
		itemColor->setBackground(qcol);
        // selected状態になると色がわかりづらいので非selectable
		itemColor->setFlags(itemColor->flags() & ~Qt::ItemIsSelectable);
		ui->tableWidgetColor->setItem(rowNumber, COLOR_COL_NUMBER, itemColor);
        // 別名
        const std::string &aliasName = matColData.aliasName();
        auto itemAlias = new QTableWidgetItem(QString::fromStdString(aliasName));
        ui->tableWidgetColor->setItem(rowNumber, ALIAS_COL_NUMBER, itemAlias);
        // fontScale
        double fontScale = matColData.printSize();
        auto itemFontScale = new QTableWidgetItem(QString::number(fontScale));
        ui->tableWidgetColor->setItem(rowNumber, FSCALE_COL_NUMBER, itemFontScale);

	}


	// ####################### Advanced
	ui->spinBoxBBTimeout->setValue(gconf.cuiConfig.timeoutBB);


	ui->labelOpenGL->setText("");
}

void GuiConfigDialog::chooseBgColor()
{
	auto currentColor = gutils::getWidgetColor(ui->pushButtonBackgroundColor);
	gutils::chooseAndSetColor(this, &currentColor, ui->pushButtonBackgroundColor);
}


// ######################## 以下 private ルーチン

void GuiConfigDialog::chooseXsdir()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select xsdir file"), ".");
	if(!fileName.isEmpty()) {
		ui->lineEditXsdir->setText(fileName);
	}
}

void GuiConfigDialog::chooseUiFont()
{
	gutils::chooseAndSetFont(this, &uiFont_, ui->pushButtonUiFont);
}

void GuiConfigDialog::chooseEditorFont()
{
	gutils::chooseAndSetFont(this, &editorFont_, ui->pushButtonEditorFont);
}

//void GuiConfigDialog::addNewColorRow()
//{
//	// 実際にはitem追加しないとあとでselectedItems()が機能しなくなる。
//	ui->tableWidgetColor->insertRow(ui->tableWidgetColor->rowCount());
//	QTableWidgetItem *itemMat = new QTableWidgetItem("");
//	ui->tableWidgetColor->setItem(ui->tableWidgetColor->rowCount()-1, MAT_COL_NUMBER, itemMat);

//	QTableWidgetItem *itemCol = new QTableWidgetItem("");
//	//itemCol->setBackgroundColor(QColor(Qt::white));
//	itemCol->setBackground(QColor(Qt::white));
//	itemCol->setFlags(itemCol->flags() & ~Qt::ItemIsSelectable);
//	ui->tableWidgetColor->setItem(ui->tableWidgetColor->rowCount()-1, COLOR_COL_NUMBER, itemCol);
//}

//void GuiConfigDialog::removeSelectedColorRow()
//{
//	// 削除する範囲（のrowインデックス）を計算
//	int upperRowIndex = -1, lowerRowIndex = ui->tableWidgetColor->rowCount();
//	for(const auto& item: ui->tableWidgetColor->selectedItems()) {
//		upperRowIndex = (std::max)(upperRowIndex, item->row());
//		lowerRowIndex = (std::min)(lowerRowIndex, item->row());
//	}
//	// 削除する時はindexが変わるので一番小さいindex番号のところから削除する。
//	if(upperRowIndex >= lowerRowIndex && upperRowIndex >= 0) {
//		for(int i = 0; i <= upperRowIndex - lowerRowIndex; ++i) {
//			ui->tableWidgetColor->removeRow(lowerRowIndex);
//		}
//	}
//}




void GuiConfigDialog::handleItemPressed(QTableWidgetItem *item)
{
	if(item->column() == COLOR_COL_NUMBER) {
//		auto color = QColorDialog::getColor(item->backgroundColor(), this, tr("Color"));
//		if(color.isValid()) item->setBackgroundColor(color);
		auto color = QColorDialog::getColor(item->background().color(), this, tr("Color"));
		if(color.isValid()) item->setBackground(color);
	}
}

void GuiConfigDialog::setDefault()
{
    auto defaultConf = GuiConfig();
    std::map<std::string, img::MaterialColorData> tmpColorMap = defaultColorMap_;
    for(auto it = tmpColorMap.begin(); it != tmpColorMap.end();) {
        if(!img::MaterialColorData::isUserDefinedColor(it->second)) {
            it = tmpColorMap.erase(it);
        } else {
            ++it;
        }
    }
    defaultConf.cuiConfig.colorMap = tmpColorMap;
    setConfigToGui(defaultConf);
}
