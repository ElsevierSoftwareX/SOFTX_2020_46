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
#include "json_utils.hpp"

#include "message.hpp"

#ifdef ENABLE_GUI
picojson::value utils::jsonValue(const QFont &font)
{
    picojson::object obj;
    obj.insert(std::make_pair("family", font.family().toStdString()));
    obj.insert(std::make_pair("pointSize", static_cast<double>(font.pointSize())));
    obj.insert(std::make_pair("weight", static_cast<double>(font.weight())));
	obj.insert(std::make_pair("italic", font.italic()));
    return picojson::value(obj);
}

QFont utils::fromJsonObject(picojson::object obj)
{
//    picojson::value val;
//    const std::string err = picojson::parse(val, jsonStr);
//    if (!err.empty()) {
//        mFatal("Parsing json file for QFont failed. err = ", err);
//    }
//    picojson::object obj = val.get<picojson::object>();
    auto family = QString::fromStdString(obj["family"].get<std::string>());
    auto pointSize = static_cast<int>(obj["pointSize"].get<double>());
    auto weight =  static_cast<int>(obj["weight"].get<double>());
    auto italic = obj["italic"].get<bool>();

    return QFont(family, pointSize, weight, italic);
}
#endif
