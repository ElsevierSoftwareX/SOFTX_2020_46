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
#include "cell.hpp"

#include <cstdlib>
#include <sstream>
#include <thread>  // for debug
#include <utility>
#include <vector>
#include "core/geometry/surface/plane.hpp"
#include "core/utils/message.hpp"
#include "core/material/material.hpp"
#include "core/math/nvector.hpp"
#include "core/geometry/cell/bb_utils.hpp"
#include "core/geometry/cell/boundingbox.hpp"

const char geom::Cell::UNDEF_CELL_NAME[] = "*C_u";
const char geom::Cell::VOID_CELL_NAME[]= "*C_v";
const char geom::Cell::UBOUND_CELL_NAME[] = "*C_ub";
const char geom::Cell::BOUND_CELL_NAME[] = "*C_b";
const char geom::Cell::DOUBLE_CELL_NAME[] = "*C_d";
const char geom::Cell::OMITTED_CELL_NAME[] = "*C_o";


std::shared_ptr<const geom::Cell> geom::Cell::undefinedCell_ = nullptr;

namespace {


constexpr double AIR_DENSITY_CRITERIA = 0.0015;
// PV=nRTより標準状態ではn=2.686E+19 (1/cm3)
// mcnpでは1e24/cm3なので n=2.686E-5 (1e+24/cm3)
// 裕度として1桁見ておくので 基準値は2.686E-4とする。
constexpr double AIR_NUMBER_DENSITY_CRITERIA = 2.686E-4;

}

/*
 * コンストラクタでは
 * 1．隣接(=セル定義論理式に出てくる)Surfaceをsurface名で引き出せるようにグローバルなsurfaceマップから
 *    セルで使用しているsurfaceのみの局所Surfaceマップ(contactSurfaceMap_)を作成しておく
 *
 */
geom::Cell::Cell(const std::string &name,
				 const Surface::map_type &globalSurfaceMap,
				 const lg::LogicalExpression<int> &poly,
				 double imp)
	:cellName_(name), polynomial_(poly),
	 importance_(imp), material_(mat::Material::void_material_ptr()), density_(0)
{
	// 接触しているsurfaceをcontactSurfaceMapに取得。（厳密にはbool多項式に冗長な部分があるので接触しているとは限らない）
    if(cellName_ != UNDEF_CELL_NAME) assert(!poly.uniqueFactors().empty()); // UndefCell以外は論理多項式が空では駄目。

//	mDebug() << "\nCellname===" << cellName_ << " definition polynomial =" << poly.toString();
//	mDebug() << "polyu.uniq=" << poly.uniqueFactors();
	//mDebug() << "globalSurface=" << globalSurfaceMap.toString();
	// Boolean多項式のfactorはSurface名を表している。
	for(auto& factor: poly.uniqueFactors()) {
		try{
			/*
			 * マイナス符号のついたsurfaceは法線が逆向きになったsurfaceとして別のsurfaceとして扱う
			 * つまり"s"と"-s"は別のsurfaceとして扱う(位置と形状は同じだけど)
			 */
			// contatcSurfaceMap_はmap<string, shared_ptr<Surface>>でconstがついていない。
			// これは後からcontactSurfaceMap_を手繰ってCellとsurfaceの接続関係を更新するため。
			// 型が合わないのでshared_ptr<const Surface>からconstを外す。
			contactSurfacesMap_.registerSurface(factor, std::const_pointer_cast<Surface>(globalSurfaceMap.at(factor)));
//			contactSurfacesMap_[factor] = std::const_pointer_cast<Surface>(globalSurfaceMap[factor]);


			// 論理多項式中に出てきた面は、その裏側も接触面として登録する。
			// 登録面数は倍になるが、登録しないと-Sと+Sを別面扱いできない。交差判定は表面のみとすることで計算量を減らしている。
			contactSurfacesMap_.registerSurface(-factor, std::const_pointer_cast<Surface>(globalSurfaceMap.at(-factor)));

		} catch (std::out_of_range & e) {
			//mDebug() << __FILE__ << __LINE__ << e.what();
			std::stringstream ss;
            ss << "Surface \"" << factor << "\" not found." << e.what();
			throw std::out_of_range(ss.str());
		}
	}
	// cell定義に利用したsurfaceのcontactCellMapにこのセルを登録したい。
	/*
	 * https://cpprefjp.github.io/reference/memory/enable_shared_from_this.html
	 * "thisポインタを単純にshared_ptr<T>(this)としてしまうと、参照カウントが増えず、
	 * deleteが2重に呼ばれてしまいバグを引き起こす"
	 *
	 * どうしてもshared_ptr(this)したければ
	 * std::enable_shared_from_this を使う。
	 * が、コンストラクタ内ではやはり使えない。
	 */
//	mDebug() << "End construct cell name =" << cellName_;
//	mDebug() << "contactSurfacesMap=====";
//	mDebug() << contactSurfacesMap_;

}

