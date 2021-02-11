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
#include "cellpane.hpp"
#include "ui_cellpane.h"

#include <QTreeWidget>

#include "core/simulation.hpp"
#include "core/geometry/geometry.hpp"
#include "core/geometry/cell/cell.hpp"
#include "core/utils/time_utils.hpp"
#include "core/utils/workerinterface.hpp"
#include "gui/option/guiconfig.hpp"

//// unordered_mapでQStringをキーにするためのハッシュ関数
//#include <functional>
//#include <QString>
//namespace std {
//template<> struct hash<QString> {
//    std::size_t operator()(const QString& s) const {
//        return qHash(s);
//    }
//};
//}

namespace {
const int CHK_SPACING_SIZE = 3;
const int CHK_COL = 0;  // チェックボックスの列番号
const int CELL_COL = 1; // セル名の列番号


const int CELLNAME_COLUMN = 0;
const int CHECK_COLUMN = 0;  // 末端ではないitemのチェックボックス列
//const int SPACE_COLUMN = 1;
const int CHECK_COLUMN2 = 1; // 末端itemのチェックボックス列。2行目にチェックボックスを付けたい
const int FULLNAME_COLUMN = 3; // 一意な識別名を余分の列にキャッシュしておく
}

namespace {



// セル名を再帰的に辿ってフルネームセル名をキーに、直属の親セル名のマップを作る。
void makeCellMap (const std::string name,
				  std::map<std::string, std::string, bool(*)(const std::string&, const std::string&)>* cmap)
{
	std::string::size_type pos = name.find_first_of("<");
	if(pos == std::string::npos) {
		// セル名に"<"を含まなければ親なしなので追加して終わり。
		cmap->emplace(name, "");
	} else {
		// "<"より後ろの部分が親セル名
		std::string parentCellName{name.substr(pos+1, name.size()-pos-1)};
		cmap->emplace(name, parentCellName);
		// 親セルがセルマップに未登録なら親セルも登録(そして登録時には親の親がチェックされる)
		if(cmap->find(parentCellName) == cmap->end()) {
			makeCellMap(parentCellName, cmap);
		}
	}
	return;
}

}  // end anonymous namesapce

// makeCellMapを並列実行するworker
class MakingCellMapWorker;
template <> struct WorkerTypeTraits<MakingCellMapWorker> {
	using result_type = std::map<std::string, std::string, bool(*)(const std::string&, const std::string&)>;
};
class MakingCellMapWorker: public WorkerInterface<MakingCellMapWorker>
{
public:
//	using result_type = std::map<std::string, std::string, bool(*)(const std::string&, const std::string&)>;
	typedef WorkerTypeTraits<MakingCellMapWorker>::result_type result_type;
	MakingCellMapWorker(const std::vector<std::string> *cellNames)
		: cellNames_(cellNames)
	{;}

	void operator() (std::atomic_size_t *counter, std::atomic_bool *stopFlag,
					 size_t threadNumber, size_t startIndex, size_t endIndex,
                     result_type *workerResult, std::exception_ptr *ep, bool quiet)
	{
		(void) threadNumber;
        (void) quiet;
		// result_typeでは比較関数を与えていないのでインスタンス生成時に比較関数を与えないとinsert時にバグる。
		result_type cmap(geom::Cell::cellNameLess);
		*workerResult = cmap;
		try{
			for(size_t i = startIndex; i < endIndex; ++i) {
				if(stopFlag->load()) return;
				makeCellMap(cellNames_->at(i), &cmap); // const mapの読みだしなのdata raceしないはず
				++(*counter);
			}
			workerResult->insert(std::make_move_iterator(cmap.begin()), std::make_move_iterator(cmap.end()));
		} catch (...) {
			*ep=std::current_exception();
		}
	}

	static result_type collect(std::vector<result_type> *resultVec)
	{
		result_type result(geom::Cell::cellNameLess);
		for(auto &cm: *resultVec) {
			result.insert(std::make_move_iterator(cm.begin()), std::make_move_iterator(cm.end()));
		}
        return result;
	}

	static OperationInfo info() {
		return OperationInfo(
					QObject::tr("Progress").toStdString(),
					QObject::tr("Initializing the cell child-parent connection").toStdString(),
					QObject::tr("Canceling cell tree construction").toStdString(),
					"");
	}

private:
	const std::vector<std::string> *cellNames_;
};




