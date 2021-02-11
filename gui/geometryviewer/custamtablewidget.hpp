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
#ifndef CUSTAMTABLEWIDGET_HPP
#define CUSTAMTABLEWIDGET_HPP

#include <string>
#include <unordered_map>

#include <QCheckBox>
#include <QTableWidget>

// セルに対するkeyイベントを受け付けるためのQTableWidget継承クラス
class CustamTableWidget : public QTableWidget
{
	 Q_OBJECT
public:
	CustamTableWidget(QWidget *parent = 0);
	void setCellNameItem(int row, QTableWidgetItem *item);
	void setCellCheckWidget(int rowNum, QCheckBox *box);
	QCheckBox *getCheckBox(int row);
	void clear();

public slots:
	void setAllCheckBox(bool check);  // trueなら全チェックつけfalseなら全チェック外し
	void restoreDefaultCheckData();  // 初期チェックボックス状態を復元する。
protected:
	void keyPressEvent(QKeyEvent *e);


private:
	// セルデータを読み込んだ時にデフォルトでどの列にチェックを入ったか保持しておく
	QVector<bool> defaultCheckData_;
	// セル名->行番号の対応関係を格納したマップ
	std::unordered_map<std::string, int> cellRowMap_;
};

#endif // CUSTAMTABLEWIDGET_HPP