geom::Cell::Cell(const std::string &cellName,
				 const geom::Surface::map_type &globalSurfaceMap,
				 const lg::LogicalExpression<int> &poly,
				 const std::shared_ptr<const mat::Material> &mat,
				 double dens,
				 double imp)
	:Cell(cellName, globalSurfaceMap, poly, imp)  // とりあえず共通部分はvoidセルコンストラクタで初期化
{

	material_ = mat;
	/*
	 * 与えられたdensityが負の場合 g/cm3として、正の場合 1E+24 atoms/cm3としてあつかう。
	 * 内部表現は g/cm3
	 */
	if(dens < 0) {
		density_ = -dens;
	} else if(dens > 0){
		// 密度が正で入力されている場合 ×1E+24atoms/cm3として処理
		// 1.6605390 = 1e+24/6.0221409E+23
		//mDebug() << "cellname=" << cellName << "density=" << dens << "amu=" << material_->averageAtomicMass();
		// NOTE ここで（断面積未指定などで各種データが不在の場合、amuは負の値(-1)が返ることに注意）
		// --no-xsでもawrをaverageAtomicMassを計算できるようにしないと空気より重いか判定できない
		density_ = 1.6605390*dens*material_->averageAtomicMass();
	} else {
		density_ = dens;  // dens == 0ならそのまま0
	}
}



// 第二引数で指定されたsurfaceは内部/外部判定に使わず常にtrueとする。
bool geom::Cell::isInside(const math::Point& pos) const
{
	// UndefinedCellのisInsideは常にfalse;
	if(isUndefined()) return false;
//	mDebug() << "\n############ Enter Cell::isInside cell=" <<  cellName_ << ", poly=" << polynomial_.toString()<< ", pos=" << pos;
//	mDebug() << "contactSurfaceMap=" << contactSurfacesMap_;

	/*
	 * 面、特に凸面が増えると項数が爆発する問題。
	 * 面の多いセルで極端に測度が低下するのは項数が飛躍的に増えるため。
	 * 例えばrhpは8面で構成されるが、球を5個のrhpで抜いたセルを定義した場合
	 * polynomialの項数は8^5 = 32768となってしまっている。
	 * 項数Nに対して計算量が2^Nとなっている。
	 * たとえばマクロボディの括弧を展開せず
	 * (1.1:1.2:1.3:1.4:1.5:1.6) (1.1:1.2:1.3:1.4:1.5:1.6)
	 * の括弧のまま解ければ計算量はNと大幅に削減できるし、そうすべき。
	 *
	 * 即ち括弧は展開せずにchildrenのまま保持し、childrenごとのtrue/falseの
	 * 組み合わせから多項式全体のtrue/falseを決定する。
	 */
	return lg::evaluateL(polynomial_, contactSurfacesMap_, pos);
}

std::string geom::Cell::cellName() const {return cellName_;}

/*
 * 特殊材料の材料名がまだ決まっていない。
 * ・通常セルは id=1から順、 名前=入力した値
 * ・void物質は id=VOID_ID, 名前=VOID_NAME
 * ・未定義領域物質は id=UNDEF_ID 名前= UNDEF_NAME
 *
 */
// shared_ptr<const Material>から物質名を取得して返す
std::string geom::Cell::cellMaterialName() const
{
	// material_がnullptrならundef物質
	// void物質は普通にid()とか呼び出せる
	if(material_ == nullptr) {
		return mat::Material::UNDEF_NAME;
	} else {
		return material_->name();
//		return mat::Material::idToName(material_->id());
	}

}


bool geom::Cell::isHeavierThanAir() const
{
	// density_の内部はg/ccとしている。
	// が、--no-xsのように断面積を読み込んでいない場合は平均原子質量が不明なので
	// 平均原子質量は-1が返り、入力ファイルで数密度で与えられたセルについては負の値が格納されている
	// ゆえに平均原子数密度の閾値で判断する。
	if(density_ > 0) {
		return density_ > AIR_DENSITY_CRITERIA;
	} else {
		// 数密度は -1*1.6605390*densとなっているので割り戻す
		// 1.6605390 = 1e+24/6e+23 なので
		return -density_/1.6605390 > AIR_NUMBER_DENSITY_CRITERIA;
	}
    //return this->density_ > AIR_DENSITY_CRITERIA;
}