CellPane::CellPane(QWidget *parent, const GuiConfig *guiconf) :
	QWidget(parent),
	ui(new Ui::CellPane), guiConfig_(guiconf)
{
	ui->setupUi(this);
	connect(ui->pushButtonRestoreDefaultCheck, &QPushButton::pressed, this, &CellPane::setDefaultCheck);
	connect(ui->pushButtonCheckAll, &QPushButton::pressed, this, [this](){this->checkAll(true);});
	connect(ui->pushButtonCheckNone, &QPushButton::pressed, this, [this](){this->checkAll(false);});
    connect(ui->pushButtonExpand, &QPushButton::pressed, this, [this](){expandFileTree(true);});
    connect(ui->pushButtonCollapse, &QPushButton::pressed, this, [this](){expandFileTree(false);});
    hideExpandingWidgets(true);
    //auto header = ui->treeWidget->header();
	//header->setStretchLastSection(false);
	QStringList labels;
	auto headerItem = ui->treeWidget->headerItem();
	for(int i = 0; i < headerItem->columnCount(); ++i) {
		labels << headerItem->text(i);
	}
//	labels[SPACE_COLUMN] = "";
//	ui->treeWidget->setHeaderLabels(labels);
//	header->setSectionResizeMode(SPACE_COLUMN, QHeaderView::Fixed);
//	header->setSectionResizeMode(SPACE_COLUMN, QHeaderView::QHeaderView::ResizeToContents);


}
CellPane::~CellPane() {delete ui;}

// チェックのついたitemの名前をnamesに再帰的格納する。
void getCheckedChildNames(QTreeWidgetItem* item, std::set<std::string> *names) {
	if(item->childCount() == 0) {
		if(item->checkState(CHECK_COLUMN2) == Qt::Checked) names->emplace(item->text(FULLNAME_COLUMN).toStdString());
	} else {
		for(int i = 0; i < item->childCount(); ++i) {
			getCheckedChildNames(item->child(i), names);
		}
	}
}

std::set<std::string> CellPane::getCheckedCellNames() const
{
	std::set<std::string> cellNames;
	for(int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
		auto topItem = ui->treeWidget->topLevelItem(i);
		getCheckedChildNames(topItem, &cellNames);
	}
	return cellNames;
}





// 階層構造の一番下の名前を返す。MCNP記法ではfirst name
QString deepestName(const QString &name)
{
	int pos = name.indexOf("<");
	if(pos < 0) {
		return name;
	} else {
		return name.mid(0, pos);
	}
}

