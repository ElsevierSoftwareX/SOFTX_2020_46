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
#include "filling_utils.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

#include "core/utils/message.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/math/nmatrix.hpp"


/*
 * 新アルゴリズムへ変更したい。
 *
 * DimDeclaratorを計算
 * 引数
 * ・次元数(これがないと2次元六角形格子と3次元矩形格子の判別ができない)
 * ・格子要素の中心
 * ・格子の面と面を結ぶインデックスベクトルのセット
 * ・基本格子要素の-側平面のセット
 * ・外側セルのBoundingBox
 * 返り値
 * ・インデックス番号の下限値と上限値のペアのセット
 *
 * 方針：格子構成面が外側セル(or セルのBB)と交差しないところまでインデックスを増大させる。
 *       しかし基本要素[000]が外側セル外なら最初から交差しないので注意。
 *
 * 2．外側セルBBと基本要素中心の距離を計算し、ijkインデックスいくつ分に相当するか計算(BB中心の所属する要素番号計算)
 * 3．外側セルBB中心へ単位セルを移動し、そこからインデックスをいくつ進めるまで外側セルBBと交差するか計算
 * 4．range決定。
 *
 * 一応方向ごとに独立して求めることは可能。かとおもいきや、六角形格子の場合不可なのでやっぱり全方向求める。
 */

constexpr int MAX_INDEX = 10000;

namespace {
/*
 * (1,0,0)(0,1,0)(0,0,1)を基底とする位置ベクトルの
 * basis1, basis2, basis3を基底とする空間へ変換した場合の
 * それぞれの係数をarray<3>に入れて返す。
 */
std::array<double, 3> convertPoint(const math::Point &pt,
			 const math::Vector<3> &basis1,
			 const math::Vector<3> &basis2,
			 const math::Vector<3> &basis3)
{
	auto mat = math::Matrix<3>::fromRowVectors({basis1, basis2, basis3});
	if(!mat.isRegular()) throw std::runtime_error("Singular matrix");
	math::Point ivec = pt*mat.inverse();
	// ここでVector<3>::x()を呼んでいるが、x()はdata_[0]を返しているだけで、
	// xyz座標系というわけではない。
	return std::array<double, 3>{ivec.x(), ivec.y(), ivec.z()};
}

}  //end anonymous namespace

// xyz系で表されている点ptをbasis[123]を基底とする座標系での値に変換する。


std::array<std::pair<int, int>, 3>
	calcDimensionDeclarator(int dimension,
							const math::Point &baseUnitCenter,
							const std::vector<math::Vector<3>> &indexVectors,
							const geom::BoundingBox &outerCellBB)
{
	assert(dimension > 0 && dimension < 4);
	static const std::array<std::pair<int, int>, 3> EMPTY_RANGE = {
		std::make_pair(+MAX_INDEX, -MAX_INDEX),
		std::make_pair(+MAX_INDEX, -MAX_INDEX),
		std::make_pair(+MAX_INDEX, -MAX_INDEX),
	};

	bool isHexagon = indexVectors.size() > static_cast<size_t>(dimension);
	/*
	 * bbの頂点をsuv座標系で表し、全ての頂点を包含できるsuvの範囲を求めれば良い。
	 * 六角形格子の場合stv座標系でも同様の手続きを取り、マージする。
	 */
	std::array<std::pair<int, int>, 3> ranges = EMPTY_RANGE;

	for(const auto& pt: outerCellBB.vertices()) {
		math::Vector<3> thirdVector;
		if(dimension == 3) {
			// 3次元の場合
			size_t zindex = (isHexagon) ? 3 : 2;
			thirdVector = indexVectors.at(zindex);
		} else if(dimension == 2) {
			// 2次元格子の場合
			// 第三の方向インデックスベクトルは第一、第二と直交するように選べば
			// 第一、第二方向には影響が出ないので、同様に計算して最終結果の第三要素を捨てれば良い。
			thirdVector = math::crossProd(indexVectors.at(0), indexVectors.at(1)).normalized();
		}
		std::array<double, 3> res = convertPoint(pt - baseUnitCenter,
												 indexVectors.at(0), indexVectors.at(1), thirdVector);

		// resの値に±無限大が含まれる場合は例外発生。
		// だたし2次元格子でres[2]がinftyなのは許容される。
		// 同様に1次元格子でres[1],res[2]がinftyなのも許容される。
		for(int i= 0; i < dimension; i++) {
			if(std::abs(res.at(i)) > std::numeric_limits<int>::max()*0.1) throw std::runtime_error("Infinit lattice");
		}


		/*
			 * convertPointの結果はそのまま四捨五入して整数化すればいい。
			 * 本来原点から遠い方に丸める(正なら+0.5負なら-0.5して四捨五入)する所だが、
			 * s=-0.5〜0.5が s＝0要素の範囲なので座標系とセルのイデックス番号は元々0.5ずれており
			 * 相殺されるから。
			 */
		for(size_t i = 0; i < ranges.size(); ++i) {
			// res.at(i)はintで表現できる最大最小を超えている可能性があるので、その場合は適当に丸める。
			// 現実的な値では無いかもしれないが、大小関係が損なわれていると今のロジックでは対応できないので修正する。
			int rangeValue;
			if(std::round(res.at(i)) < std::numeric_limits<int>::min()) {
				rangeValue = std::numeric_limits<int>::min();
			} else if(std::round(res.at(i)) > std::numeric_limits<int>::max()) {
				rangeValue = std::numeric_limits<int>::max();
			} else {
				rangeValue = static_cast<int>(std::round(res.at(i)));
			}
			ranges.at(i).first = (std::min)(ranges.at(i).first, rangeValue);
			ranges.at(i).second = (std::max)(ranges.at(i).second, rangeValue);
		}

		// 六角形格子の場合stvだけでなくsuvでもチェックしてマージ
		if(isHexagon) {
			res  = convertPoint(pt - baseUnitCenter, indexVectors.at(0), indexVectors.at(2), thirdVector);
			for(size_t i = 0; i < ranges.size(); ++i) {
				int rangeValue;
				if(std::round(res.at(i)) < std::numeric_limits<int>::min()) {
					rangeValue = std::numeric_limits<int>::min();
				} else if(std::round(res.at(i)) > std::numeric_limits<int>::max()) {
					rangeValue = std::numeric_limits<int>::max();
				} else {
					rangeValue = static_cast<int>(std::round(res.at(i)));
				}
				ranges.at(i).first = (std::min)(ranges.at(i).first, rangeValue);
				ranges.at(i).second = (std::max)(ranges.at(i).second, rangeValue);
//				ranges.at(i).first = (std::min)(ranges.at(i).first, static_cast<int>(std::round(res.at(i))));
//				ranges.at(i).second = (std::max)(ranges.at(i).second, static_cast<int>(std::round(res.at(i))));
			}
		}
	}

	// チェックするのは有効な次元まで。
	for(int i = 0; i < dimension; ++i) {
		if(ranges.at(i).first <= -MAX_INDEX || ranges.at(i).second >= MAX_INDEX) throw std::runtime_error("Excess max DimDecl index");
	}
//	for(const auto &range: ranges) {
//		if(range.first <= -MAX_INDEX || range.second >= MAX_INDEX) throw std::runtime_error("Excess max DimDecl index");
//	}


	// 2次元の場合3つ目のdeclaratorは0:0なので0代入, 1次元なら2つめも0:0
	if(dimension <= 2) 	ranges.at(2) = std::make_pair(0, 0);
	if(dimension <= 1) 	ranges.at(1) = std::make_pair(0, 0);
	return ranges;
}