bool geom::Cell::isVoid() const
{
    return material_ == mat::Material::void_material_ptr();
}



// importance=-1のセルは?→importance -1 density 0
// undefinedcellは？     →importance +1 density 0
double geom::Cell::macroTotalXs(phys::ParticleType ptype, double energy) const
{
	return material_->macroTotalXs(ptype, density_, energy);
}

std::string geom::Cell::polynomialString() const
{
	std::stringstream ss;
	ss << polynomial_.toString(contactSurfacesMap_.nameIndexMap());
	return ss.str();
}

std::string geom::Cell::toString() const
{
	std::stringstream ss;
	ss << "Name = " << this->cellName();
	ss << " " << this->polynomialString();
	//ss   << ", contacts = " << contactSurfacesMap_.frontSurfaceNames();
	//ss << ", " << polynomial_.toString(contactSurfacesMap_.nameIndexMap());
	return ss.str();

//	ss << "Name = " << this->cellName() << ", contacts = ";
//	for(auto itr = contactSurfacesMap_.begin(); itr != contactSurfacesMap_.end(); itr ++) {
//		if(itr != contactSurfacesMap_.begin()) ss << ", ";
//		ss << itr->first;
//	}
//	ss << ", ";
//	ss << "Polynomial{" << polynominal_ << "}";
//	return ss.str();
}

//! TODO 最終的なセルカード入力文字列に変換する。
std::string geom::Cell::toFinalInputString() const
{
	std::string text = cellName_ + " ";
	// NOTE VOID あるいは平均質量数が負(--no-xsオプションのときなど)はvoid扱いにしてmaterial名は0とする。
	if(material_->name() == mat::Material::VOID_NAME || material_->averageAtomicMass() < 0) {
		text += "0 ";
	} else {
		text += material_->name() + " -" + std::to_string(density_) + " ";
	}
	text += polynomial_.toString(contactSurfacesMap_.nameIndexMap());
	return  text;
}

void geom::Cell::setInitBB(const BoundingBox &bb)
{
	initialBB_ = std::make_shared<BoundingBox>(bb);
}



//std::pair<std::shared_ptr<const geom::Surface>, math::Point>
std::pair<const geom::Surface*, math::Point>  // ナマポ版
	geom::Cell::getNextIntersection(const math::Point& point, const math::Vector<3> &direction) const
{
	auto tmp(getNextIntersections(point, direction));
	if(tmp.first.empty()) {
		return std::make_pair(nullptr, tmp.second);
	} else {
		return std::make_pair(tmp.first.front(), tmp.second);
	}
}






class IntersectionCompare
{
public:
	bool operator()(const std::tuple<int, math::Point, double> &s1, const std::tuple<int, math::Point, double> &s2) const
	{
		return std::get<2>(s1) < std::get<2>(s2);
	}
};
IntersectionCompare icomp;




