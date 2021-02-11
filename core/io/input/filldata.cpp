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
#include "filldata.hpp"

#include <memory>
#include <regex>
#include <stdexcept>
#include "baseunitelement.hpp"
#include "cellcard.hpp"
#include "filling_utils.hpp"
#include "core/geometry/cellcreator.hpp"
#include "core/geometry/surfacecreator.hpp"
#include "surfacecard.hpp"
#include "core/geometry/cell/cell.hpp"
#include "core/utils/utils.hpp"
#include "core/utils/message.hpp"

namespace {
constexpr double NUM_ELEM_FACTOR = 1.31;
constexpr std::size_t BB_CALC_LIMIT_MSEC = 20000;
constexpr int MAX_INDEX = 1000;



/*
 * elementCenter: lattice要素中心
 * displaceVector: 対抗面を結ぶベクトル
 * outerCellPairs: 「fillされている外側セルのスマポとfill適用時のTR文字列ペア」をベクトルにまとめたもの
 *
 * 返り値は 「最小マイナス側要素番号、最大プラス側要素番号」のペア
 */
//  [0 0 0]要素がFILLedセル内に無くてもOKにしたい mcnp.example4.3.3 → 外側BBを元に計算する新アルゴリズムで対応
std::pair<int, int> calcFillRange(math::Point elementCenter, const math::Vector<3> &displaceVector,
								const std::unordered_map<size_t, math::Matrix<4>> &trMap,
								const std::vector<std::pair<std::shared_ptr<const geom::Cell>, std::string>> &outerCellPairs)
{
	if(outerCellPairs.empty()) throw std::invalid_argument("Can not calculate DimDeclarator, no filled cell is given");

	/*
	 * 旧アルゴリズム
	 *
	 * 外側セルのindex方向のサイズを元に1.3倍程度の幅を充填するように要素数を決定する。
	 * 非効率な実装であることは警告する。
	 * 1.セル要素[0 0 0]の中心を検知 (=elementCenterで既に求まっている)
	 * 2.+-xyz方向の距離を調べる
	 * 3. 要素のサイズを調べる
	 * 4．dimension declaratorを作成する
	 */


	// 1．基本要素[0 0 0]の中心を求める

	// 2．+-方向の長さを調べる
	//    lattice定義セルではなくlatticeがfillされているセルの大きさを調べる必要がある。
	//    可能性としては一つのLatticeが複数のセルからfillされている場合が有り得る。
	//    その場合dimension declaratorは両方をカバーできるように方向ごとに多い方の要素数を採用する。
	double plusCellSize = 0, minusCellSize = 0;
	for(auto outerCellPair: outerCellPairs) {
		const std::shared_ptr<const geom::Cell> &outerCell = outerCellPair.first;
		std::string trString = outerCellPair.second;
		math::Matrix<4> matrix = (trString.empty()) ? math::Matrix<4>::IDENTITY()
													:utils::generateTransformMatrix(trMap, trString);
		math::affineTransform(&elementCenter, matrix);
		auto rotMat = matrix.rotationMatrix();

//		mDebug() << "outerCell=" << outerCell->toString();
//		mDebug() << "trString=" << trString;

		auto plusDir = (displaceVector*rotMat).normalized();
		auto minusDir = -plusDir;
//		math::Point plusPoint = (outerCell->getNextIntersection(elementCenter, plusDir)).second;
//		math::Point minusPoint = (outerCell->getNextIntersection(elementCenter, minusDir)).second;
		math::Point plusPoint = (outerCell->getFarestIntersection(elementCenter, plusDir)).second;
		math::Point minusPoint = (outerCell->getFarestIntersection(elementCenter, minusDir)).second;
		if(!plusPoint.isValid() || !minusPoint.isValid()) {
			mDebug() << "plusPoint=" << plusPoint;
			mDebug() << "minusPoint=" << minusPoint;
			throw std::runtime_error("automatic adjustment for dimension declarator failed (element[0 0 0] is out of outer cell)."
										"Specify explicitly.");
		}
		double plusDistance = (plusPoint - elementCenter).abs();
		double minusDistance = (minusPoint - elementCenter).abs();

		if(plusCellSize < plusDistance) plusCellSize = plusDistance;
		if(minusCellSize < minusDistance) minusCellSize = minusDistance;
//		mDebug() << "Center=" << elementCenter << "+dir=" << plusDir << "交点=" << plusPoint << "dist=" << plusDistance;
//		mDebug() << "Center=" << elementCenter << "-dir=" << minusDir << "交点=" << minusPoint << "dist=" << minusDistance;
	}
	double disp = displaceVector.abs();
    return std::make_pair(-std::lround(NUM_ELEM_FACTOR*std::ceil(minusCellSize/disp - 0.5)),
                           std::lround(NUM_ELEM_FACTOR*std::ceil(plusCellSize/disp - 0.5)));
    //return retPair;
}

}  // end anonymous namespace


