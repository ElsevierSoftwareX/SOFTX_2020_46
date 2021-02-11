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
#ifndef CELLCOLORPALETTE_HPP
#define CELLCOLORPALETTE_HPP

#include <limits>
#include <list>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "color.hpp"
#include "matnamecolor.hpp"

namespace inp {
class DataLine;
}

namespace img {






/*
 * このクラスの位置づけは要検討
 *
 * registerする時はcell, mat, colorの3要素で登録するが、
 * cel->color が重要なのかmaterial->colorが重要なのか
 * よくわからなくなっている。
 *
 * 同一材料同一色で塗るからmaterial->color対応だけで良いのではないか？？？
 *
 * 最終的には
 * auto color = palette.getColorByIndex(palette.getIndexByCellName(cellName));
 * みたいにセル名から色を求めている。
 *い
 */
class CellColorPalette {
public:
	static constexpr int NOT_INDEX = std::numeric_limits<int>::max();

    CellColorPalette(){}
    bool empty() const {return materialColorDataList_.empty();}
    size_t size() const {return materialColorDataList_.size();}
    void clear();
    void registerColor(const std::string &cellName, const std::string &matName, const Color& color);
    void registerColor(const std::string &cellName, const std::string &matName, const std::string &aliasName, double fontScale, const Color &color);

    const std::shared_ptr<const img::Color> &getColorByCellName(const std::string &cellName) const {return cellColorDataMap_.at(cellName)->color();}

    // 結局の所pixelarray等ではデータをindex番号の配列で持っているのでindexを全廃することは不可能。
    // 個別データ取得.ここでいうindexはセルのIDではなく材料のIDに対応する。
	int getIndexByCellName(const std::string& cellName) const;
	int getIndexByColor(const Color & color) const;

    // 材料名-MaterialcolorDataのマップを作成して返す。
    std::map<std::string, MaterialColorData> colorMap() const;
    const std::vector<std::shared_ptr<MaterialColorData>> &materialColorDataList() const {return materialColorDataList_;}
private:
    /*
     * cell名-- Mat名--MatIndex--colorの対応関係を保持するだけなのに
     * privateにいくつものmapを持つようなデータ構造はおかしい。
     * cell名->その他 だけが多対一でそれ以外は一対一なので
     * Mat名-MatIndex-colorの組を保持するタプルかクラスとcell名のmapにする。
     * ・Material情報としてはMatNameColorクラスがあるのでそれを使う。
     * ・用途はcell名をキーにしてColorを取得する役割が大半なので、cell名をキーにしたアクセスは高速であるべし
     *  → unordered_map<string, MatNameColor*>とvector<shared_ptr<MatNameColor>>を内部で保持。
     *  → vector要素への直接のポインタはresize再配置の時に無効になるからだめ。
     *
     *  とりあえず使っていないメソッドの削除からリファクタリングする
     *
     */
    // セル名をキーとして材料色データを値とするmap
    std::unordered_map<std::string, std::shared_ptr<MaterialColorData>> cellColorDataMap_;
    // uniqueなMaterialColorDataを保持するvector
    std::vector<std::shared_ptr<MaterialColorData>> materialColorDataList_;
};



}  // end namespace img


#endif // CELLCOLORPALETTE_HPP
