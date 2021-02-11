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
#include "inputviewer.hpp"
#include "ui_inputviewer.h"

#include <regex>
#include <string>

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>

#include "gui/inputviewer/filetabitem.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/system_utils.hpp"
#include "gui/option/guiconfig.hpp"
#include "gui/subdialog/messagebox.hpp"

/*
 * 今は
 * 1．ファイル読み込み
 * 2．読み込みが終了したらInputViewがファイルリストをもらいupdateInputFileDataを実行。
 * 3．updateInputFileDataではInputViewは(タブ復元情報を保存してから)一旦クリアする
 * 4．InputViewでのファイルツリー構築と（ルートファイルが同じなら）タブ復元
 * という流れになっているので、1．のファイル読み込み中にエラー情報をInputViewに渡して表示させても
 * 3．のところで一旦クリアされてしまうので意味がない。
 * この方式の利点は再読込や別ファイル読み込みが1．へ戻るだけで実行できること。
 *
 * よって、タブ復元情報にエラー情報も加えておけば良い。エラー情報は1.の前にクリアする
 *
 * 問題：ファイル読み込み時にエラーが発生した場合、ファイルリストは読めた所までになる。
 *      さらに、Fatalログはエラー発生時即座にInputViewへ送られてしまい、これはファイルリスト受取に先行してしまう。
 *
 *
 * 代替案。エラー情報届く前にファイル名を取得しておきたい。
 * 0．ルートファイルはとりあえずInputViewで表示する
 * 1．InputDataでファイルを読み取る場合、ファイルを開くのに成功次第、オープン成功ファイル名と親ファイル名をemitする。
 * 2．InputViewで受け取ってファイルツリーを構築する。
 * とする。
 *
 * (親なしの)ルートファイルが届いた時（=新規読み込みor再読込）
 * ・クリア(レストア情報保存、全タブ閉じ、ツリークリア)。ここでエラー情報はクリアされる
 * ・ルートファイル名が変わっていればレストア情報の破棄
 * ・ファイルツリーのクリア
 * ・新規ツリーのルート作成とセット
 * ・ルートファイルはとりあえず開く
 *
 * (親ありの)非ルートファイルが届いた時
 * ・ファイルツリーへchildとして追加
 * ・レストア情報に記録されていたらファイルタブ復元
 *
 *
 */

namespace{
// openedFileTreeでは表示しないアイテムの３列目にrelativeファイル名を格納する
// 1列目も同様のファイル名だが、ファイル変更によって＊をつけてファイル名に等しくない場合がある。
const int CELLNAME_COLUMN = 0;
const int CLOSE_BUTTON_COLUMN = 1;
const int TRUENAME_COLUMN = 2;
}

size_t InputViewer::counter = 0;

