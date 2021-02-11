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
#ifndef INPUTVIEWER_HPP
#define INPUTVIEWER_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <set>

#include <QDir>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QTreeWidgetItem>


#include "gui/tabitem.hpp"
class FileTabItem;
struct GuiConfig;


namespace Ui {
class InputViewer;
}

class InputViewer : public TabItem
{
	Q_OBJECT
public:
	InputViewer(QWidget *parent = 0, const QString &tabText = "", const GuiConfig* gconf = nullptr);
	~InputViewer();
	void init() override final;
	void retranslate() override final;
	void clearStyleSheet() override final;
	// 再読込時に未保存ファイルの扱い(保存する/しない/再読込キャンセル)を尋ねる。キャンセル選択時にはfalseを返す。
	bool confirmUnsavedFiles();

public slots:
	void receiveOpenedFileName(std::pair<std::string, std::string> files);  // files.firstが親ファイル, secondが開いた子ファイル
	void receiveFatalLog(QSharedPointer<std::string> strP);  // Fatalエラーを受け取って適当な位置にマークを付けて移動
	// Editメニューからのアクションを受ける
	void handleSaveAction();
	void handleReloadAction();
	void handleCopyAction();
	void handleCutAction();
	void handlePasteAction();
	void handleFindAction();
	void handleUndoAction();
	void handleRedoAction();
    void handleCommentRegionAction();
	void handleGuiConfigChanged();
protected:
	void keyPressEvent(QKeyEvent *ev) override;
	void changeEvent(QEvent*event) override final;
	/*どの種類のファイルパスを使うか？
	 * 絶対パス
	 * 相対パス
	 * カノニカルパス ← 基本使わない。symlinkはsymlinkのままにしたいから
	 *
	 * 冗長な表現  ../dir/../dir..  はファイル名の一意性を欠くためタブの復元やtree間の連携を失わせる。
	 * 故に絶対パスは  「パス＋ファイル」 のように単純な形ではなく、一度QFileInfoを経由させる。
	 *
	 */

private:
	Ui::InputViewer *ui;

	// タブアイテムへのポインタをキーにしてtuple<ファイル名(絶対パス), アスタリスク適用済みフラグ, 開いた順番>を格納する
	std::unordered_map<FileTabItem*, std::tuple<QString, bool, size_t>> tabNameMap_;
	// 最初の入力ファイルのフォルダを相対パスインクルード時の起点とする
	QDir rootDir_;
	QString absRootFilePath_;

	// タブ復元用情報
	QString prevActiveTabAbsFileName_;
	QString previousAbsRootPath_;
	// 開いていた順番までは復元するつもりはないからsetで十分
	std::set<QString>previouslyOpendFileNames_;
//	std::map<size_t, QString> previouslyOpendFileNames_;  // keyは開いた順番、valueはファイル名


	// privateメソッド
	void clear();
	// 現在表示しているファイルタブのアイテム
	FileTabItem *getCurrentItem();
	void restorePreviousFileTabs();
	// relative⇔absoluteファイルパスの変換
	QString toRelativeFilePath(const QString absFilePath);
	QString toAbsoluteFilePath(const QString relativeFilePath);
	FileTabItem *getTabItembyAbsFilePath(const QString &absFilePath) const;

	/*
	 * ファイル名、タブインデックス、タブポインタは相互に変換可能(でないと不便)
	 *
	 * ファイルタブポインタ → index番号の取得
	 * index = ui->tabWidget->indexOf(static_cast<QWidget*>(fileTabItem));
	 *
	 * ファイル名 → ファイルタブポインタ
	 * FileTabItem *getTabItembyAbsFilePath(const QString &absFilePath)
	 *
	 * よってファイル名 → index番号は
	 * index = ui->tabWidget->indexOf(static_cast<QWidget*>(getTabItembyAbsFilePath(const QString &absFilePath));
	 *
	 * ファイルタブポインタ → absファイル名
	 *  getCurrentItem()->canonicalFilePath()
	 */

	// 現在開いているファイル(relativeファイルパス)のリスト
	std::vector<QString> getOpenedRelativeFileList() const;

	static size_t counter;  // ファイルをひらいた通算の数。

private slots:
	void closeTabItem(int index);
	void openFileTab(const QString &fileName);
	void saveCurrentFile();
	void reloadCurrentFile();
	void openAllFiles();
	void handleOpendFileItemClicked(QTreeWidgetItem *item, int column);
	void setCurrentTabByRelativePath(QString relativePath);
    void removeCriticalIcon(QString absFilePath);
	void resetStyleSheet();






};

#endif // INPUTVIEWER_HPP