int inp::FillingData::size(std::size_t index) const {
	switch(index) {
	case 0: return isize();
	case 1: return jsize();
	case 2: return ksize();
	default: throw std::out_of_range("Filling range index should be < 3, value=" + std::to_string(index));
	}

}

const std::pair<int, int> &inp::FillingData::range(std::size_t index) const
{
	switch(index) {
	case 0: return irange_;
	case 1: return jrange_;
	case 2: return krange_;
	default: throw std::out_of_range("Filling range index should be < 3" + std::to_string(index));
	}
}

void inp::FillingData::init(const geom::CellCreator *creator,
							 const std::unordered_map<std::string, inp::CellCard> *solvedCards,
							 const inp::CellCard &latticeCard,
							 const std::vector<std::string> &surfaceNames,
							 int latvalue,
							 bool *isSelfFilled)
{
	// lattice要素[0 0 0]のxyzそれぞれの-面〜+面の方向ベクトル（軸ベクトルの定数倍）を計算するにはsurfaceMapが必要。
	/*
	 * surfaceNamesの入力は4, 6, 8があり得る。
	 * 4：平行2平面2ペアの   矩形2次元格子
	 * 6：平行2平面3ペアの   矩形3次元格子 or 2次元六角形格子
	 * 8：平行2平面4ペアの   六角形3次元格子
	 * が、DimDeclaratorは"必ず三次元"で記述するので次元数がいくつかというのは問題にならない。
	 * むしろセル定義に必要な平面数(2*numIndex_)が重要
	 */

	// ####################### 単位要素の計算

	baseUnitElement_ = BaseUnitElement::createBaseUnitElement(latvalue, surfaceNames, creator->surfaceCreator()->map());

	// DimDeclaratorが手動で与えられている時はここで終わり。そうでなければ自動計算する。
	if(hasDimDeclarator_) return;

	// ################# 単位要素セルの計算終わり。以下はDimension Declaratorの自動推定。 #######################

	// 「外側セル、univに掛かっているTR文字列」をペアのfirstとsecondにまとめる。
	// (lattice含む)universeは複数の外側セルから参照される場合があるため、ここではペアのvectorに格納している。
	std::vector<std::pair<std::shared_ptr<const geom::Cell>, std::string>> outerCellPairs;
	for(auto &cardPair: *solvedCards) {
		const inp::CellCard& targetCard = cardPair.second; // targetCardはこのLatticeでfillしている外側カードを検索中
		if(targetCard.parameters.find("fill") != targetCard.parameters.end()) {
			auto univSet = targetCard.fillingUniverses();  // univSetはuniv名をキーに、適用されるTRをvalueにしたmultimap
			if(univSet.find(latticeCard.parameters.at("u")) != univSet.end()) {
				//mDebug() << "univ=" << latticeCard.parameters.at("u") << "is used in cellcard=" << targetCard.toInputString();

				/*
				 *  ここまできたらtargetCardは
				 * ・targetCardはfillパラメータを持つ
				 * ・targetCardの充填にはlatticeCardの定義しているuniverseが使われている。
				 */

				// univSetはuniv名をキーにした、外側セルでの充填時にそのunivに適用されているTR引数 のマルチマップなので
				// univRangeはlatticeCardの定義しているuniv番号をfirstに、TR引数をsecondにしたペアへの
				auto univRange = univSet.equal_range(latticeCard.parameters.at("u"));
				for(auto it = univRange.first; it != univRange.second; ++it) {
					outerCellPairs.emplace_back(std::make_pair(creator->createCell(targetCard), it->second));
				}
			}
		}
	}


	if(!outerCellPairs.empty()) {
		try{
			std::array<std::pair<int, int>, 3> rangeArray{{
				{MAX_INDEX, -MAX_INDEX}, {MAX_INDEX, -MAX_INDEX}, {MAX_INDEX, -MAX_INDEX}
			}};
			for(const auto& cellPair: outerCellPairs) {
				const std::shared_ptr<const geom::Cell> cell = cellPair.first;
				std::string trString = cellPair.second;

				// ここのtrStringのTRってbaseUnitに適用するんだっけ？ 外側セルに適用するんだっけ？
				// → FILL時に適用するTRなので単位要素へ適用する。

				math::Point center = baseUnitElement_->center();
				std::vector<math::Vector<3>> indexVecs = baseUnitElement_->indexVectors();
				if(!trString.empty()) {
					auto mat = utils::generateTransformMatrix(creator->surfaceCreator()->transformationMatrixMap(), trString);
					math::affineTransform(&center, mat);
					// 方向ベクトルは平行移動させる意味がないので回転のみ適用する。
					mat.setTranslationVector(math::Vector<3>{0, 0, 0});
					for(auto &vec: indexVecs) math::affineTransform(&vec, mat);
				}
				geom::BoundingBox outerCellBB = cell->boundingBox(BB_CALC_LIMIT_MSEC);


//				mDebug() << "Argument, center ===" << center;
//				mDebug() << "index vecs ===" << indexVecs;
//				mDebug() << "outer cell BB ===" << outerCellBB.toInputString();
//				assert(!"Debug end");

				auto tmpArray = calcDimensionDeclarator(baseUnitElement_->dimension(),
												  center, indexVecs,
												  outerCellBB);
				for(size_t i = 0; i < rangeArray.size(); ++i) {
					rangeArray.at(i).first = (std::min)(rangeArray.at(i).first, tmpArray.at(i).first);
					rangeArray.at(i).second = (std::max)(rangeArray.at(i).second, tmpArray.at(i).second);
				}

			}
			irange_ = rangeArray.at(0);
			jrange_ = rangeArray.at(1);
			krange_ = rangeArray.at(2);

		} catch (std::runtime_error &re) {
			(void) re;
			mWarning() << "Detailed estimation of dimension declarator failed. reason =" << re.what()
					   << " Trying alternative method ... ";
			try{
				// 新アルゴリズムが失敗したのでわりといい加減な旧アルゴリズムへフォールバック
				auto trMap = creator->surfaceCreator()->transformationMatrixMap();
				this->irange_ = calcFillRange(baseUnitElement_->center(), baseUnitElement_->indexVectors().at(0), trMap, outerCellPairs);
				this->jrange_ = calcFillRange(baseUnitElement_->center(), baseUnitElement_->indexVectors().at(1), trMap, outerCellPairs);
				if(dimension() < 3) {
					this->krange_ = std::make_pair(0, 0);
				} else {
					this->krange_ = calcFillRange(baseUnitElement_->center(), baseUnitElement_->indexVectors().back(), trMap, outerCellPairs);
				}
			} catch (std::runtime_error &e) {
				// TODO それでもだめなら 適当な値を使おう。
				mWarning() << e.what() << " Default value used.";
				irange_ = std::make_pair(-10, 10);
				jrange_ = std::make_pair(-10, 10);
				krange_ = std::make_pair(-5,5);
			}
		}

		// rangeを決めたのでrangeに併せてfillしているuniverseを格納するコンテナの中身も作る。
		int fillsize = (this->irange_.second - this->irange_.first + 1)
				*(this->jrange_.second - this->jrange_.first + 1)
				*(this->krange_.second - this->krange_.first + 1);
		this->universes_ = std::vector<std::string>(fillsize, latticeCard.parameters.at("fill"));

	} else {
		// 場合によっては外側セルがない(=fillに使われていない)lattice定義セルがある。
		// そのようなセルは要素数計算もインスタンス作成もする必要がない。
		mWarning(latticeCard.pos()) << "Lattice cell is defined but not used.";
		// dimension declaratorを生成しないようにrange.first > range.secondに設定する。
		// SELFユニバースも生成しないようにisSelfFilled＝falseを適用する。
		this->irange_ = std::make_pair(0, 0);
		this->jrange_ = std::make_pair(0, 0);
		this->krange_ = std::make_pair(0, 0);
		*isSelfFilled = false;
	}

	mDebug().setSpacing(false) << "Estimated dimension declarator lattice cellcard "
							   << latticeCard.name << " = "
							   << this->irange_.first << ":" << this->irange_.second << " "
							   << this->jrange_.first << ":" << this->jrange_.second << " "
							   << this->krange_.first << ":" << this->krange_.second;
}