InputViewer::InputViewer(QWidget *parent, const QString &tabText, const GuiConfig *gconf) :
	TabItem(parent, tabText, gconf), ui(new Ui::InputViewer)
{
	ui->setupUi(this);
	resetStyleSheet();

	// 開いているファイル一覧Treeの設定
	// opened file表示部分のヘッダ部分調整
	QHeaderView *header = ui->treeWidgetOpenedFiles->header();
	header->setStretchLastSection(false);
	header->setSectionResizeMode(CELLNAME_COLUMN, QHeaderView::QHeaderView::Stretch);
	header->setSectionResizeMode(CLOSE_BUTTON_COLUMN, QHeaderView::QHeaderView::ResizeToContents);
	connect(ui->treeWidgetOpenedFiles, &QTreeWidget::itemClicked,
			this, &InputViewer::handleOpendFileItemClicked);


	// テキスト表示タブの設定
	// currentTabが変わったらコンボボックスファイル名も変える
    ui->tabWidget->tabBar()->hide();
	connect(ui->tabWidget, &QTabWidget::tabCloseRequested,	this, &InputViewer::closeTabItem);
	connect(ui->comboBoxSlectFile, &QComboBox::currentTextChanged,
			this, &InputViewer::setCurrentTabByRelativePath);
	// 現在開いているタブが変わったらactivateして強調表示する
	connect(ui->tabWidget, &QTabWidget::currentChanged,
			this, [=](){
		// まず現在のselect状態を解除する
		for(int i = 0; i < ui->treeWidgetOpenedFiles->topLevelItemCount(); ++i) {
			ui->treeWidgetOpenedFiles->topLevelItem(i)->setSelected(false);
		}

		auto fileItem = getCurrentItem();
		if(fileItem == nullptr) return;

		QString relativeFilePath = toRelativeFilePath(fileItem->canonicalFilePath());
		ui->comboBoxSlectFile->setCurrentText(relativeFilePath);
		QTreeWidgetItem *openedItem = nullptr;
		for(int i = 0; i < ui->treeWidgetOpenedFiles->topLevelItemCount(); ++i) {
            QString itemName = ui->treeWidgetOpenedFiles->topLevelItem(i)->text(TRUENAME_COLUMN);
            // 変更が加わっている場合opened file treeでは末尾に*がついている可能性がある。
            if(relativeFilePath == itemName) {
				openedItem = ui->treeWidgetOpenedFiles->topLevelItem(i);
			}
		}
		if(openedItem == nullptr) {
			// openTabFile()での実装がまずいと最初にファイルタブが開かれた時はopendFilesTreeのアイテムはセットされていない
			mFatal("ProgramError: File =", relativeFilePath, "is not opened");
		}
		openedItem->setSelected(true);
	});


	// ファイルツリー表示の設定
	connect(ui->treeWidgetFiles, &QTreeWidget::itemDoubleClicked,
			this, [=](QTreeWidgetItem *item, int column) {
				Q_UNUSED(column);
				openFileTab(toAbsoluteFilePath(item->text(0)));
			});
	// TODO ファイルツリー右クリックの挙動を追加するならここ。
	// 実はWindowsでもパスセパレータは"/"で良い
}

InputViewer::~InputViewer() {delete ui;}

void InputViewer::init()
{
	// splitterのサイズ調整
	if(hasInitialized_) return;
	int w = this->size().width();
	const double ratio = 0.2;
//	int infoWidth = std::max(static_cast<int>(std::round(w*ratio)), 20*QFontMetrics(QFont()).width('0'));
	int infoWidth = std::max(static_cast<int>(std::round(w*ratio)), 20*QFontMetrics(QFont()).horizontalAdvance('0'));
	ui->splitterH->setSizes(QList<int>{infoWidth, w - infoWidth});
	ui->treeWidgetFiles->setFocus();  // InputViewerを表にした状態でCtrl-Fを受け取れるようにfocusを適当なchildにうつしておく
	TabItem::init();
}

void InputViewer::retranslate() {ui->retranslateUi(this);}

void InputViewer::clearStyleSheet() {ui->tabWidget->setStyleSheet("");}

bool InputViewer::confirmUnsavedFiles()
{
    // ファイルの保存について尋ねる。	再読込前に未保存ファイルがあれば警告すること
    // 0. 未保存ファイルのリストを作る
    // 1. 未保存ファイルタブをcurrentTabにする
    // 2. 保存するか尋ねる
    // 3. 保存するならsaveCurrentTab

	std::vector<QString> fileNameList = getOpenedRelativeFileList();
	auto rmResult = std::remove_if(fileNameList.begin(), fileNameList.end(), [](const QString &str){return !str.endsWith("*");});
	fileNameList.erase(rmResult, fileNameList.end());


    for(auto &fileName: fileNameList) {
        setCurrentTabByRelativePath(fileName);

        QMessageBox msgBox;
        QString text = "\"" + fileName.mid(0, fileName.size()-1) + "\"" +  tr(" has been modified");
        msgBox.setText(text);
        msgBox.setInformativeText(tr("Do you want to save your changes?"));
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        int ret = msgBox.exec();

        switch (ret) {
        case QMessageBox::Save:
            // Save was clicked
            this->saveCurrentFile();
            break;
        case QMessageBox::Discard:
            // なにもしない
            break;
        case QMessageBox::Cancel:
            return false;
        default:
            assert(false);
            break;
        }
    }
	return true;
}





