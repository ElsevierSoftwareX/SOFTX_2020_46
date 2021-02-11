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
#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "component/picojson/picojson.h"

#ifdef ENABLE_GUI
#include <QFont>
#endif


namespace utils {

#ifdef ENABLE_GUI
// json QFont関数
picojson::value jsonValue(const QFont &font);
QFont fromJsonObject(picojson::object obj);
#endif

} // end namespace pj

#endif // JSON_UTILS_H