#define PREC(ARG) std::setw(ARG) << std::setprecision(ARG)
// pointからdirection方向に交差する Surfaceと交点のペアを返す。
// 同等の面(平面は割とよく被る)が複数あり、距離が同じ交点が複数ある場合があるのでpair.firstはvectorとする
//std::pair<std::vector<std::shared_ptr<const geom::Surface> >, math::Point>
std::pair<std::vector<const geom::Surface*>, math::Point>  // ナマポ版
geom::Cell::getNextIntersections(const math::Point& point, const math::Vector<3> &direction) const
{

	// 交点を保持しておくコンテナ。メンバ変数にするとMT時に意図しない動作になる。thread_localにすると早くできるか
	//std::vector<std::pair<int, math::Point>> nextIntersectionVector;
	std::vector<std::tuple<int, math::Point, double>> nextIntersectionVector;  // 交差surfaceID, 交点, 交点への距離タプルをvectorで保存
	nextIntersectionVector.reserve(contactSurfacesMap_.frontSurfaces().size());

//	mDebug() << "\n### Entered to Cell::getNextIntersectionS, CurrentCell=" << cellName() << ", point=" << point <<  ", dir=" << direction;
//	mDebug() << " セルに隣接する面はconcatcSurfs=\n" << contactSurfacesMap_;

	// surface  交点を求めるときは表面のみを対象とする。
	for(auto itr = contactSurfacesMap_.frontSurfaces().begin(); itr != contactSurfacesMap_.frontSurfaces().end(); ++itr) {
		math::Point isect = (itr->second)->getIntersection(point, direction);
		if(isect.isValid()) {
            nextIntersectionVector.emplace_back((itr->second)->getID(), isect, math::distance(point, isect));
		}
	}

	if(nextIntersectionVector.empty()) return std::make_pair(std::vector<const geom::Surface*>(), math::Point::INVALID_VECTOR());

	// nextIntersectionVectorはstd::vectorなので交点計算中に距離に応じてinsertするより計算後にsortした方が速い、と思われる。
	std::sort(nextIntersectionVector.begin(), nextIntersectionVector.end(), icomp);

	// もし最近接点の距離がmath::Point::delta()の以内なら自分が今居る点を交点として認識しているので削除
	if(std::get<2>(nextIntersectionVector.front()) <= math::Point::delta()*1.1) {
		nextIntersectionVector.erase(nextIntersectionVector.begin());
    }
	// TODO ここで直線上の交点が沢山みつかるのだから一番先まで進めた方が効率的なのではないか。
//	mDebug() << "現在点=" << point << "からの次の交点候補は";
//	for(auto &info: nextIntersectionVector) {
//		mDebug() << "surfID=" << std::get<0>(info) << "交点=" << std::get<1>(info) << "距離=" << std::get<2>(info);
//	}

	/*
	 * 1．最初に全ての接触面との"前方の"交点を求める
	 * 2．それら交点との距離を求める
	 * 3．距離最小のものを選ぶ
	 *
	 * 前方の距離最小の交点を求めれば、その交点の手前がセル内かどうかはチェックする必要がない。
	 * なぜなら最近接交点の手前がセル内部なのは自明であり、そうでなければ交点ではなくセル定義に問題があるので
	 * ここではなく、コンストラクタがおかしいということになり、対処はそこで行う。
	 *
	 * 未定義セルの場合は最近接点から復帰できればいいので特殊な対応は不要。
	 *
	 * 問題は同等の面があった場合同じ距離に複数の交点が候補となること。
	 * 先頭からチェックする
	 *
	 */


	// 粒子の進行方向に交点はセル外にしかない…などというケースはあるか？いやない。
	// 問題は誤差の範囲内に複数の交点がある場合。
	// この時は内外判定は行わず、複数のsurfaceをまとめたvectorと代表的な交点のペアを返す。

	// nextIntesrectionはmap<string, Point> contactSurfacemapは<string, sharedptr<Surface>>
	math::Point nearestPoint = std::get<1>(nextIntersectionVector.front());
	double nearestDistance = std::get<2>(nextIntersectionVector.front());
	auto surfaceIndex = std::get<0>(nextIntersectionVector.front());
	std::vector<const Surface*> nearestSurfaces;
	nearestSurfaces.emplace_back(contactSurfacesMap_.at(surfaceIndex).get());

	for(size_t i = 1; i < nextIntersectionVector.size(); ++i) {
		// 交点の距離が現在の今のnearestDistance+0.5deltaの範囲ならsurfaceを追加する。
		if(std::get<2>(nextIntersectionVector.at(i)) < nearestDistance + 0.5*math::Point::delta()) {
			nearestSurfaces.emplace_back(contactSurfacesMap_.at(std::get<0>(nextIntersectionVector.at(i))).get());
		} else {
			// そうでないならbreakする。(nextIntersectionVectorはソート済みなので)
			break;
		}
	}

	//mDebug() << "選択した交点は=" << nearestPoint << "交差面は=" << nearestSurfaces.front()->name();
	return std::make_pair(nearestSurfaces, nearestPoint);
}