void InputViewer::receiveOpenedFileName(std::pair<std::string, std::string> files)
{
	//mDebug() << "Opened files received, parent===" << files.first << ", child=" << files.second;
	// ファイルオープンに成功したファイル名が送られてくるのでこれを元にファイルツリーをアップデートする
	assert(!files.second.empty());


	if(files.first.empty()) {
		this->clear(); // ルートファイルが届いたらまずクリア。この時レストア情報が保存される。

		// まず絶対/相対名を確定させる。
		QFileInfo rootInfo(QString::fromStdString(files.second));
		rootDir_ = QDir(rootInfo.absolutePath());
		absRootFilePath_ = rootInfo.absoluteFilePath();

		// ルートファイルが変わっていればレストア用開いていたタブリスト削除
		if(absRootFilePath_ != previousAbsRootPath_) {
			prevActiveTabAbsFileName_.clear();
			previouslyOpendFileNames_.clear();
		}

		// ファイルツリーをルートから作り直す
		QTreeWidgetItem *rootItem = new QTreeWidgetItem(ui->treeWidgetFiles);
		rootItem->setText(0, toRelativeFilePath(absRootFilePath_));
		rootItem->setExpanded(true);
		ui->treeWidgetFiles->insertTopLevelItems(0, QList<QTreeWidgetItem*>{rootItem});

		// とりあえずルートファイルは開く
		this->openFileTab(absRootFilePath_);

	} else {

		// それ以外の通常ファイルの場合rootから辿って親の下につける
		assert(ui->treeWidgetFiles->topLevelItemCount() != 0);
		QTreeWidgetItem *topItem =ui->treeWidgetFiles->topLevelItem(0);

		// itemにセットするファイル名は表示したい名前なのでrelativeを用いる
		QString childAbs = QFileInfo(QString::fromStdString(files.second)).absoluteFilePath();
		QString parentRelative = toRelativeFilePath(QFileInfo(QString::fromStdString(files.first)).absoluteFilePath());
		QString childRelative = toRelativeFilePath(childAbs);

		// itemが親ファイルを指していればchildを追加、そうでなければ更にたどる
		std::function<bool(const QString&, const QString&, QTreeWidgetItem*)> addChild
			= [&](const QString &parentFile, const QString &childFile, QTreeWidgetItem *item)
		{
			//mDebug() << "top=" << item->text(0) << ", parent=" << parentFile;
			if(item->text(0) == parentFile) {
				QTreeWidgetItem *childItem = new QTreeWidgetItem(item);
				childItem->setText(0, childFile);
				childItem->setExpanded(true);
				return true;
			} else {
				for(int i = 0; i < item->childCount(); ++i) {
					if(addChild(parentFile, childFile, item->child(i))) return true;
				}
				return false;
			}
		};

		if(!addChild(parentRelative, childRelative, topItem)) {
			mFatal("Adding child file= ", childRelative, " failed. No parent file found.");
		}

		/*
		 * previouslyOpenedFileNamesを絶対パスにしていた場合includeカードの入力で..などがあると
		 * ファイルの同一性が認識できない。
		 *
		 */
		// 開いていたファイルリスト内に該当していれば再度開く
		if(previouslyOpendFileNames_.find(childAbs) != previouslyOpendFileNames_.end()) {
			this->openFileTab(childAbs);
			// 新しく開いたら前回のアクティブタブがcurrentに来るように調整する。
			auto fileList = getOpenedRelativeFileList();
			auto findResult = std::find(fileList.begin(), fileList.end(), toRelativeFilePath(prevActiveTabAbsFileName_));
			if(findResult != fileList.end()) {
				ui->tabWidget->setCurrentWidget(getTabItembyAbsFilePath(prevActiveTabAbsFileName_));
			}
		}
	}
}


namespace {
// アイテムツリーを再帰的に辿り、 第一引数の名前を持つアイテムを第三引数に追加する。
void findItem(const QString &str, QTreeWidgetItem* item, std::vector<QTreeWidgetItem*>* vec)
{
    if(item->text(0) == str) {
        vec->emplace_back(item);
        return;
    } else {
        for(int i = 0; i < item->childCount(); ++i) {
            findItem(str, item->child(i), vec);
        }
    }
}
}

