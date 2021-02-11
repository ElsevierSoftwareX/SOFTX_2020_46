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
#ifndef BOX_HPP
#define BOX_HPP


#include "macro.hpp"


namespace inp{
class DataLine;
}

namespace geom {
namespace macro{
// マクロは展開用の関数2種類をstaticで持つだけ。

class Box
{
public:
    static const char mnemonic[];          //! マクロボディのニモニックここでは"box";
    static constexpr int numSurfaces = 6;  //! マクロボディ展開で生成する面の数
    /*!
     * \brief expand マクロボディ展開関数
     * \param trMap TR番号をキー、アフィン変換行列を値とするmap
     * \param it マクロボディを定義しているサーフェイスカードへのイテレータ
     * \param surfInputList サーフェイスカードリスト
     * \return  展開されたマクロボディの名前(上の例では1) とニモニック(同RPP)のペア
     *
     * 副作用：
     * この関数よってマクロボディは複数のsurfaceへ展開され、
     * この結果第二引数の指している。DataLineは削除され、
     * 生成された複数のsurfaceのDatalineが第三引数に追加される。
     * たとえば
     *     1 RPP -1 1 -2 2 -3 3
     * は
     *     1.1 PX -1
     *     1.2 PX 1
     *     1.3 PY -2
     *     1.4 PY 2
     *     1.5 PZ -3
     *     1.6 PZ 3
     * のように６平面へと展開されてsurfInputListへ追加される。
     * (厳密にはいくつかの法線は逆向きにされるが)
     */
	static std::pair<std::string, std::string>
				expand(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
						std::list<inp::DataLine>::iterator &it,
						std::list<inp::DataLine> *surfInputList);

    /*!
     * \brief replace セルカードの入力行データ中に存在するマクロボディを展開後の基本面で置換する
     * \param macroBodyName 置換するマクロボディ名
     * \param it 入力データ行へのイテレータ
     */
    static void replace (const std::string macroBodyName,
						 const std::list<inp::DataLine>::iterator &it);

};
}  // end namespace macro
}  // end namespace geoms
#endif // BOX_HPP
