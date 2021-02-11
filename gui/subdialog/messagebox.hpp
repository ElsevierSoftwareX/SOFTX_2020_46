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
#ifndef MESSAGEBOX_HPP
#define MESSAGEBOX_HPP

#include <QString>
#include <QWidget>

namespace GMessageBox{
int warning(QWidget *parent, const QString &title, const QString &message, bool copyable);
int critical(QWidget *parent, const QString &title, const QString &message, bool copyable);
}

#endif // MESSAGEBOX_HPP