// TODO 欲を言えば重複セルみたいな複数ヶ所エラーがわかっている場合は複数マークを付けたい
// FIXME 発生箇所情報と、エラー内容の情報を分離して渡したい。頻繁にバグを発生させている。
// fatalメッセージを受け取ってファイル名と行番号を取得し、適当なタブへ投げる
void InputViewer::receiveFatalLog(QSharedPointer<std::string> strP)
{
    std::string message = (*(strP.data()));
    // Fatal: C:/aa/b b\c/d:6 みたい可能性がある。
    std::smatch sm;
//    static const std::regex fatallRegex(R"(Fatal: *(.*?):([0-9]*))");  // :数字　までがファイル名とする
	static const std::regex fatallRegex("^Fatal: *(.*?):(\\d+)");  // :数字　までがファイル名とする
    if(std::regex_search(message, sm, fatallRegex)) {
        for(size_t i = 0; i < sm.size(); ++i) {
            mDebug() << "i===" << i << sm.str(i);
        }
//		abort();
        QString absFilePath;
        if(utils::isRelativePath(sm.str(1))) {
			/*
			 * sm.str(1)はエラーの発生したファイル名であるが
			 *  1.絶対パス指定
			 *  2.カレントディレクトリからの相対パス
			 * でのどちらかで送られてくる。ところがこのクラスのstaticメンバ
			 * toAbsoluteFilePathではトップ入力ファイルのフォルダからの相対パスとして扱うため
			 * ここでは使用できない。
			 */
			//absFilePath = toAbsoluteFilePath(QString::fromStdString(sm.str(1)));
			absFilePath = QFileInfo(QString::fromStdString(sm.str(1))).absoluteFilePath();
		} else {
            absFilePath = QString::fromStdString(sm.str(1));
        }

        //ここまででエラーが発生しているファイル(absFilePath)はわかった。

        mDebug() << "Error! message===" << message << " file ===" << absFilePath;

        // plainTextEditへの表示
        this->openFileTab(absFilePath);  // とりあえず開く。すでに開いていればcurrentTabにくる。
        FileTabItem *tabItem = getCurrentItem();
        if(tabItem != nullptr) {
            int line = utils::stringTo<int>(sm.str(2));
            // ファイル名と行名がわかったのでfile表示
            tabItem->setFatalInLineEdit(line);
        } else {
            mWarning() << "An error is happend in file =" << absFilePath << ", but not found.";
        }

        // Treeviewへもcriticalアイコンを追加する
        auto relativeFilePath = toRelativeFilePath(absFilePath);
        using ItemVec = std::vector<QTreeWidgetItem*>;

        ItemVec errItems;
		/*
		 * ファイル読み取り中にエラーが発生すると、(ロガーの中継等をはさんで)
		 * ここへ来るが、simulation生成後にファイル一覧を取得する方式の場合、
		 * ここでエラー発生箇所を指摘しようとするとまだ、こちらのファイルツリーには
		 * 追加されておらず、見つからないためエラーになる。
		 * →やはりInputDataにはQObjectになってもらってsignalを中継するしか無い。
		 */
        for(int i = 0; i < ui->treeWidgetFiles->topLevelItemCount(); ++i) {
            findItem(relativeFilePath, ui->treeWidgetFiles->topLevelItem(i), &errItems);
        }
		// TODO エラー発生箇所が見つからなかった時のハンドリングはassert中止で良いか？
        //assert(errItems.size() == 1);
        if(errItems.size() >= 1) {
            auto item = errItems.front();
            auto icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical);
            item->setIcon(0, icon);
        }

	}
}

void InputViewer::handleSaveAction()
{
	saveCurrentFile();
}

void InputViewer::handleReloadAction()
{
	reloadCurrentFile();
}

void InputViewer::handleCopyAction()
{
	getCurrentItem()->copy();
}

void InputViewer::handleCutAction()
{
	getCurrentItem()->cut();
}

