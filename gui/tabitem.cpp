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
#include "tabitem.hpp"

#include <iostream>
#include "option/guiconfig.hpp"

TabItem::TabItem(QWidget *parent, const QString &text, const GuiConfig *gconf)
	: QWidget(parent), hasInitialized_(false), tabText_(text), guiConfig_(gconf)
{

}

void TabItem::showEvent(QShowEvent *ev)
{
	Q_UNUSED(ev);
	emit tabFocused(true);
}

void TabItem::hideEvent(QHideEvent *ev)
{
	Q_UNUSED(ev);
	emit tabFocused(false);
}

