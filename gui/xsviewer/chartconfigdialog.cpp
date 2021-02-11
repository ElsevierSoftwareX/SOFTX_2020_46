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
#include "chartconfigdialog.hpp"
#include "ui_chartconfigdialog.h"

#include <QColorDialog>
#include <QFontDialog>
#include <QFontMetrics>
#include <QString>
#include <QStringList>

#include "../guiutils.hpp"
#include "../../core/utils/message.hpp"
namespace {




// Qt::Penstyle順に文字列化したpenstlyeのリストを返す。
QStringList qPenStyleList()
{
	auto stringBaseStyleMap = getStringPenstyleMap();
	std::map<int, QString> styleMap;
	for(auto &p: stringBaseStyleMap) {
		// CustomDashLineはvtkに対応するenumが無いので削除する。
		if(Qt::CustomDashLine != p.second) styleMap.emplace(static_cast<int>(p.second), p.first);
	}
	QStringList qsl;
	for(auto &p: styleMap) {
		qsl << p.second;
	}
	return qsl;
}

void setLsToWidgets(const LineStyle &ls, QWidget *colWid, QComboBox *comboType, QLineEdit *widEdit)
{
	gutils::setWidgetColor(ls.qColor(), colWid);
	comboType->setCurrentIndex(static_cast<int>(ls.qPenStyle()));
	widEdit->setText(QString::number(ls.width()));
}

}  // end anonymous namespace






ChartConfigDialog::ChartConfigDialog(QWidget *parent, const ChartConfig& conf, int defaultLineNumber) :
    QDialog(parent), ui(new Ui::ChartConfigDialog), config_(conf),
    prev_xpos_(LEGEND_POS::RIGHT), prev_ypos_(LEGEND_POS::TOP)
{

	// GUI部品のChartConfig非依存部分調整
    ui->setupUi(this);
	this->setWindowTitle(tr("Chart config"));
	ui->comboBoxLineTypePlot->addItems(qPenStyleList());
	ui->comboBoxLineTypeXgrid->addItems(qPenStyleList());
	ui->comboBoxLineTypeYgrid->addItems(qPenStyleList());
//	auto wid = QFontMetrics(QApplication::font()).width("000");
	auto wid = QFontMetrics(QApplication::font()).horizontalAdvance("000");
	ui->lineEditWidthPlot->setMaximumWidth(wid);
	ui->lineEditWidthXgrid->setMaximumWidth(wid);
	ui->lineEditWidthYgrid->setMaximumWidth(wid);
    ui->comboBoxLegendXpos->addItems(ChartConfig::legendHPosList());
    ui->comboBoxLegendYpos->addItems(ChartConfig::legendVPosList());
	updateWidgets(defaultLineNumber);



    // comboBoxを初期化する過程で currentChangedがemitされ、それに応じてconfigの値まで変化してしまう。
	// よってconboBoxの項目が決定してから手動でconnectする。
	connect(ui->comboBoxLineStyleNum, SIGNAL(currentIndexChanged(QString)), this, SLOT(changePlotLineWidgets(QString)));
	connect(ui->comboBoxLineTypePlot, SIGNAL(currentIndexChanged(QString)), this, SLOT(changePlotLineType(QString)));
	connect(ui->comboBoxLineTypeXgrid, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeXgridLineType(QString)));
	connect(ui->comboBoxLineTypeYgrid, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeYgridLineType(QString)));
    connect(ui->comboBoxLegendXpos, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeLegendXpos(QString)));
    connect(ui->comboBoxLegendYpos, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeLegendYpos(QString)));
}

ChartConfigDialog::~ChartConfigDialog() {delete ui;}

int ChartConfigDialog::getEditingLineNumber() const
{
	return ui->comboBoxLineStyleNum->currentText().toInt();
}