void InputViewer::handlePasteAction()
{
	getCurrentItem()->paste();
}

void InputViewer::handleFindAction()
{
	getCurrentItem()->findAct();
}

void InputViewer::handleUndoAction()
{
	getCurrentItem()->undo();
}

void InputViewer::handleRedoAction()
{
    getCurrentItem()->redo();
}

void InputViewer::handleCommentRegionAction()
{
    getCurrentItem()->commentRegion();
}


void InputViewer::handleGuiConfigChanged()
{
	// 適用するのはエディタのフォント
	// ui->tabWidget->widget(i)->setFont()だとTabItemの親クラスであるQWidget::setFont()しか呼ばれない。
	for(auto& tabItemPair: tabNameMap_) {
		if(guiConfig_ != nullptr) tabItemPair.first->setFont(guiConfig_->editorFont);
	}
}


void InputViewer::keyPressEvent(QKeyEvent *ev)
{
	// Ctrl-wで現在のタブを閉じる
	if(ev->modifiers() == Qt::ControlModifier) {
		if(ev->key() == Qt::Key_W) {
			closeTabItem(ui->tabWidget->currentIndex());
		} else if(ev->key() == Qt::Key_F) {
			//Ctrl-Fはファイルタブアイテムで処理する。
			//mDebug() << "InputView::Ctrl-F!!!!!";
			if(getCurrentItem()) {
				getCurrentItem()->handleCtrlFkey();
			}
		}
	}
	TabItem::keyPressEvent(ev);
}

void InputViewer::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::StyleChange) resetStyleSheet();
}

void InputViewer::clear()
{
    // 現在開いているファイル名を記録する。ルートファイルが変わったら場合は記憶しない。
	previouslyOpendFileNames_.clear();
	for(auto it = tabNameMap_.begin(); it != tabNameMap_.end(); ++it) {
		//previouslyOpendFileNames_.emplace(std::get<2>(it->second), std::get<0>(it->second));
		previouslyOpendFileNames_.emplace(std::get<0>(it->second));
	}

	auto itemPtr =getCurrentItem();
	if(itemPtr)	prevActiveTabAbsFileName_ = itemPtr->canonicalFilePath();

	previousAbsRootPath_ = absRootFilePath_;

	// QTabWidget::clear() でもcurrentChangedが呼ばれてしまって
	// 想定していた今まで開いていたタブのインデックスが保存できないので一時保存してクリア後に戻す
	ui->tabWidget->clear();
	ui->treeWidgetFiles->clear();
	ui->treeWidgetOpenedFiles->clear();
	ui->comboBoxSlectFile->clear();
	tabNameMap_.clear();
	rootDir_.cleanPath("");
}

// index番目のタブを閉じる。
void InputViewer::closeTabItem(int index)
{
    /*
     * Tabを閉じる手続き
     *
     * QTabWidget::removeTab(index)はタブ上widgetを除去するがdeleteはしない
     * しかしタブ上Widgetのインスタンスをdeleteするとその時点でremoveが起こる。
     * この現象はドキュメント化されていないので利用したくない。
     * 故に、removeTab→delteの順でおこなう。当然removeTabしたあとはindexはずれるので使用不可
     *
     */
    if(index < 0 || index > ui->tabWidget->count()) {
        mDebug() << "invalid index ===" << index;
        assert(index >= 0 && index < ui->tabWidget->count());
        return;
    }
	for(int i = 0; i < ui->treeWidgetOpenedFiles->topLevelItemCount(); ++i) {
		ui->treeWidgetOpenedFiles->topLevelItem(i)->setSelected(false);
	}

    auto widget = ui->tabWidget->widget(index);
    // 注意！removeTabしてしまうとindexはずれるし、eraseするとitは無効
    auto it = tabNameMap_.find(static_cast<FileTabItem*>(widget));
	QString canonicalFilePath;
	if(it == tabNameMap_.end()) {
		mFatal() << "The closing tab is not found. index =====" << index;
	}
	canonicalFilePath = std::get<0>(it->second); // ここでtabNameMap_のeraseを先に実行してはならない
	// ※tabはtabWidgetから削除すれば不可視になるが、まだ存在はしている。
	// parent=thisでnewされているから、InputViewerがデストラクトされるまでは存続する。
	// →でもこれopen・close繰り返したらメモリ消費量増える一方なのでdeleteする。

    ui->tabWidget->removeTab(index);  // indexは以後使わないのでここでタブを削除
    delete widget;  // tabWidgetに乗っているwidgetの削除
    // tabMap_のエントリを削除。STLコンテナは所有権なんて取らないので削除に問題はない。がiterator無効になる
    tabNameMap_.erase(it);

	// closeしたらopendFileTreeから削除
	QString relativeFilePath = toRelativeFilePath(canonicalFilePath);
    for(int i = 0; i < ui->treeWidgetOpenedFiles->topLevelItemCount(); ++i) {
        if(ui->treeWidgetOpenedFiles->topLevelItem(i)->text(TRUENAME_COLUMN) == relativeFilePath) {
            ui->treeWidgetOpenedFiles->takeTopLevelItem(i);
            break;
        }
    }

    // コンボボックスからも削除
	ui->comboBoxSlectFile->removeItem(ui->comboBoxSlectFile->findText(relativeFilePath));
}