std::string inp::FillingData::toInputString() const {
	if(!hasDimDeclarator_) {
		assert(universes_.size() == 1);
		return universes_.front();
	} else {
		std::string retstr =  std::to_string(irange_.first) + ":" + std::to_string(irange_.second) + " "
				+ std::to_string(jrange_.first) + ":" + std::to_string(jrange_.second) + " "
				+ std::to_string(krange_.first) + ":" + std::to_string(krange_.second) + " ";
		for(auto &u: universes_) {
			retstr += " " + u;
		}
		return retstr;
	}
}

// FILLパラメータで指定されているunivers名を取得する。第二引数は*FILL=で全体がdegree指定されている場合true
std::string getUnivString(const std::string &univArgStr, bool isDegree)
{
	//mDebug() << "isDegree=" << isDegree << ", get universe string from =" << univArgStr;
	auto pos = univArgStr.find_first_not_of(" ");
	if(pos == std::string::npos) {
		throw std::invalid_argument("Empty fill argument string");
	} else if (univArgStr.at(pos) == '(' || univArgStr.at(pos) == ')') {
		throw std::invalid_argument("Universe string should not start with \"()\", str=" + univArgStr);
	}
	std::smatch trMatch;
	std::string name="", tr="";

	// universeのTRは必ず（）でくくられており、regex_searchは最長一致なので()の中に再度()が出てきても問題ない。
    static std::regex univRegex(R"( *(\w+) *\(([-+*/%{}(). \w]*)\))");
	if(std::regex_search(univArgStr, trMatch, univRegex)) {
		name = trMatch.str(1);
		tr = trMatch.str(2);  // trMatch.str(2)は一番外側の括弧は外されている。
//        mDebug() << "univName =" << name << "tr=" << tr;
	} else {
		name = univArgStr;
	}
	utils::trim(&name);
	utils::trim(&tr);

	if(name.find('=') != std::string::npos) {
		throw std::invalid_argument("Invalid universe name \"" + name + "\"");
	}

	if(!tr.empty()) {
		return (!isDegree || tr.front() == '*') ? name + "(" + tr + ")"
		                                        : name + "(*" + tr + ")";
	} else {
		return name;
	}
}