void ChartConfigDialog::updateWidgets(int defaultLineNumber)
{
	// ここでconfig_を各ウィジェットパーツに反映させる
	ui->lineEditXtitle->setText(config_.xtitle());
	ui->lineEditYtitle->setText(config_.ytitle());
	// Font
	ui->pushButtonChooseAxisTitleFont->setText(gutils::simpleFontString(config_.axisTitleFont_.toString()));
	ui->pushButtonChooseAxisLabelFont->setText(gutils::simpleFontString(config_.axisLabelFont_.toString()));
	ui->pushButtonChooseLegendFont->setText(gutils::simpleFontString(config_.legendFont_.toString()));
	// Color
	gutils::setWidgetColor(config_.fontColor_, ui->pushButtonFontColor);
	gutils::setWidgetColor(config_.bgColor_, ui->pushButtonBGColor);
	gutils::setWidgetColor(config_.axisColor_, ui->pushButtonAxisColor);

    // legend
    if(config_.xpos_ == LEGEND_POS::CUSTOM || config_.ypos_ == LEGEND_POS::CUSTOM) {
        ui->checkBoxLegendDraggable->setChecked(true);
        ui->comboBoxLegendXpos->setDisabled(true);
        ui->comboBoxLegendYpos->setDisabled(true);
    } else {
        ui->comboBoxLegendXpos->setCurrentText(ChartConfig::fromPosToString(config_.xpos_));
        ui->comboBoxLegendYpos->setCurrentText(ChartConfig::fromPosToString(config_.ypos_));
    }
	// LineStyle
	for(size_t i = 0; i < config_.lineStyleCollection_.size(); ++i) {
		ui->comboBoxLineStyleNum->addItem(QString::number(i));
	}
	// ここでcomboBoxのサイズが変わるので、xgrid, ygridのスペーサーもサイズを変えたい
	// 三角マークのサイズも含むから大きくなる。
//	ui->comboBoxLineStyleNum->setMinimumWidth(QFontMetrics(QApplication::font()).width("000000"));
	ui->comboBoxLineStyleNum->setMinimumWidth(QFontMetrics(QApplication::font()).horizontalAdvance("000000"));


	assert(!config_.lineStyleCollection_.empty());
	if(defaultLineNumber >= static_cast<int>(config_.lineStyleCollection_.size())) defaultLineNumber = 0;
	int initIndex = defaultLineNumber;
	ui->comboBoxLineStyleNum->setCurrentIndex(initIndex);
	auto initLineStyle = config_.lineStyleCollection_.getLineStyle(initIndex);
	// 色、スタイル、幅
	setLsToWidgets(initLineStyle, ui->pushButtonColorPlot, ui->comboBoxLineTypePlot, ui->lineEditWidthPlot);
	setLsToWidgets(config_.xgridLineStyle_, ui->pushButtonColorXgrid, ui->comboBoxLineTypeXgrid, ui->lineEditWidthXgrid);
	setLsToWidgets(config_.ygridLineStyle_, ui->pushButtonColorYgrid, ui->comboBoxLineTypeYgrid, ui->lineEditWidthYgrid);


}


void ChartConfigDialog::setXtitle() {config_.xtitle_ = ui->lineEditXtitle->text();}
void ChartConfigDialog::setYtitle() {config_.ytitle_ = ui->lineEditYtitle->text();}
void ChartConfigDialog::chooseAxisTitleFont()
{
	gutils::chooseAndSetFont(this, &config_.axisTitleFont_, ui->pushButtonChooseAxisTitleFont);
}
void ChartConfigDialog::chooseAxisLabelFont() {gutils::chooseAndSetFont(this, &config_.axisLabelFont_, ui->pushButtonChooseAxisLabelFont);}
void ChartConfigDialog::chooseLegendFont() {gutils::chooseAndSetFont(this, &config_.legendFont_, ui->pushButtonChooseLegendFont);}
void ChartConfigDialog::chooseFontColor() {gutils::chooseAndSetColor(this, &config_.fontColor_, ui->pushButtonFontColor);}
void ChartConfigDialog::chooseAxisColor() {gutils::chooseAndSetColor(this, &config_.axisColor_, ui->pushButtonAxisColor);}
void ChartConfigDialog::chooseBGColor() {gutils::chooseAndSetColor(this, &config_.bgColor_, ui->pushButtonBGColor);}