/* ファイルを開く
 * 1. tabNameMapへ登録
 * 2. openedFilesTreeへ登録
 * 3. opendFilesComboboxへ登録
 * 4．tabWidgetで開く。
 *
 * tabWidgetとopenedFilesTreeWidgetで別々にデータを持ってるから面倒。
 * データを一箇所にしてTreeViewを導入したい。
 */
void InputViewer::openFileTab(const QString &fileName)
{
	// ..を消して絶対ファイルパスは一意になるようにする。と今度はopnedFileTreeとの連携がおかしくなる。
	QString absFilePath = QFileInfo(fileName).absoluteFilePath();
//	QString absFilePath = fileName;
	if(utils::isRelativePath(fileName.toStdString())) {
		absFilePath = toAbsoluteFilePath(absFilePath);
	}

	// 入力ファイル読み込み中にエラーが発生した場合、エラー箇所の表示のために既に開いているファイル
	// を引数として再度openFileTabが呼ばれることがある。その時はそのファイルを全面に出すだけ。
	if(!QFileInfo(absFilePath).exists()) {
        GMessageBox::critical(0, "critical", QString("File ") + toRelativeFilePath(absFilePath) + " not found..", true);
        return;
    }

	QString relativeFilePath = toRelativeFilePath(absFilePath);
	// 既に開かれているならばファイルは開かず、現在タブを変更するだけ、
	for(auto it = tabNameMap_.begin(); it != tabNameMap_.end(); ++it) {
		if(absFilePath == std::get<0>(it->second)) {
			ui->tabWidget->setCurrentWidget(static_cast<QWidget*>(it->first));
			return;
		}
	}

	// 1. tabNameMapへの追加
	auto fileTabItem = new FileTabItem(this, absFilePath, guiConfig_->mcMode());
	tabNameMap_.emplace(fileTabItem, std::make_tuple(absFilePath, false, ++counter));

	// 2．Opend file TreeWidgetへ追加
	/*
	 * QTreeWidgetItemにparentを設定するとinsertTopLevelItemの順序が機能しなくなる。よってparentを与えない。
	 * この時のメモリの管理はどのように行われるか？Qtのリファレンスマニュアルによると
	 * QTreeWidgetのデストラクタ：
	 *		Destroys the tree widget and all its items.
	 * QTreeWidgetItemのデストラクタ:
	 *		Destroys this tree widget item.The item will be removed from QTreeWidgets to which it has been added.
	 *      This makes it safe to delete an item at any time.
	 *
	 * とあるので手動で管理せずともうまくやってくれることになっている。
	 */
	QTreeWidgetItem *openedFileItem = new QTreeWidgetItem;
    openedFileItem->setText(CELLNAME_COLUMN, relativeFilePath);
    openedFileItem->setIcon(CLOSE_BUTTON_COLUMN, QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    openedFileItem->setText(TRUENAME_COLUMN, relativeFilePath);
    ui->treeWidgetOpenedFiles->addTopLevelItem(openedFileItem);
    ui->treeWidgetOpenedFiles->sortItems(TRUENAME_COLUMN, Qt::AscendingOrder);


	// 3. ファイル選択コンボボックスへ追加。順序は辞書順で考慮する
	ui->comboBoxSlectFile->addItem(relativeFilePath);
	ui->comboBoxSlectFile->model()->sort(0);


	// 4．ファイルタブの追加。 タブ追加は最後にする。なぜならaddTabでtabChangedがemitされてしまうから
	ui->tabWidget->addTab(fileTabItem, relativeFilePath);
	if(guiConfig_ != nullptr) fileTabItem->setFont(guiConfig_->editorFont);


	// 5. connect
    connect(fileTabItem, &FileTabItem::fileTabModificationChanged, this,
        [=](bool flag){
        bool *currentModifiedFlag = &std::get<1>(tabNameMap_.at(fileTabItem));
        if(flag == *currentModifiedFlag) return;
        if(!(*currentModifiedFlag)) {
            openedFileItem->setText(CELLNAME_COLUMN, openedFileItem->text(TRUENAME_COLUMN) + "*");
            *currentModifiedFlag = true;
        } else {
            openedFileItem->setText(CELLNAME_COLUMN, openedFileItem->text(TRUENAME_COLUMN));
            *currentModifiedFlag = false;
        }
	});
    // save後はtreeにcriticalのアイコンがあったら消す。
    connect(fileTabItem, &FileTabItem::fileSaved, this, &InputViewer::removeCriticalIcon);


    connect(fileTabItem, &FileTabItem::requestOpenFile, this, &InputViewer::openFileTab);
	// openしたらcurrentTabを移動する。
	ui->tabWidget->setCurrentWidget(fileTabItem);

}



FileTabItem *InputViewer::getCurrentItem()
{
	QWidget* currentItem = ui->tabWidget->widget(ui->tabWidget->currentIndex());
	FileTabItem* tabItem = nullptr;
	for(auto it = tabNameMap_.begin(); it != tabNameMap_.end(); ++it) {
		if(it->first == static_cast<FileTabItem*>(currentItem)) tabItem = it->first;
	}
	return tabItem;
}


QString InputViewer::toRelativeFilePath(const QString absFilePath)
{
	assert(!rootDir_.isEmpty());
	return rootDir_.relativeFilePath(absFilePath);
}

QString InputViewer::toAbsoluteFilePath(const QString relativeFilePath)
{
	// rootDir_は入力ファイルのルートフォルダであってカレントフォルダと異なる場合がある。
	assert(!rootDir_.isEmpty());
	return QFileInfo(rootDir_, relativeFilePath).absoluteFilePath();
	//return rootDir_.canonicalPath() + "/" + relativeFilePath;

//	QFileInfo info(rootDir_.canonicalPath() + "/" + relativeFilePath);
//	return info.canonicalFilePath();  // symlinkが辿られるのは好ましくない。
//	return info.absoluteFilePath();  // 相対パスを含んでいる場合置換されてしまうのは好ましくない。
}

// カノニカルなフルパスから現在開いているタブを取得する。なければnullptr
FileTabItem *InputViewer::getTabItembyAbsFilePath(const QString &absFilePath) const
{
	for(auto it = tabNameMap_.begin(); it != tabNameMap_.end(); ++it) {
		if(std::get<0>(it->second) == absFilePath) return it->first;
	}
	return nullptr;
}


std::vector<QString> InputViewer::getOpenedRelativeFileList() const
{
	std::vector<QString> fileNameList;
	for(int i = 0; i < ui->treeWidgetOpenedFiles->topLevelItemCount(); ++i) {
		fileNameList.emplace_back(ui->treeWidgetOpenedFiles->topLevelItem(i)->text(CELLNAME_COLUMN));
	}
	return fileNameList;
}


void InputViewer::saveCurrentFile()
{
	auto tabItem = getCurrentItem();
	if(tabItem == nullptr) return;
	tabItem->save();
}

void InputViewer::reloadCurrentFile()
{
	auto tabItem = getCurrentItem();
	if(tabItem == nullptr) return;
	tabItem->reload();
}


void InputViewer::openAllFiles()
{
	QList<QTreeWidgetItem *> items
			= ui->treeWidgetFiles->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard | Qt::MatchRecursive);

	for(auto &item: items) {
		//mDebug() << "opening file=" << rootDir_.canonicalPath() + "/" + item->text(0);
		openFileTab(rootDir_.canonicalPath() + "/" + item->text(0));
	}
}