// simはセルの密度やインポータンスを調べるために必要とされている。本来セルリストがあれば十分だが。
void CellPane::updateCellPane(const std::shared_ptr<const Simulation> &sim, const std::vector<std::string> &cellNameList)
{
	utils::SimpleTimer timer;
	timer.start();
	this->clear();
	if(!sim) return;
	simulation_ = sim;

	// 子ノードフルネームをキーとして、親ノードフルネームを格納するマップ
	// 順序付関数を定義し、上の階層がが先になるようにする。
	std::map<std::string, std::string, bool(*)(const std::string&, const std::string&)> cellMap(geom::Cell::cellNameLess);
//	// シンプルなシングルスレッドでのやり方
//	for(auto &cell: cellNameList) {
//		makeCellMap(cell, &cellMap);
//	}
	// マルチスレッド版。言うほど速度は変わらないがprogress出せるメリットはある。
	auto mkCmapInfo = MakingCellMapWorker::info();
	mkCmapInfo.numTargets = cellNameList.size();
	mkCmapInfo.numThreads = guiConfig_->cuiConfig.numThread;
	cellMap = ProceedOperation<MakingCellMapWorker>(mkCmapInfo, &cellNameList);


//timer.stop();
////mDebug() << "初期化時間(ms)===" << timer.msec();
//timer.start();
	// cellMapは階層の上の方が先に来ているので上の階層からアイテムを構築する。
	// 一意なフルネーム--アイテム  生成したらここに追加
	std::unordered_map<QString, QTreeWidgetItem*> itemMap;
	QList<QTreeWidgetItem*> topItemList;
	for(auto &cellPair: cellMap) {
		QString child{QString::fromStdString(cellPair.first)};
		QString parent{QString::fromStdString(cellPair.second)};
//		mDebug() << "アイテムツリー作成開始 child===" << child << "parent=" << parent;
		QTreeWidgetItem *item;
		if(parent.isEmpty()) {
			item = new QTreeWidgetItem(ui->treeWidget);
			topItemList << item;
		} else {
			item = new QTreeWidgetItem(itemMap.at(parent));
		}

		item->setText(CELLNAME_COLUMN, deepestName(child));
		item->setText(FULLNAME_COLUMN, child);

		// ここの段階ではまだ末端アイテムかわからないからチェックは付けない
		// item->setCheckState(CHECK_COLUMN, Qt::Checked);
		itemMap.emplace(child, item);
	}
	ui->treeWidget->addTopLevelItems(topItemList);

//timer.stop();
//mDebug() << "アイテム追加時間(ms)===" << timer.msec();
//timer.start();


    // cellに階層構造があればexpand/collapseボタンを表示する
    for(int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        auto item = ui->treeWidget->topLevelItem(i);
        if(item->childCount() != 0) {
            this->hideExpandingWidgets(false);
            break;
        }
    }

//timer.stop();
//mDebug() << "展開ボタン追加(ms)===" << timer.msec();
//timer.start();

	/*
	 *  できたツリーを再帰的にたどりながら状態等を設定する。
	 * この過程ではtoplevelItemから木構造をたどるので並列不可。
	 * 一応toplevelItemごとの並列はできるが、だいたいのmcnp入力では
	 * ごく少数ののtoplevelItemに大量のセルがくっついている構造なので
	 * 並列化してもあまり意味はない。
	 * FIXME 従ってbarなしプログレスダイアログを出してうだうだするだけ。
	 *      セルの総数はわかっているのである程度はプログレスを出せる可能性はある。
	 *
	 *
	 * constructTree関数は
	 * ・itemをexpandする
	 * ・itemに子がある場合は1列目になければ2列めにチェックボックスを追加する。
	 * ・itemツリーの末端をcellMapへ登録する。
	*/
	std::function<void(QTreeWidgetItem*, std::unordered_map<std::string, QTreeWidgetItem*>*)>
	constructTree = [&constructTree](QTreeWidgetItem *item, std::unordered_map<std::string, QTreeWidgetItem*> *cmap) {
		item->setExpanded(true);
		auto numChild = item->childCount();
		if(numChild == 0) {
			item->setCheckState(CHECK_COLUMN2, Qt::Checked);
			cmap->emplace(item->text(FULLNAME_COLUMN).toStdString(), item);
		} else {
			item->setCheckState(CHECK_COLUMN, Qt::Unchecked);
			for(int i = 0; i < numChild; ++i) {
				constructTree(item->child(i), cmap);
			}
		}
	};

	for(auto &topItem: topItemList) {
		constructTree(topItem, &terminalCellMap_);
	}

//timer.stop();
//mDebug() << "ツリー作成時間(ms)===" << timer.msec();
//timer.start();



	// 末端itemに親がある場合は、親がトップレベルでなければ1レベルだけ畳む。
	for(auto &itemPair: terminalCellMap_) {
		auto parentItem = itemPair.second->parent();
		if(parentItem && topItemList.indexOf(parentItem) == -1) {
			parentItem->setExpanded(false);
		}
	}
//timer.stop();
//mDebug() << "初期折りたたみ状態時間(ms)===" << timer.msec();
//timer.start();


	// importanceが0以下、あるいはdensityが一定値以下のセルはデフォルトでは表示しない設定
	setDefaultCheck();

	// QTreeWidgetItemはemitしない。QTreeWidgetがitemChanged(*item, column)をemitする。
	// Treeのチェック状態が変化した場合に適当に判断して cellNameAdded(std::string)/cellNameRemoved(std::string)をemitする。
	connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &CellPane::handleItemChanged);
//timer.stop();
////mDebug() << "デフォルトチェック復元時間(ms)===" << timer.msec();
//timer.start();

	// ui->treeWidget->resizeColumnToContents(CELLNAME_COLUMN);
	// ui->treeWidget->resizeColumnToContents(SPACE_COLUMN);
	for(int i = 0 ; i < ui->treeWidget->columnCount(); ++i) {
		ui->treeWidget->resizeColumnToContents(i);
	}
timer.stop();
//mDebug() << "そのた最終処理時間(s)===" << timer.sec();
timer.start();
}

void CellPane::retranslate() {ui->retranslateUi(this);}

void CellPane::clear()
{
	simulation_.reset();
	ui->treeWidget->clear();
	terminalCellMap_.clear();
    this->hideExpandingWidgets(true);
}

