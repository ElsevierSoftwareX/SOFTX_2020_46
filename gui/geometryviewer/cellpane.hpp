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
#ifndef CELLPANE_HPP
#define CELLPANE_HPP

#include <list>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <QTreeWidgetItem>
#include <QWidget>
#include "../../core/utils/progress_utils.hpp"

class Simulation;
struct GuiConfig;

namespace Ui {
class CellPane;
}

class CellPane : public QWidget
{
	Q_OBJECT

public:
	explicit CellPane(QWidget *parent, const GuiConfig *guiconf);
	~CellPane();
	// チェックのついているセル名のsetを返す
	std::set<std::string> getCheckedCellNames() const;
	// テーブルのアップデート
	void updateCellPane(const std::shared_ptr<const Simulation> &sim, const std::vector<std::string> &cellNameList);
	void retranslate();
	void clear();
	// セルペインの（スクロールせずに表示できる）最低幅
	int minimumNoScrollWidth() const;

//	// rowNumの行のセルのチェックがついた/外れたsignalを受けてadding/removingCells_に登録する。
//	void registerCheckChangedCell(bool checked, int rowNum);

	// cellNameを与えて、それが現在checked状態かを返す
	bool isChecked(const std::string &cellName) const;
	// cellNameを与えて、チェック状態を第二引数のようにセットする。
	void setChecked(const std::string &cellName, bool state);

signals:
	// CellPaneは可視化セルの増減を↓のメソッドで伝達している
	// GeometryViewerの変更セルなどの設定へconnectされている。
	void cellNameAdded(std::string);
	void cellNameRemoved(std::string);

private:
	Ui::CellPane *ui;
	std::shared_ptr<const Simulation> simulation_;
	// treeを毎回たどるのはしんどいので末端アイテムはmapで管理する。
	// キー：末端アイテムのフルネーム,  値:itemへのポインタ
	std::unordered_map<std::string, QTreeWidgetItem*> terminalCellMap_;
	const GuiConfig *guiConfig_;

private slots:
	void handleItemChanged(QTreeWidgetItem *item, int column);
	void checkAll(bool value);
    void expandFileTree(bool doExpand);
    void hideExpandingWidgets(bool doHide);
	void setDefaultCheck();
    void setChildState(Qt::CheckState state, QTreeWidgetItem *childItem);
};



#endif // CELLPANE_HPP
