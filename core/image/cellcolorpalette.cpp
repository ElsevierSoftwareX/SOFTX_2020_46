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
#include "cellcolorpalette.hpp"


#include <memory>

#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"


constexpr int img::CellColorPalette::NOT_INDEX;



// セルと材料、index, 色を登録する。
void img::CellColorPalette::registerColor(const std::string &cellName, const std::string &matName,
									 const img::Color &color)
{
    registerColor(cellName, matName, "", 1.0, color);
}

#include <algorithm>

void img::CellColorPalette::registerColor(const std::string &cellName, const std::string &matName,
                                          const std::string &aliasName, double fontScale, const img::Color &color)
{
    // cellName、matNameいずれかが登録済みの場合は登録しない。
    auto it1 = std::find_if(materialColorDataList_.cbegin(), materialColorDataList_.cend(),
                            [matName](const std::shared_ptr<MaterialColorData> &matColData) {
                                return matColData->matName() == matName;
                            });
    auto it2 = cellColorDataMap_.find(cellName);
    if(it1 == materialColorDataList_.cend()) {
        if(it2 == cellColorDataMap_.end()) {
            // material, cellNameともに未登録 → 両方リストに登録する。
            auto matColData = std::make_shared<MaterialColorData>(matName, aliasName, fontScale,
                                                                  std::make_shared<const Color>(color.r, color.g, color.b, color.a));
            materialColorDataList_.emplace_back(matColData);
            cellColorDataMap_.emplace(cellName, matColData);
//            auto col =matColData->color();
//            mDebug() << "RegisterColor for cell ===" << cellName << "color===" << col->r << col->g << col->b;
        } else {
            // material未登録、cellName登録済み。おかしい。
            std::stringstream ss;
            ss << "Color data for cell = " << cellName << " are dupricated, current material = "
               << matName << ", already registered = " << it2->first;
            throw std::invalid_argument(ss.str());
        }
    } else {
        if(it2 == cellColorDataMap_.end()) {
            // material登録済み, cellName未登録 → MaterialColorDataは生成せず、cellNameのみ新規登録
            cellColorDataMap_.emplace(cellName, *it1);
        }
        // material登録済み、cellNam登録済みの場合は何もしない。
    }
}

void img::CellColorPalette::clear()
{
    materialColorDataList_.clear();
    cellColorDataMap_.clear();
}

int img::CellColorPalette::getIndexByCellName(const std::string &cellName) const
{
    if(cellColorDataMap_.find(cellName) == cellColorDataMap_.end()) {
        return NOT_INDEX;
    } else {
        return getIndexByColor(*cellColorDataMap_.at(cellName)->color().get());
    }
}

int img::CellColorPalette::getIndexByColor(const img::Color &color) const
{
    auto itr = std::find_if(materialColorDataList_.cbegin(), materialColorDataList_.cend(),
                            [color](const std::shared_ptr<MaterialColorData> &matColData) {
                                return color == *matColData->color().get();
    });
    if(itr == materialColorDataList_.cend()) {
        return NOT_INDEX;
    } else {
        return static_cast<int>(std::distance(materialColorDataList_.cbegin(), itr));
    }
}

std::map<std::string, img::MaterialColorData> img::CellColorPalette::colorMap() const
{
    std::map<std::string, img::MaterialColorData> retMap;
    for(const auto& matColData: materialColorDataList_) {
        retMap.emplace(matColData->matName(), *matColData.get());
    }
    return retMap;
}

