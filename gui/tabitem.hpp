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
#ifndef TABITEM_HPP
#define TABITEM_HPP

#include <cassert>
#include <QWidget>

struct GuiConfig;


class TabItem : public QWidget
{
	Q_OBJECT
public:
    explicit TabItem(QWidget *parent = nullptr, const QString &text = "", const GuiConfig* gconf = nullptr);

	virtual void init(){hasInitialized_ = true;}
	virtual void retranslate() = 0;
	// タブによってはwidgetパーツのインスタンスにcssを適用したい。がそれをするとQApplicationでcss変更した時にsegvする。
	// これを防止するためには個別のインスタンスのcssをクリアする必要がある。
	virtual void clearStyleSheet(){;}
	// タブの中身をエクスポートするインターフェイス
	virtual void exportToRasterGraphics() {assert(!"exportToRaster should be implemented or shouldn't be called.");}
	virtual void exportToVectorGraphics() {assert(!"exportToVector should be implemented or shouldn't be called.");}

	void setIndex(int index) {index_ = index;}
	int index() const {return index_;}
	const QString &tabText() const {return tabText_;}

signals:
	void tabFocused(bool);

protected:

	virtual void showEvent(QShowEvent *ev) override;
	virtual void hideEvent(QHideEvent *ev) override;

	int index_;
	bool hasInitialized_;
	QString tabText_;


	const GuiConfig *guiConfig_;
};

#endif // TABITEM_HPP