std::pair<std::shared_ptr<const geom::Surface>, math::Point>
geom::Cell::getFarestIntersection(const math::Point &point, const math::Vector<3> &direction) const
{
	// 交点を保持しておくコンテナ。メンバ変数にするとMT時に意図しない動作になる。thread_localにすると早くできるか
	std::vector<std::pair<int, math::Point>> nextIntersectionMap;
	nextIntersectionMap.reserve(contactSurfacesMap_.frontSurfaces().size());
	/*
	 * 1．最初に全ての表面との交点を求める
	 * 2．全ての交点との距離を求める
	 * 3．距離最大でinvalidでないのものを選ぶ
	 */

	// surface  交点を求めるときは表面のみを対象とする。
	for(auto itr = contactSurfacesMap_.frontSurfaces().begin(); itr != contactSurfacesMap_.frontSurfaces().end(); ++itr) {
		// itr->secondはshared_ptr<Surface>を指している
		nextIntersectionMap.emplace_back((itr->second)->getID(), (itr->second)->getIntersection(point, direction));
	}

	// nextIntesrectionはmap<string, Point> contactSurfacemapは<string, sharedptr<Surface>>
	math::Point farestPoint = math::Point::INVALID_VECTOR();
	double farestDistance = 0;
	std::shared_ptr<const Surface> farestSurface;

	for(auto itr = nextIntersectionMap.begin(); itr != nextIntersectionMap.end(); itr++) {
		// firstはsurface名、secondは交差点のインスタンス
		math::Point intersection = itr->second;
		if(intersection.isValid()) {
			// 交点(-オフセット)が現在セル内では無い場合は交点候補からは外す。
			if(isUndefined() || isInside(intersection - math::Point::delta()*direction)){
				// dist は現在の交点候補(intersection)と基準点(point)との距離。
				double dist =  math::distance(point, intersection);
				// distが現在のこれまでの参照点-交点最短距離よりdelta以上短ければ更新する。
				if(farestDistance < dist) {
					farestDistance = dist;
					farestPoint = intersection;
					farestSurface = contactSurfacesMap_.at(itr->first);
				}
			}
		}
	}
	return std::make_pair(farestSurface, farestPoint);
}

const std::string &geom::Cell::undefindeCellName()
{
	static std::string undefCellName = std::string(UNDEF_CELL_NAME);
	return undefCellName;
}


// static メンバ
void geom::Cell::initUndefinedCell(const geom::Surface::map_type &surfMap)
{
	/*
	 * 未定義セルの
	 * ・論理多項式は空
	 * ・shared_ptr<Material>データはnullptr
	 * ・contactSurfaceは全surface
	 * ※通常Surfaceの隣接セルにはundefinedCellは含まれないことに注意
	 *   undefCellからsurfaceへ移動することはあるがその逆は無いということ。
	 *
	 */
	// とりあえず空の多項式と空のsurfaceMapで構築する。
	std::shared_ptr<Cell> tmpUndefCell
			= std::make_shared<Cell>(std::string(UNDEF_CELL_NAME),
									 geom::Surface::map_type(),
									 lg::LogicalExpression<int>(), 1.0);



	for(auto it = surfMap.frontSurfaces().begin(); it != surfMap.frontSurfaces().end(); ++it) {
		tmpUndefCell->contactSurfacesMap_.registerSurface((it->second)->getID(), it->second);
	}
	for(auto it = surfMap.backSurfaces().begin(); it != surfMap.backSurfaces().end(); ++it) {
		tmpUndefCell->contactSurfacesMap_.registerSurface((it->second)->getID(), it->second);
	}
	tmpUndefCell->material_ = nullptr;

	undefinedCell_ = tmpUndefCell;


}