// opendFile treeViewのアイテムの
// ファイル名部分がクリックされた場合は現在の表示ファイルをそれにして
// 閉じるボタン部分をクリックされたら閉じる
void InputViewer::handleOpendFileItemClicked(QTreeWidgetItem *item, int column)
{
    QString relativePath = item->text(TRUENAME_COLUMN);
	const QString canonicalPath = toAbsoluteFilePath(relativePath);
	if(column == CELLNAME_COLUMN) {
		setCurrentTabByRelativePath(relativePath);
	} else if (column == CLOSE_BUTTON_COLUMN)  {
		FileTabItem* fileTabItem = this->getTabItembyAbsFilePath(toAbsoluteFilePath(relativePath));
		if(fileTabItem == nullptr) {
			std::vector<QString> openedFiles;
			for(auto it = tabNameMap_.begin(); it != tabNameMap_.end(); ++it) {
				openedFiles.emplace_back(std::get<0>(it->second));
			}
			mFatal("filetab file=", canonicalPath, " is tried to be closed. but not opened.",
				   "opend files =", openedFiles);
		}
		int index = ui->tabWidget->indexOf(static_cast<QWidget*>(fileTabItem));
		this->closeTabItem(index);
	}
}

void InputViewer::setCurrentTabByRelativePath(QString relativePath)
{
	auto canonicalPath = toAbsoluteFilePath(relativePath);
	auto fileTabItem = getTabItembyAbsFilePath(canonicalPath);
	if(fileTabItem != nullptr) {
		ui->tabWidget->setCurrentWidget(fileTabItem);
    }
}