int CellPane::minimumNoScrollWidth() const
{
	int padding =15;
	int wid = ui->pushButtonCheckAll->width() + ui->pushButtonCheckAll->width() + ui->pushButtonCheckNone->width();
	//wid = std::max(wid, ui->treeWidget->minimumWidth());
//	int wid2 = ui->treeWidget->columnWidth(0) + QFontMetrics(QFont()).width("0")*4;
	int wid2 = ui->treeWidget->columnWidth(0) + QFontMetrics(QFont()).horizontalAdvance("0")*4;
	wid = std::max(wid, wid2);
	return (wid + 2*padding)*1.05;
}

bool CellPane::isChecked(const std::string &cellName) const
{
	// TreeWidget版.treeたどるの面倒くさいからマップを用意しておいて使う
	return terminalCellMap_.at(cellName)->checkState(CHECK_COLUMN2) == Qt::Checked;
}

void CellPane::setChecked(const std::string &cellName, bool state)
{
	Qt::CheckState qstate = state ? Qt::Checked : Qt::Unchecked;
	terminalCellMap_.at(cellName)->setCheckState(CHECK_COLUMN2, qstate);
}


// 子を持つitemノードの状態を決定する。
//子ノード全てがcheckedならchecked, 全てがuncheckedならunchecked、それ以外ならpartially
void setParentState(QTreeWidgetItem *item)
{
	bool allChecked = true, allUnchecked = true;
	for(int i = 0; i < item->childCount(); ++i) {
		Qt::CheckState state;
		if(item->child(i)->childCount() == 0 ) {
			state = item->child(i)->checkState(CHECK_COLUMN2);
		} else {
			state = item->child(i)->checkState(CHECK_COLUMN);
		}
		allChecked = allChecked && state == Qt::Checked;
		allUnchecked = allUnchecked && state == Qt::Unchecked;
	}
	if(allChecked) {
		item->setCheckState(CHECK_COLUMN, Qt::Checked);
	} else if(allUnchecked) {
		item->setCheckState(CHECK_COLUMN, Qt::Unchecked);
	} else {
		item->setCheckState(CHECK_COLUMN, Qt::PartiallyChecked);
	}
	// 親ノードにも伝搬させる
	if(item->parent()) setParentState(item->parent());
}

//現在ノードの状態を子ノードに再帰的に末端まで伝搬させる
void CellPane::setChildState(Qt::CheckState state, QTreeWidgetItem *childItem)
{
    // partially状態は伝搬させない
    if(state == Qt::PartiallyChecked) return;

    // とりあえず子ノードにチェックする
    auto numChild = childItem->childCount();
    if(numChild == 0) {
        // childItemは末端ノード
        childItem->setCheckState(CHECK_COLUMN2, state);
        if(state == Qt::Checked) {
			emit cellNameAdded(childItem->text(FULLNAME_COLUMN).toStdString());
        } else if(state == Qt::Unchecked) {
			emit cellNameRemoved(childItem->text(FULLNAME_COLUMN).toStdString());
        }
    } else {
        childItem->setCheckState(CHECK_COLUMN, state);
        for(int i = 0; i < numChild; ++i) {
            setChildState(state, childItem->child(i));
        }
    }
}


void CellPane::handleItemChanged(QTreeWidgetItem *item, int column)
{
	// 一時的にdisconnectしておかないとitemChangedの伝播で大変なことになる。
	// その代わりチェックを入れたり外したりした時は手動でsignalをemitする必要が有る。
	disconnect(ui->treeWidget, &QTreeWidget::itemChanged, this, &CellPane::handleItemChanged);

	auto state = item->checkState(column);
	if(item->childCount() != 0) {
        // 子持ちノードの場合

		// 中間ノードの場合やはり親の状態をセットする。
        if(item->parent()) setParentState(item->parent());

        // 同様に子ノードに状態を伝搬させていく
        for(int i = 0; i < item->childCount(); ++i) {
            setChildState(state, item->child(i));
        }

	} else {
		// 末端ノードの場合

		// 親ノードがあれば親ノードは子ノードを調べて状態決定する
		if(item->parent()) setParentState(item->parent());
		// 末端itemはチェック状態に応じてadded/removedシグナルをemitする。
		auto text = item->text(FULLNAME_COLUMN).toStdString();
		if(item->checkState(column) == Qt::Checked) {
			emit cellNameAdded(text);
		} else if(item->checkState(column) == Qt::Unchecked) {
			emit cellNameRemoved(text);
		}
	}
	connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &CellPane::handleItemChanged);
}

