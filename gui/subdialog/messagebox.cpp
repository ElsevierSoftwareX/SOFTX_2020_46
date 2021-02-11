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
#include "messagebox.hpp"

#include <QMessageBox>


namespace {
constexpr int MAX_MESSAGE_LENGTH = 300;
}  // end anonymous namespace


template<QMessageBox::Icon ICON>
int execMessageBox(QWidget *parent, const QString &title, const QString &message, bool copyable)
{
	QString dispMessage = message;
	if(dispMessage.size() > MAX_MESSAGE_LENGTH) {
		// 前半MAX_MESSAGE_LENGTH文字 + " ...... " + 後ろ20文字で代表させる
		dispMessage = dispMessage.mid(0, MAX_MESSAGE_LENGTH)
				+ " ...*snip*(see log widget on the status bar)... "
				+ dispMessage.mid(message.size()-20);
	}
	QMessageBox qmb(ICON, title, dispMessage, QMessageBox::Ok, parent);
	if(copyable) qmb.setTextInteractionFlags(Qt::TextSelectableByMouse);
	return qmb.exec();
}

int GMessageBox::warning(QWidget *parent, const QString &title, const QString &message, bool copyable)
{
	return execMessageBox<QMessageBox::Warning>(parent, title, message, copyable);
}


int GMessageBox::critical(QWidget *parent, const QString &title, const QString &message, bool copyable)
{
	return execMessageBox<QMessageBox::Critical>(parent, title, message, copyable);
}