inp::FillingData inp::FillingData::fromString(const std::string &fillStr)
{
	//mDebug() << "文字列からFillingDataの作成。文字列=" << fillStr;

	FillingData filldata;
	filldata.hasDimDeclarator_ = false;

	// fill=u1 (x y z...)  とfill= 0:0 1:2 2:3 1 2(x y z...) 3 4 ... の場合と両方ある。
	// それぞれに角度入力*FILLの場合があり得る。
	/*
	 * dimension declaratorがある場合の処理
	 * 1．declarator全体を読んで要素数を確定させる
	 * 2．要素数分だけ充填univ番号を読み取る。
	 * 3. fill=の頭から充填univ番号の最終要素までの文字列をcellStrから削除する。
	 */
	std::smatch declaratorMatch, univMatch;
	std::regex declPattern = CellCard::getDeclPattern();
//	std::regex univPattern(R"(\w+ *\([-+ 0-9]+\)|\w+)");
	std::regex univPattern(R"(\w+ *\([-+*/%0-9a-zA-Z .{}()]+\)|\w+)");  // TRの()内には文字列が入り得る(fortran)



	std::smatch fillMatch;
	// =はオプショナルなので無くてもOKにする必要がある。セルカードは小文字化が確定しているので小文字だけでよい。
	if(!std::regex_search(fillStr, fillMatch, std::regex(R"((\**fill)( *[ =] *))"))) {
		throw std::invalid_argument("Invalid fill data string \"" + fillStr + "\"");
	}
	std::string valueStr(fillMatch[2].second, fillStr.cend());
	filldata.isDegree_ = fillMatch.str(1).front() == '*';
//    mDebug() << "fillstr===" << fillStr;
//    mDebug() << "In FillingData::fromString, fillvaluestr===" << valueStr << "isdegree=" << filldata.isDegree();

	//fill= N:... のNからdeclaratorを検索する。
	if(!std::regex_search(valueStr, declaratorMatch, declPattern)) {
		// Declaratorが見つからない場合。このときは単一U指定化不完全なdeclaratorかのどちらか。
		if(!std::regex_search(valueStr, univMatch, univPattern)) {
			throw std::invalid_argument("Invalid universs string =" + valueStr);
		}
		filldata.universes_.emplace_back(getUnivString(univMatch.str(0), filldata.isDegree_));
		filldata.hasReadCount_ = std::distance(valueStr.cbegin(), univMatch[0].second);
		return filldata;
	}
	// declaratorが見つかった場合
	filldata.hasDimDeclarator_ = true;
	filldata.irange_ = std::make_pair(std::stoi(declaratorMatch.str(1)), std::stoi(declaratorMatch.str(2)));
	filldata.jrange_ = std::make_pair(std::stoi(declaratorMatch.str(3)), std::stoi(declaratorMatch.str(4)));
	filldata.krange_ = std::make_pair(std::stoi(declaratorMatch.str(5)), std::stoi(declaratorMatch.str(6)));


	int xsize = filldata.irange_.second - filldata.irange_.first + 1;
	int ysize = filldata.jrange_.second - filldata.jrange_.first + 1;
	int zsize = filldata.krange_.second - filldata.krange_.first + 1;
	if(xsize <= 0 || ysize <= 0 || zsize <= 0) {
		throw std::invalid_argument(std::string("latice size should be > 0, (x, y, z) size = (")
									+ std::to_string(xsize) + ", " + std::to_string(ysize)
									+ ", " + std::to_string(zsize) + ")");
	}
//	mDebug() << "DimDeclsize===" << xsize << ysize << zsize;
	// latticeサイズがわかったのでsm3[0].secondから x*y*z個 要素univ番号を読み取る。
	// universe番号列は空白と数字" 2 2 1"に加えてTR適用ケース 5(10 0 5) みたいな場合もある。


	// ##### ここまででDimension確定。#####

	// univにTRが付く場合があり、TRにFortran数式が含まれる場合があるので、簡単な正規表現では判断できない。
	std::string fillArgStr = std::string(declaratorMatch[0].second, valueStr.cend());  // fillArgStrはDimDeclを除いた後半部分
	if(fillArgStr.empty()) throw std::invalid_argument("Filling argument string is empty");

	// fillArgの空白は意味がないので除去したい。…がfillStrのうち何文字までがfillパラメータなのか
	// 正しく認識しなければならないのでempty要素を削除してはならない
	std::vector<std::string> fillArgVec = utils::splitString(std::make_pair('(', ')'), " ", fillArgStr, false);
//	mDebug() << "fillArgStr========" << fillArgStr;
//	mDebug() << "fillArgVec(スペース削除前)========" << fillArgVec;

	// filling引数の最初のuniv番号が前に無いところでfill時TR番号が来たらエラー
    /*
     * ・fillArgVecがempty
     * ・fillArgVecの要素がから文字列のみ
     * ・fillArgVecの最初の非empty要素が'('で始まる
     * 場合はエラー
     */
    if(fillArgVec.empty()) throw std::invalid_argument("Argument of fill parameter is empty.");
    auto fillFrontIt = fillArgVec.begin();
    while(fillFrontIt->empty() && fillFrontIt != fillArgVec.end()) {++fillFrontIt;}
    if(fillFrontIt == fillArgVec.end()) {
        throw std::invalid_argument("No valid argument found for fill parameter.");
    } else if(fillFrontIt->front() == '(') {
        throw std::invalid_argument("Filling string should not start with TR number. string=" + fillArgStr);
    }

    int numRemovedSpaces = 0;  // 削除したスペースの数を記録しておく。後でfillの引数部分の最後を認識するのに必要

    // fillArgVecの先頭からのempty要素を削除して詰める
    if(fillFrontIt != fillArgVec.begin()) {
        numRemovedSpaces = static_cast<int>(std::distance(fillArgVec.begin(), fillFrontIt));
        fillArgVec.erase(fillArgVec.begin(), fillFrontIt);
    }

// FIMXE ここのit->front()で落ちる。

// fill= 1 (2) のようにfill時TR番号とuniv番号が離れている場合があるので間を詰める。
	// 具体的にはfillArgVecの要素の最初の文字が(なら、emptyではないところまで前の要素に連結して、今の要素を削除する。
	for(auto it = fillArgVec.begin()+1; it != fillArgVec.end();) {
        if(!it->empty() && it->front() == '(') {
			decltype(it) prevItr;
			std::string prevString;
			// 複数個スペースが入っている場合があるのでprevItrは!empty()になる所まで遡っていく
			do {
				if(it == fillArgVec.begin()) {
					// 連結していった結果、先頭まで戻った場合さらに遡り連結を試行してはならない]
					std::stringstream ss;
					for(size_t i = 0; i < fillArgVec.size(); ++ i) {
						ss << fillArgVec.at(i);
						if(i != fillArgVec.size()-1) ss << ", ";
					}
					throw std::invalid_argument("No valid universe exists in front of fill arguments, args=" + ss.str());
				}
				prevItr = it-1;
				prevString = *prevItr;
				*prevItr = *prevItr + *it;
				it = fillArgVec.erase(it);
				--it;
				++numRemovedSpaces;
			} while (prevString.empty());
		} else {
			++it;
		}
	}

//	mDebug() << "fillArgVec(スペース削除あと)========" << fillArgVec;

//	mDebug();
//	for(size_t j = 0; j < fillArgVec.size(); ++j) {
//		mDebug() << "j=" << j << "str=\"" << fillArgVec.at(j);
//	}

	std::string buff;
	size_t numChar = 0;  // 読み込んだ文字数
	size_t j = 0;         //  j-1が区切り文字' 'の数
	try {
		for(int i = 0; i < xsize*ysize*zsize; ++i) {
			//mDebug() << "iForループ最初でのj=" << j;
			buff = fillArgVec.at(j);
			++j;
			if(buff.empty()) {
				--i;// buffが空ならunivは読み取れないのでiはデクリメントする
			} else {
				numChar += buff.size();
				auto univ = getUnivString(buff, filldata.isDegree_);
//				mDebug() << "univbuff===" << buff;
//				mDebug() << "univStr=" << univ;
				filldata.universes_.emplace_back(univ);
				buff.clear();
			}
		}
	} catch(std::out_of_range &e) {
        (void) e;
		throw std::invalid_argument("Too few universes in lattice cell card, expected = " + std::to_string(xsize*ysize*zsize)
									+ ", actual = " + std::to_string(j) + ".");
	}

	// numChar が文字数、 j-1が区切り文字' 'の数
	size_t declSize = std::distance(declaratorMatch[0].first, declaratorMatch[0].second);
	filldata.hasReadCount_ = numChar + j-1 + declSize + numRemovedSpaces;
//	mDebug() << "numChar=" << numChar << "j=" << j << "declSize=" << declSize << "numRemovedSpaces=" << numRemovedSpaces;
//	mDebug() << "readCount=" << filldata.hasReadCount_;


	return filldata;
}