// ファイル保存があった場合、treeのクリティカルアイコンは消す。
// 引数は保存しようとするファイル名のフルパス
void InputViewer::removeCriticalIcon(QString absFilePath)
{

    // currentFileがtreeView内でcriticalアイコン付きならアイコン消去
	std::vector<QTreeWidgetItem*> itemVec;
    for(int i = 0; i < ui->treeWidgetFiles->topLevelItemCount(); ++i) {
        auto item = ui->treeWidgetFiles->topLevelItem(i);
        findItem(toRelativeFilePath(absFilePath), item, &itemVec);
    }
    assert(itemVec.size() <= 1);
	// 後から追加して右クリックでファイルを開いた場合、そのファイルはtreeWidgetに登録されていないので
	// ここでのitemVecはemptyとなりレンジエラーする。
	// ・抜本的に解決するなら 右クリックで開いた時に登録すべし。
	// ・その場しのぎで解決するならemptyチェックすれば良い。
	// とりあえずその場しのぎの方法を採用する。
	//
	// そもそもtreeWidgetを2個使っている設計が悪いので、
	// 抜本的にはモデル1個にview2個構成に切り替えるべし。
	if(!itemVec.empty()) {
		auto item = itemVec.at(0);
		item->setIcon(0, QIcon());
	}
}

void InputViewer::resetStyleSheet()
{
    // ここではQApplicationでのスタイルシート設定に重畳してインスタンスにcss設定する。
//	QString css = "QTabWidget::pane {border: 0 solid white; margin: -9px -9px -9px -9px;}";
	// TabWidgetのデフォルト設定ではタブの中身のマージンが大きすぎるので詰める。
    static const QString css = "QTabWidget::pane {margin: -9px -9px -9px -9px;}";
	//ui->tabWidget->setStyleSheet(css);
	if(ui->tabWidget->styleSheet().isEmpty()){ui->tabWidget->setStyleSheet(css);}
}