const std::shared_ptr<const geom::Cell> &geom::Cell::UNDEFINED_CELL_PTR()
{
	if(undefinedCell_ == nullptr) {
		std::cerr << "ProgramError: Undefined cell is used but not initialized." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	// undefinedCell_が std::shared_ptr<Cell>の場合 std::shared_ptr<const geom::Cell>の一時コピーが返される。
	// この時の返り値の参照を返すと不定になる。
	return undefinedCell_;
}


// strict==trueなら重複定義セルを検出し、その場合未定義セルと返す。
const std::shared_ptr<const geom::Cell> &geom::Cell::guessCell(const geom::Cell::const_map_type &cellList,
														const math::Point &pos, bool strict, bool enableCache)
{

	// 正解セルをキャッシュすれば概ね定数時間、最悪セルス比例くらいになるはず。
	// よってlatticeなどで細かいセルが増えた場合、断面描画時間が飛躍的に延びてしまう。 cell初期位置推定の
	// 高度化が必要。前回の結果をキャッシュするとか。BSPなどでlog(N)に緩和するなど。

//	static const std::shared_ptr<const Cell> *cache = nullptr;
	thread_local const std::shared_ptr<const Cell> *cache = nullptr;

	if(strict) {
        // strictモードでは全セルに対してisInsideを実行して内外判定し、
        // 複数セルの内側に該当する場合未定義領域セルを返す。断面描画ときの重複領域検出にはこちらを使う
		std::vector<const std::shared_ptr<const geom::Cell>*> cellVec;
		for(auto &cellPair: cellList) {
			if(cellPair.second->isInside(pos)) {
					if(!cellVec.empty()) return geom::Cell::UNDEFINED_CELL_PTR();
					cellVec.emplace_back(&(cellPair.second));
			}
		}
		return cellVec.empty() ? geom::Cell::UNDEFINED_CELL_PTR(): *(cellVec.front());
	}

	//キャッシュ
	if(enableCache && cache != nullptr) {
		if((*cache)->isInside(pos)) {
			//mDebug() << "キャッシュヒット！！！！！！！！！！ cell=" << (*cache)->cellName();
			return *cache;
		}
	}

	// strictで無い場合は最初に該当したセルを返す。
	for(auto &cellPair: cellList) {
		if(cellPair.second->isInside(pos)) {
			if(enableCache && cache != nullptr) {
				mDebug() << "キャッシュ更新 旧セル=" << (*cache)->cellName();
				mDebug() << "新セル=" << cellPair.second->cellName();
				cache = &(cellPair.second);
//				mDebug() << "キャッシュ更新 セル=" << (*cache)->cellName();
			}
			return cellPair.second;
		}
	}

    return geom::Cell::UNDEFINED_CELL_PTR();
}

std::vector<std::string> geom::Cell::getHierarchialCellNames(const std::string &cellName)
    // 親、祖父、曽祖父…のセル名を返す。
    // セル名"2<12<15"  をvector {"2<12<15", "12<15", "15"}に分解する。
    // 返り値のfront()は末端セルなのでemptyにはならない。
{
    std::vector<std::string> retvec;
    std::string::size_type pos = cellName.find_first_of('<');
    // 先頭が"<"の場合は不適当なので例外発生にする
    if(pos == 0) {
        throw std::runtime_error(std::string("Invalid cell name, started with \"<\". name = ") + cellName);
    } /*else if(pos == std::string::npos) {
        return std::vector<std::string>{cellName};
    }*/

    retvec.emplace_back(cellName);
    while(pos != std::string::npos) {
        retvec.emplace_back(cellName.substr(pos+1));
        pos = cellName.find_first_of('<', pos+1);
    }
    //    mDebug() << "in geteachcellname cell==" << cellName << "Elements===" << retvec;
    assert(!retvec.empty());
    return retvec;
}


bool geom::Cell::cellNameLess(const std::string &str1, const std::string &str2)
{
	// 階層が浅い方を前に、階層が同じなら普通の文字列比較で判定。数値文字列なら数値として比較。
	auto c1 = std::count(str1.begin(), str1.end(), '<');
	auto c2 = std::count(str2.begin(), str2.end(), '<');
	if(c1 != c2) {
		return c1 < c2;
	} else {
		//strが数値化可能であれば数値として比較する。
		if(utils::isArithmetic(str1) && utils::isArithmetic(str2)) {
			return utils::stringTo<double>(str1) < utils::stringTo<double>(str2);
		} else {
			return str1 < str2;
		}
	}
}




// 一定時間経過したらflag_をtrueにするクラス。
namespace {
class ThreadTimer {
public:
    ThreadTimer(std::atomic_bool* pf, std::atomic_bool *cf):cancelFlag_(cf), timeoutFlag_(pf){}
	void operator()(size_t muSec)
	{
		size_t dt = muSec/100;
		size_t elapsed = 0;
		while(elapsed < muSec) {
			std::this_thread::sleep_for(std::chrono::microseconds(dt));
			elapsed += dt;
			if(cancelFlag_->load()) break;
		}

		// whileを抜けてきてcancelFlagがfalseならタイムアウト判定なのでtimeoutFlagをtrueにする。
		if(!cancelFlag_->load()) {
			timeoutFlag_->store(true);
		}
	}
private:
	std::atomic_bool *cancelFlag_;
	std::atomic_bool *timeoutFlag_;
};
}

#include "core/utils/time_utils.hpp"

geom::BoundingBox geom::Cell::boundingBox(size_t timeoutMsec) const
{
//mDebug() << "\nEnter Cell::createBoundingBox(), cell =" << toString();

    /* BB計算の流れを決める。BBには以下の三種類がある
	 * １．セルカード初期BB(latticeのみ)
	 * ２．簡易BB getRoughBB()
	 * ３．詳細BB getDetailedBB()
	 *
	 * とりあえず初期BBは問答無用に信頼できるので、
	 * 初期BBがある場合はそのまま返すか、計算コストの低い簡易BBとANDを取って返す。
	 *
	 * 詳細BB計算は目茶苦茶時間掛かるので避けたいが、BBが無駄に大きいポリゴン生成の時間が伸びる。
	 * そこで、大体詳細BB計算が失敗しがちで、かつ描画されることも少ない
	 * void/空気セル(=デフォルト非描画セル)は簡易BB→詳細BB計算の順で実施し、
	 *
	 * 非voidセルは詳細BB → 簡易BBの順に計算する。
	 */


	// 初期BBは計算値より確度が高いと考えられるので、(計算コストの低い)簡易BBとのANDをとってreturnする。
	if(initialBB_) 	{
//		mDebug() << "initBB===" << initialBB_->toInputString() << "rough===" << getRoughBB().toInputString();
		return BoundingBox::AND(getRoughBB(), *initialBB_.get());
	}

	// ###########3 以下 initialBBがある場合以外

//	// 方針1 先に簡易BBを求めて、求まらなかった場合詳細BB計算をする。
//	// Laticceでは要素がfilledセル外にある場合にはbbがemptyになる。
//	//mDebug() << "\nStart creating bounding box for cell===" << this->cellName_ << "poly===" << polynomial_.toString();
//	BoundingBox bb = getRoughBB();

//	// bb.isUniversal() で
//	// strict=trueとすると 厳密計算を実施しがちなのでBBは正確だが時間が掛かる。
//	// strict=falseとすると本来厳密には求まるはずなのに計算時間を嫌ってBBを部分的にしか計算しない
//	// bbは大雑把とはいえ必ずBB内にcellを含んでいるはずであり、bb.empty()というのはセル定義のせいで
//	// 正しく体積ゼロになっていると言える。故にbb.empty()の場合は詳細BB計算は行わない。
//	if(bb.isUniversal(false)) {
//		auto tmpBB = getDetailedBB();
//		if(!tmpBB.empty()) bb = BoundingBox::AND(tmpBB, bb);
//	}


	BoundingBox bb = BoundingBox::universalBox();
	if(importance_ > 0 && isHeavierThanAir()) {
		// 通常の(default描画対象)セルは詳細計算→簡易計算の順にBBを求める。
		utils::SimpleTimer timer;
		timer.start();
//        mDebug() <<"Cell" << this->cellName_ << "Begin get detailed BB!!!!!!!!!!!!!!!1";
		bb = getDetailedBB(timeoutMsec);
		timer.stop();
//        mDebug() << "GetDetailedBB end in " << timer.msec() << "ms. Detailed BB===" << bb.toInputString();

        //assert(!"DEBUG END");

		if(bb.isUniversal(false)) {

			auto medBB = getMediumBB(true, timeoutMsec);
//            mDebug() << "medium BB calculation (accept multipiece) result ===" << medBB.toInputString();

			//assert(!"DEBUG END デバッグ中に付きdetailed計算を省略しているのであとで直すこと");

			bb = BoundingBox::AND(bb, medBB);
			if(bb.isUniversal(false)) {
				medBB = getMediumBB(false, timeoutMsec);
//                mDebug() << "medium BB calculation (ignore multipiece) result ===" << medBB.toInputString();
				bb = BoundingBox::AND(bb, medBB);
				if(bb.isUniversal(false)) {
					bb = BoundingBox::AND(bb, getRoughBB());
//                    mDebug() << "rough BB calculation result ===" << bb.toInputString();
				}
			}
		}
	} else {
		// 空気セルは描画頻度が低く、かつ詳細BB計算の失敗率が高いので、
		// 先に簡易BBを計算して失敗したら詳細BBとのANDを取る。
		bb = getRoughBB();
		//mDebug() << "RoughBB===" << bb.toInputString();
		if(bb.isUniversal(false)) {
			auto med1BB = getMediumBB(false, timeoutMsec);
			bb = BoundingBox::AND(bb, med1BB);
			//mDebug() << "MediumBB1 ===" << med1BB.toInputString();
			if(bb.isUniversal(false)) {
				med1BB = getMediumBB(true, timeoutMsec);
				bb = BoundingBox::AND(bb, med1BB);
				//mDebug() << "MediumBB2 ===" << getMediumBB(true, timeoutMsec).toInputString();
				auto tmpBB = getDetailedBB(timeoutMsec);
				// deteiledBBが失敗したらuniversalが返るからemptyチェックは不要のはずだが一応
				if(!tmpBB.empty()) bb = BoundingBox::AND(tmpBB, bb);
                if(bb.isUniversal(false)) {
                    mWarning() << "Bounding box of cell =" << this->cellName()
                               << "was completely failed.";
                }
			}
		}
	}


	//mDebug() << "Cell===" << cellName_ << "'s  BB=" << bb.toInputString();
	return bb;
}



geom::BoundingBox geom::Cell::getRoughBB() const
{
    return geom::bb::createBoundingBox(polynomial_, contactSurfacesMap_);
}


// 論理式のAND演算の部分だけでまずBBを作って、あとはそれらのBB間ORでBBを計算する
// 論理式内に括弧で括った論理式があればその内部は再帰的にBBを計算する。
// 引数がtrueならセルがマルチピースとなるばあい、個別の部分を考慮して詳細計算する。
// falseならマルチピース部分は全て無視する。
geom::BoundingBox geom::Cell::getMediumBB(bool acceptMultiPiece, size_t timeoutMsec) const
{
	geom::BoundingBox bb;
	std::atomic_bool timeoutFlag(false), cancelFlag(false);
	ThreadTimer threadTimer(&timeoutFlag, &cancelFlag);
	std::thread th(threadTimer, timeoutMsec*1000);
	try {
        bb = bb::createBoundingBox2(polynomial_, contactSurfacesMap_, &timeoutFlag, acceptMultiPiece);
		// BB計算が終わったらタイマースレッドのsleepを終了させる。
		cancelFlag.store(true);
		th.join();
	} catch (std::exception &e) {
		mWarning() << "Calculating bounding box(medium, multi="
				   << acceptMultiPiece << ") for cell=" << cellName_ << "failed," << e.what();
		cancelFlag.store(true);
		th.join();
		bb = BoundingBox::universalBox();
	}
	return bb;
}


static const size_t MAX_BOUNDING_PLANE_VECTORS = 1000;

geom::BoundingBox geom::Cell::getDetailedBB(size_t timeoutMsec) const
{
//    mDebug() << "Detailed BB calculation for cell=== " << cellName_ << " started, timeout===" << timeoutMsec;

	// boundingSurfacesのvector要素数は簡単に爆発するのでtimeoutFlagを設定し、
	// flagがtrueになったら例外を投げてもらって計算を諦めるようにしている。
	std::atomic_bool timeoutFlag(false), cancelFlag(false);
	ThreadTimer threadTimer(&timeoutFlag, &cancelFlag);
	std::thread th(threadTimer, timeoutMsec*1000);
    try{
		std::vector<std::vector<Plane>> boundingPlaneVectors
             = bb::boundingSurfaces(polynomial_, &timeoutFlag, contactSurfacesMap_);
        // 注意！シングルスレッド実行(--threads=0のとき)はタイムアウトは機能しないので要注意。
//        mDebug() << "creating detailed BB from planeVec size========" << boundingPlaneVectors.size();
        if(boundingPlaneVectors.size() > MAX_BOUNDING_PLANE_VECTORS) {
            throw std::runtime_error("Too much bounding planes");
        }
//        mDebug() << "planevec===";
//        for(size_t i = 0; i < boundingPlaneVectors.size(); ++i) {
//            mDebug() << "vec i=" << i;
//            for(auto pl: boundingPlaneVectors.at(i)) mDebug() << "    pl=" << pl.toString();
//        }

		auto tmpBB = BoundingBox::fromPlanes(&timeoutFlag, boundingPlaneVectors);
        //mDebug() << "Detailed BB calculation for " << cellName_ << " result=" << tmpBB.toInputString();



		// ここでタイマースレッドのsleepを終了させる。
		cancelFlag.store(true);
		th.join();
		return tmpBB;

    } catch (std::exception &e) {
        mWarning() << "Calculating detailed bounding box for cell=" << cellName_
                   << "failed," << e.what();
        cancelFlag.store(true);
        th.join();
        return BoundingBox::universalBox();
    }
}



#ifdef ENABLE_GUI
vtkSmartPointer<vtkImplicitFunction> geom::Cell::createImplicitFunction() const
{
	//mDebug() << "Creating implicit function for cell=" << toString();
    return geom::bb::createImplicitFunction(polynomial_ , contactSurfacesMap_);

}
#endif




