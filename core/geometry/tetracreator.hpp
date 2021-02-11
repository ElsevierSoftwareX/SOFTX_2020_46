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
#ifndef TETRACREATOR_HPP
#define TETRACREATOR_HPP

#include <array>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"
#include "core/io/input/cellcard.hpp"


namespace geom {

class SurfaceMap;


//#ifdef ENABLE_GUI
//#include <QObject>
//class TetraCreator : public QObject
//{
//	Q_OBJECT
//signals:
//	// ファイルのオープンに成功したら親ファイルとのペアをemitする。
//	void fileOpenSuceeded(std::pair<std::string, std::string> files);
//#else
//class TetraCreator
//{
//#endif
class TetraCreator
{
public:
	explicit TetraCreator(const inp::CellCard &latticeCard);
	std::vector<inp::CellCard> createTetraCards(const std::string &selfUnivName,
												const std::unordered_map<size_t, math::Matrix<4>> &trMap,
												geom::SurfaceMap *smap);

private:
	double tsfac_;
	std::string baseName_;
	const inp::CellCard latticeCellCard_;
//	const std::unordered_map<size_t, math::Matrix<4>> trMap_;
	size_t maxElemId_;
	bool checkOuterCell_;  // 要素の論理多項式に外側セルの論理式を追加するか？のフラグ
	std::string universe_;  // latticeセルードが定義しているuniv名

	// 残念ながらnode番号は0から順になっていることは保証されないのでvectorではなくmapを使う
	// nodes_.at(番号)でアクセスできるし、なければ例外になってくれる
	std::unordered_map<std::size_t, math::Point> nodes_;
	// elemntIDをキーにして elementファイル行番号、位相情報array, 要素充填univ名のタプルmap
	std::unordered_map<std::string, std::tuple<size_t, std::array<std::size_t, 4>, std::string>> elements_;
};

}
#endif // TETRACREATOR_HPP
