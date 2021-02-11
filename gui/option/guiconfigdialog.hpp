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
#ifndef GUICONFIGDIALOG_HPP
#define GUICONFIGDIALOG_HPP

#include <map>
#include <QDialog>
#include <QFont>

namespace Ui {
class GuiConfigDialog;
}

namespace img{
class MaterialColorData;
}
class QTableWidgetItem;
struct GuiConfig;

class GuiConfigDialog : public QDialog
{
	Q_OBJECT

public:
	// 初期値を指定してコンストラクト
    explicit GuiConfigDialog(QWidget *parent, const GuiConfig &gconf,
                             const std::map<std::string, img::MaterialColorData> &defaultColorMap);
	~GuiConfigDialog();
	// 現在セットされている設定を返す。
	GuiConfig getCurrentConfig() const;

private:
	Ui::GuiConfigDialog *ui;

	QFont uiFont_;
	QFont editorFont_;

    const std::map<std::string, img::MaterialColorData> & defaultColorMap_;
	void setConfigToGui(const GuiConfig &gconf);

private slots:
	void chooseBgColor();
	void chooseXsdir();
	void chooseUiFont();
	void chooseEditorFont();
//	void addNewColorRow();
//	void removeSelectedColorRow();
	void handleItemPressed(QTableWidgetItem *item);
	void setDefault();
};

#endif // GUICONFIGDIALOG_HPP