void ChartConfigDialog::choosePlotColor()
{
	int lineNumber = ui->comboBoxLineStyleNum->itemText(ui->comboBoxLineStyleNum->currentIndex()).toInt();
	QColor *colorPointer = &config_.lineStyleCollection_.getLineStyleP(lineNumber)->color_;
	gutils::chooseAndSetColor(this, colorPointer, ui->pushButtonColorPlot);
}

void ChartConfigDialog::chooseXgridColor()
{
	gutils::chooseAndSetColor(this, &config_.xgridLineStyle_.color_, ui->pushButtonColorXgrid);
}

void ChartConfigDialog::chooseYgridColor()
{
	gutils::chooseAndSetColor(this, &config_.ygridLineStyle_.color_, ui->pushButtonColorYgrid);
}

void ChartConfigDialog::changePlotLineWidgets(QString lineNumberString)
{
	LineStyle ls = config_.lineStyleCollection_.getLineStyle(lineNumberString.toInt());
	setLsToWidgets(ls, ui->pushButtonColorPlot, ui->comboBoxLineTypePlot, ui->lineEditWidthPlot);
}

void ChartConfigDialog::changePlotLineType(QString lineTypeString)
{
	Qt::PenStyle ps = fromStringToPenstyle(lineTypeString);
	int lineNumber = ui->comboBoxLineStyleNum->itemText(ui->comboBoxLineStyleNum->currentIndex()).toInt();
	config_.lineStyleCollection_.getLineStyleP(lineNumber)->qstyle_ = ps;
}

void ChartConfigDialog::changeXgridLineType(QString lineTypeString)
{
	Qt::PenStyle ps = fromStringToPenstyle(lineTypeString);
	config_.xgridLineStyle_.qstyle_ = ps;
}

void ChartConfigDialog::changeYgridLineType(QString lineTypeString)
{
	Qt::PenStyle ps = fromStringToPenstyle(lineTypeString);
	config_.ygridLineStyle_.qstyle_ = ps;

}

void ChartConfigDialog::changePlotLineWidth(QString width)
{
	int lineNumber = ui->comboBoxLineStyleNum->itemText(ui->comboBoxLineStyleNum->currentIndex()).toInt();
	config_.lineStyleCollection_.getLineStyleP(lineNumber)->width_ = width.toInt();
}

void ChartConfigDialog::changeXgridWidth(QString width)
{
	config_.xgridLineStyle_.width_ = width.toInt();
}

void ChartConfigDialog::changeYgridWidth(QString width)
{
	config_.ygridLineStyle_.width_ = width.toInt();
}

void ChartConfigDialog::setDefault()
{
	config_ = ChartConfig();
	this->updateWidgets(ui->comboBoxLineStyleNum->currentText().toInt());

}

// combobox Draggableがチェックされたときのハンドラ
void ChartConfigDialog::handleDraggableChecked(bool b)
{
    if(b) {
        prev_xpos_ = config_.xpos_;
        prev_ypos_ = config_.ypos_;
        config_.xpos_ = LEGEND_POS::CUSTOM;
        config_.ypos_ = LEGEND_POS::CUSTOM;
        // uiファイルでb==trueになったらcomboBoxがdisableするようにconnectされている。
    } else {
        config_.xpos_ = prev_xpos_;
        config_.ypos_ = prev_ypos_;
        ui->comboBoxLegendXpos->setCurrentText(ChartConfig::fromPosToString(config_.xpos_));
        ui->comboBoxLegendYpos->setCurrentText(ChartConfig::fromPosToString(config_.ypos_));
    }
}

void ChartConfigDialog::changeLegendXpos(QString str)
{
    config_.xpos_ = ChartConfig::fromStringToPos(str);
}

void ChartConfigDialog::changeLegendYpos(QString str)
{
    config_.ypos_ = ChartConfig::fromStringToPos(str);
}



