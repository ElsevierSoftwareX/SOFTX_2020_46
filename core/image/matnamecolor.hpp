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
#ifndef MATNAMECOLOR_HPP
#define MATNAMECOLOR_HPP

#include <list>
#include <memory>
#include <string>
#include <map>
#include <vector>

#include "color.hpp"
#include "component/picojson/picojson.h"

namespace inp{
class DataLine;
}

namespace img {

// 材料名、色その他データをまとめたクラス
class MaterialColorData
{
public:

    // static
    static bool isUserDefinedColor(const MaterialColorData& matColData);
    // mat-name-colorセクション入力からMatNameColorのマップを作成。
    // キーは材料名。
    static std::map<std::string, MaterialColorData> fromCardsToMap(const std::list<inp::DataLine> &inputData);
    static MaterialColorData fromJsonString(const std::string &jsonStr);
    static MaterialColorData fromJsonObject(picojson::object obj);

    // コンストラクタ
    MaterialColorData(const std::string &mname, const std::string &pname,
                 double sz, const std::shared_ptr<const img::Color> &col)
        : matName_(mname), aliasName_(pname), printSize_(sz), color_(col)
    {}
    // getter
	const std::string &matName() const {return matName_;}
    const std::string &aliasName() const {return aliasName_;}
	double printSize() const {return printSize_;}
	const std::shared_ptr<const img::Color> &color() const {return color_;}
    // conversion
    std::string toString() const;
	picojson::value jsonValue()const;


private:
    std::string matName_;                      // 計算内部で使用する材料名
    std::string aliasName_;                    // 表示用の別名
    double printSize_;                         // 表示サイズ
    std::shared_ptr<const img::Color> color_;  // 色
};


}// end namespace img
#endif // MATNAMECOLOR_HPP