void CellPane::checkAll(bool value)
{
	Qt::CheckState state = value ? Qt::Checked : Qt::Unchecked;
	for(auto &cellpair: terminalCellMap_) {
		cellpair.second->setCheckState(CHECK_COLUMN2, state);
    }
}

void CellPane::expandFileTree(bool doExpand)
{
    if(doExpand) {
        ui->treeWidget->expandAll();
    } else {
        ui->treeWidget->collapseAll();
    }
}

void CellPane::hideExpandingWidgets(bool doHide)
{
    ui->pushButtonExpand->setHidden(doHide);
    ui->pushButtonCollapse->setHidden(doHide);
}


/*
 * itemのチェック状態を設定し、それをreturnする。
 * 子ノードが全てchecked→親もchecked
 * 子ノードが全てuncheked→親もunchecked
 * 子ノードの状態がcheck/uncheck混在あるいはpartialを含む→ 親はpartiallychecked
 */
Qt::CheckState modifyCheckStateByChildren(QTreeWidgetItem *item) {
	if(item->childCount() == 0) {
		return item->checkState(CHECK_COLUMN2); // 子なし(=末端)なら状態は変更せず、チェック状態を返す。
	}
	std::vector<Qt::CheckState> childStates(item->childCount());
	for(int i = 0; i < item->childCount(); ++i) {
		childStates[i] = modifyCheckStateByChildren(item->child(i));
	}

	bool allChecked = true, allUnchecked = true;
	for(size_t i = 0; i < childStates.size(); ++i) {
		// 全チェック状態、全案チェック状態両者がfalseか、PartiallyCheckedならpartially checked適用。
		if(childStates[i] == Qt::PartiallyChecked || (!allChecked && !allUnchecked) ) {
			allChecked = false;
			allUnchecked = false;
			break;
		} else if(childStates[i] == Qt::Unchecked) {
			allChecked = false;
		} else if(childStates[i] == Qt::Checked) {
			allUnchecked = false;
		}
		//if(!allChecked && !allUnchecked) break;
		if(!(allChecked || allUnchecked)) break; //allchecked・allunchecked両者falseならpartial確定なのでbreak
	}

	Qt::CheckState	state = !(allChecked || allUnchecked) ? Qt::PartiallyChecked
														  : allChecked ? Qt::Checked : Qt::Unchecked;
	item->setCheckState(CHECK_COLUMN, state);
	return state;
}

void CellPane::setDefaultCheck()
{
	// 一時的にdisconnectしておかないとitemChangedの伝播で大変なことになる。
	disconnect(ui->treeWidget, &QTreeWidget::itemChanged, this, &CellPane::handleItemChanged);
	const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> &cellMap = simulation_->getGeometry()->cells();
	const std::string *cellName;;
	QTreeWidgetItem *item;;
	for(const auto &itemPair: terminalCellMap_) {  // terminalCellMapはアイテムツリーの末端を集めてたマップ
		// importanceが0以下、あるいはdensityが一定値以下のセルはデフォルトでは表示しない設定
		//mDebug() << "geometryViewer::cellName=" << cellName << "density=" << simulation_->getGeometry()->cells().at(cellName)->density();
		cellName = &itemPair.first;
		item = itemPair.second;
//		bool doChecked = (cellMap.at(*cellName)->importance() > 0
//						  && std::abs(cellMap.at(*cellName)->density()) >= DENSITY_CRITERIA);
		bool doChecked = (cellMap.at(*cellName)->importance() > 0 && cellMap.at(*cellName)->isHeavierThanAir());

		if(doChecked) {
			item->setCheckState(CHECK_COLUMN2, Qt::Checked);
			emit cellNameAdded(item->text(FULLNAME_COLUMN).toStdString());
		} else {
			item->setCheckState(CHECK_COLUMN2, Qt::Unchecked);
			emit cellNameRemoved(item->text(FULLNAME_COLUMN).toStdString());
		}
		// 親から子をたどれば1回で済む。ので↓はobsolete
		//if(item->parent()) setParentState(item->parent());
	}

	/*
	 * 子ノードが全てchecked→親もchecked
	 * 子ノードが全てuncheked→親もunchecked
	 * 子ノードの状態がcheck/uncheck混在→ 親はpartiallychecked
	 */
	for(int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
		modifyCheckStateByChildren(ui->treeWidget->topLevelItem(i));
	}
    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &CellPane::handleItemChanged);
	// emitNameAdded/Removedの代わりにこここでまとめてAdd removedリストを作成・送信すれば
	// 早そうだが現時点ではボトルネックではないので意味なし。
}


