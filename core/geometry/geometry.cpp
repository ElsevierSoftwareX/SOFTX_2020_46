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
#include "geometry.hpp"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#ifdef ENABLE_GUI
#include <QApplication>
#include <QProgressDialog>
#endif

#include "core/geometry/cellcreator.hpp"
#include "core/geometry/cell_utils.hpp"
#include "core/geometry/surfacecreator.hpp"
#include "core/geometry/surf_utils.hpp"
#include "core/geometry/macro/arb.hpp"
#include "core/geometry/macro/box.hpp"
#include "core/geometry/macro/ell.hpp"
#include "core/geometry/macro/macro.hpp"
#include "core/geometry/macro/qua.hpp"
#include "core/geometry/macro/rcc.hpp"
#include "core/geometry/macro/rec.hpp"
#include "core/geometry/macro/rhp.hpp"
#include "core/geometry/macro/rpp.hpp"
#include "core/geometry/macro/sph.hpp"
#include "core/geometry/macro/tor.hpp"
#include "core/geometry/macro/trc.hpp"
#include "core/geometry/macro/wed.hpp"
#include "core/geometry/macro/xyz.hpp"
#include "core/geometry/surface/surfacemap.hpp"
#include "core/geometry/tracingworker.hpp"
#include "core/utils/progress_utils.hpp"
#include "core/image/bitmapimage.hpp"
#include "core/image/matnamecolor.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/inputdata.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/io/input/phits/phitsinputsection.hpp"
#include "core/io/input/phits/transformsection.hpp"
#include "core/material/materials.hpp"
#include "core/utils/utils.hpp"
#include "core/utils/system_utils.hpp"
#include "core/utils/time_utils.hpp"
#include "core/physics/particle/tracingparticle.hpp"
#include "component/libacexs/libsrc/xsdir.hpp"

// NOTE cellはポリモーフィズム使わないからポインタ、スマートポインタを使う理由がない。(将来特殊セルを作るかもpend.)

namespace {

//void showUndefCell()
//{
//	mDebug() << "undefセルの状態";
//	auto undefCell = geom::Cell::UNDEFINED_CELL_PTR();
//	mDebug() << "name=" << undefCell->cellName();
//	mDebug() << "contacting surf =";
//	int cmax = 0;
//	for(auto &s: undefCell->contactSurfacesMap().frontSurfaces()) {
//		if(s.second) {
//			++cmax;
//			mDebug() << s.second->name();
//		}
//		if(cmax > 10) {
//			mDebug() << "............+others";
//			break;
//		}
//	}
//}

}  // end anonymous namespace


// ptypesが空ならば表示のみとする。この場合は断面積は読み込まない
std::shared_ptr<geom::Geometry> geom::Geometry::createGeometry(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
															   const std::list<inp::DataLine> &cellCards,
															   const std::list<inp::DataLine> &surfaceCards,
															   const std::shared_ptr<const mat::Materials> &materials,
															   const conf::Config &config)
{
	namespace ip = inp::phits;
    //if(config.mode != McMode::PHITS) throw std::invalid_argument("Sorry only phits is supported currently.");

	/*
	 *  ファイルからinputDataに読み込まれた時点で明白に処理すべきメタカードはinclude処理とset:の定数処理のみ。
	 * ・infl:カードはファイル構造=section構造に影響するので最初に処理する必要がある。
	 * ・set:カードは同名定数の使い回しや定数定義に定数を使うと、セクション感を跨いでファイル中での出現順序に依存する
	 *      ので処理できるところはここしかない
	 *
	 * メタカードはセクションによって処理されたりされなかったりするのでここではできない。
	 * ・cell、surfaceセクションでは#でコメントアウトされない、他のセクションではコメントアウトされる
	 * ・source・tallyセクションでのメッシュサブセクションでは前置行継続(行頭5空白)は処理されない。
	 *   しかし後置行継続(行末\)は処理される。
	 * が、適当に拡張すれば#以外は全てセクション非依存として処理できる。
	 * よって汚くなるがこの時点で簡易的にセクション判定しながら#も処理する。
	 *
	 */

	// #################### ここからphits入力開始


	// ジオメトリの構築には以下の要素が必要 ・transformMap ・surfaceInputData ・cellInputData ・material
	return std::make_shared<Geometry>(trMap,
									  surfaceCards,
									  cellCards,
									  materials,
									  config.verbose, config.warnPhitsIncompatible, config.numThread);

}


geom::Geometry::Geometry(const std::unordered_map<size_t, math::Matrix<4> > &trMap,
						 std::list<inp::DataLine> surfaceInput,
						 std::list<inp::DataLine> cellInput,
						 const std::shared_ptr<const mat::Materials> &materials,
						 bool verbose, bool warnPhitsCompat, int numThread)
	:trMap_(trMap)
{

	if(surfaceInput.empty()) {
		throw std::runtime_error("Invalid input file. Surface section is empty.");
	} else if (cellInput.empty()) {
		throw std::runtime_error("Invalid input file. Cell section is empty.");
	} else if (materials->empty()) {
		mWarning() << "Invalid input file. Material section is empty.";
	}


	/*
	 * ここからgeometryを作る。
	 * 0. map<string, shared<const Material> を作る。
	 * 1. map<string, shared<Surface>> surfaces_ を作る
	 * 2．map<string, shared<const Cell>> cells_ を作る
	 * 3．Surfaceの裏面を作る。
	 * 4．cell-surfaceの連結を作る
	 * 5．不要なsurfaceを削除する
	 * 6．UndefinedCellを初期化
	 *
	 */

	//mDebug() << "ptypes is empty?===" << ptypes.empty() << ", xsdir empty?===" << xsdir->empty();

	auto materialMap = materials->materialMapByName();
//	for(auto it = materialMap.begin(); it != materialMap.end(); ++it) {
//		mDebug() << "matID===" << it->first << "matName=" << it->second->name() << "buildup=" << it->second->bfName();
//	}


//    mDebug() << "入力Datalineリスト";
//    for(auto &dl: cellInput) {
//        mDebug() << "dl=====" << dl.data;
//    }

	// ########## surfaceデータ作成
	// surfaceのマクロボディ展開
	Geometry::expandMacroBody(trMap, &surfaceInput, &cellInput);
	if(verbose) {
		std::ofstream sof("surface.i5");
		sof << utils::toString(surfaceInput);
		std::ofstream cof("cell.i5");
		cof << utils::toString(cellInput);
	}

//    mDebug() << "マクロボディ展開後のDatalineリスト";
//    for(auto &dl: cellInput) {
//        mDebug() << "dl=====" << dl.data;
//    }



	geom::SurfaceCreator surfaceCreator(surfaceInput, trMap, warnPhitsCompat);		// ここで裏面付きSurfaceMapができる。


    geom::CellCreator cellCreator(cellInput, &surfaceCreator, materialMap, warnPhitsCompat, numThread, verbose);	// CellMapと surf-Cell連結ができる。


	// 不要な面の削除。ここでは削除時に警告しない。なぜならTRされる元となるsurfaceは使われていないが必要なものだから。
	surfaceCreator.removeUnusedSurfaces(false);
	geom::Cell::initUndefinedCell(surfaceCreator.map()); // 未定義領域の初期化

	// これでやっとcellCreator.cells()でセルが作成できる。

	// 粒子追跡時にデータ競合が起こらないことを保証するためにcellのスマポをshared<const Cell＞にしている
	//  *itは std::unordered_map<std::string, std::shared_ptr<Cell>>
	auto newCells = cellCreator.cells();
	for(auto it = newCells.begin(); it != newCells.end(); ++it) {
		cells_[it->first] = it->second;  // ここで shared<Cell>からshared<const Cell>への代入を行っている。
	}

	utils::updateCellSurfaceConnection(cells_);

	// 実際の粒子追跡ははcell → Cell::contactSurface<shared<Surf>> → surface → Surf::ContactSurfaceMap<shared<const Cell>
	// のように辿っていくのでsufacesMapは別途保持する必要はない。
//	for(auto &cellPair: cells_) {
//		mDebug() << "cells====" <<  cellPair.second->toString();
//	}
//	for(size_t i = 1; i < surfaceCreator.map().size()*0.5; ++i) {
//		if(surfaceCreator.map()[i]) {
//			mDebug() << "i=" << i << ", " << surfaceCreator.map()[i]->toString();
//		}
//	}





	// 予約されているセル、材料名、色を登録していく
	// 引数は セル名、物質名、 色
	this->setReservedPalette();
	this->setDefaultPalette();

    this->surfaceIndexNameMap_ = surfaceCreator.map().nameIndexMap();
}


geom::Geometry::Geometry(const geom::SurfaceMap &surfMap,
                         const std::unordered_map<std::string, std::shared_ptr<const geom::Cell> > &cellMap)
	:cells_(cellMap)
{
	auto tmpSurfMap = surfMap;
//	utils::addReverseSurfaces(&tmpSurfMap);
	utils::updateCellSurfaceConnection(cells_);
	utils::removeUnusedSurfaces(&tmpSurfMap);

	geom::Cell::initUndefinedCell(tmpSurfMap);

	// 予約されているセル、材料名、色を登録していく
	this->setReservedPalette();
	this->setDefaultPalette();
}


/*
 * 未定義領域と多重定義領域を区別して2D描画するかどうか？
 * →多重定義領域の検出はどうせ不完全なのだから未定義領域扱いにする。
 */
// origin から横軸をdir1, 縦軸をdir2とした解像度h/vResoの断面画像(filename)を出力する。
img::BitmapImage geom::Geometry::getSectionalImage(const math::Vector<3> &origin,
                                          const math::Vector<3> &hdir,
                                          const math::Vector<3> &vdir,
										  size_t hReso, size_t vReso,
                                          int numThread, bool verbose, bool quiet) const
{
	namespace stc = std::chrono;
	using Bitmap = img::BitmapImage;
	double dh = hdir.abs()/hReso, dv = vdir.abs()/vReso;
    math::Vector<3> hUnitVec = hdir.normalized();
    math::Vector<3> vUnitVec = vdir.normalized();
    utils::SimpleTimer timer;
    timer.start();
    if(quiet) verbose = false;

	// 水平方向走査情報
	OperationInfo hinfo = TracingWorker::info();
	hinfo.numTargets = hReso;
	hinfo.numThreads = utils::guessNumThreads(numThread);
    hinfo.waitingOperationText = hinfo.waitingOperationText + "(horizontal)";
    if(quiet) hinfo.waitingOperationText = "";

	std::vector<img::TracingRayData> h1Rays = ProceedOperation<TracingWorker>(hinfo, origin, hUnitVec, hdir.abs(), vUnitVec, dv, false, cells_);
	if(h1Rays.empty()) {
		mWarning() << "Section tracing was canceled.";
		return img::BitmapImage();
	}
	auto vinfo = TracingWorker::info();
	vinfo.numThreads = utils::guessNumThreads(numThread);
	vinfo.numTargets = vReso;
	vinfo.waitingOperationText = vinfo.waitingOperationText + "(vertical)";
    if(quiet) vinfo.waitingOperationText = "";
	std::vector<img::TracingRayData> v1Rays = ProceedOperation<TracingWorker>(vinfo, origin, vUnitVec, vdir.abs(), hUnitVec, dh, false, cells_);
	if(v1Rays.empty()) {
		mWarning() << "Section tracing was canceled.";
		return img::BitmapImage();
	}

    timer.stop();
    mDebug() << "Tracing done. time =" <<  timer.msec() << " msec.";


	// ########## ここから画像作成開始。

	// 色パレットの作成
	// セル名とそのセルのマテリアル名のペアのマップがbitmapの着色のために必要
	//std::vector<std::pair<std::string, std::string>> cellMatNameList;

	/*
	 * imageの構築には、h/v画素数、h/v物理サイズ、h/v走査データ、cell-material構成データが必要
	 */
    if(verbose) {
		mDebug() << "Output directional xpm image for vebose outout. resolutions =" << hReso << vReso;
        Bitmap(img::DIR::H, hReso, vReso, hdir.abs(), vdir.abs(),
                          h1Rays, palette_).exportToXpmFile("ploth.xpm");
        Bitmap(img::DIR::V, hReso, vReso, hdir.abs(), vdir.abs(),
                          v1Rays, palette_).exportToXpmFile("plotv.xpm");
//		Bitmap::flipHorizontally(Bitmap(img::DIR::H, hReso, vReso, hdir.abs(), vdir.abs(),
//															h2Rays, cellMatNameList)).exportToXpm("hr.xpm");
//		Bitmap::flipVertically(Bitmap(img::DIR::V, hReso, vReso, hdir.abs(), vdir.abs(),
//														  v2Rays, cellMatNameList)).exportToXpm("vr.xpm");
    }

	// 逆方向走査で二重定義検出力はアップし、オーバーラップ型重複定義は検出できるようになった。
	// しかし、水平走査方式では二重定義の完全検出(完全含有型の検出)は無理。厳密にやろうとするなら画素ベース探索など
	// 走査方式は簡易と割り切ってhv二通りスキャンで済ませるというのも方法。

	// また逆方向走査は順方向走査と異なる結果が出る。
	// これはピクセル境界とセル境界が一致したときのどちらのセルに含めるかは走査方向が逆になっても、逆方向には丸めないため。
//	auto image = Bitmap::merge(
//					Bitmap::merge(
//						Bitmap(img::DIR::H, hReso, vReso, hdir.abs(), vdir.abs(), h1Rays, cellMatNameList),
//						Bitmap(img::DIR::V, hReso, vReso, hdir.abs(), vdir.abs(), v1Rays, cellMatNameList)),

//					Bitmap::merge(
//						Bitmap::flipHorizontally(Bitmap(img::DIR::H, hReso, vReso, hdir.abs(), vdir.abs(),
//																			h2Rays, cellMatNameList)),
//						Bitmap::flipVertically(Bitmap(img::DIR::V, hReso, vReso, hdir.abs(), vdir.abs(),
//																		  v2Rays, cellMatNameList))
//								));
//	image.drawEdge(image.palette().getIndexByCellName(geom::Cell::DOUBLE_NAME),
//				   image.palette().getIndexByCellName(geom::Cell::UBOUND_NAME));
//	return image;

	/*
	 * 結局の所逆方向走査は失うものが多い
	 * ・1ピクセルズレ得る
	 * ・計算時間が倍
	 * となる一方、メリットがオーバーラップ型多重定義領域の検出のみなので割に合わない。
	 * 多重定義領域検出は点描方式を採用するとして、走査方式では簡易的な断面図作成と
	 * tracklength計算の確認をする。
	 */


    Bitmap himg(img::DIR::H, hReso, vReso, hdir.abs(), vdir.abs(), h1Rays, palette_);
    Bitmap vimg(img::DIR::V, hReso, vReso, hdir.abs(), vdir.abs(), v1Rays, palette_);

    auto retimg = Bitmap::merge(himg, vimg,
                         std::vector<std::string>{geom::Cell::UBOUND_CELL_NAME, geom::Cell::BOUND_CELL_NAME},
                         geom::Cell::DOUBLE_CELL_NAME);
    return retimg;

//	return Bitmap::merge(Bitmap(img::DIR::H, hReso, vReso, hdir.abs(), vdir.abs(), h1Rays, palette_),
//						 Bitmap(img::DIR::V, hReso, vReso, hdir.abs(), vdir.abs(), v1Rays, palette_),
//						 std::vector<std::string>{geom::Cell::UBOUND_CELL_NAME, geom::Cell::BOUND_CELL_NAME},
//						 geom::Cell::DOUBLE_CELL_NAME);

}

// TODO 未定義領域があるとこのトレーシング中に例外発生になる。
const geom::Cell *geom::Geometry::getNextCell(const geom::Cell *startCell, const math::Vector<3> &dir, math::Point *pt) const
{
    //std::string cname = startCell == nullptr ? "nullptr" : startCell->cellName();
    //mDebug() << "Enter getNextCell!!! dir===" << dir << "p===" << *pt << "startCell===" << cname ;
	// weight, pos, dir, energy, startCell, セルマップ、最大長,イベントログを取るか、領域判定を厳密に行うか
	phys::TracingParticle p1(1.0, *pt, dir, 0, startCell, cells_, 1e10, false, false);
	// セル境界まで移動。セルがなければこの関数は繰り返しParticle::moveToSurfaceを呼んでsurface交差点が見つからなければruntime_errorを投げる
	try {
		p1.moveToCellBound();
	} catch (std::runtime_error &re) {
        (void) re;
		// ここに来たということは交点が見つからないということなのヌルポを返す。
		return nullptr;
	}
	p1.enterCell();
	*pt = p1.position();
	return p1.currentCell();
}




const std::string geom::Geometry::toFinalInputString() const
{
	std::string text = "c  cells\n";
	//mDebug() << "num cells=" << cells_.size();
	// unorderedだと見づらいのでorderedなsetに詰め替えする。
	std::set<std::shared_ptr<const Cell>, geom::CellLess> cellSet;
    for(const auto &cellPair: cells_) cellSet.emplace(cellPair.second);
	for(const auto &cell: cellSet) 	text += cell->toFinalInputString() + "\n";

	text += "\n";

	// Geometryクラスのメンバでmapを保持しておいても良いが、とりあえずは
	// 手っ取り早くcellから構築する．
	std::set<std::shared_ptr<const Surface>, geom::SurfaceLess> surfSet;
	for(auto &cellPair: cells_) {
		for(const auto& surfPair: cellPair.second->contactSurfacesMap().frontSurfaces()) {
			surfSet.emplace(surfPair.second);
		}
	}
	text += "c  surfaces \n";
	for(const auto& surf: surfSet) {
		text += surf->toInputString() + "\n";
	}

	return text;
}


// macrobodyの展開を実行
void geom::Geometry::expandMacroBody(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
									 std::list<inp::DataLine> *surfInputList,
									 std::list<inp::DataLine> *cellInputList)
{

	static std::unordered_map<std::string, macro::expander_type> expanderMap {
		{macro::Arb::mnemonic, &macro::Arb::expand},
		{macro::Box::mnemonic, &macro::Box::expand},
		{macro::Ell::mnemonic, &macro::Ell::expand},
		{macro::Qua::mnemonic, &macro::Qua::expand},
		{macro::Rcc::mnemonic, &macro::Rcc::expand},
		{macro::Rec::mnemonic, &macro::Rec::expand},
		{macro::Rhp::mnemonic, &macro::Rhp::expand},
		{"hex",                 &macro::Rhp::expand},  // hexはrhpと同一
		{macro::Rpp::mnemonic, &macro::Rpp::expand},
		{macro::Sph::mnemonic, &macro::Sph::expand},
		{macro::Tor::mnemonic, &macro::Tor::expand},
		{macro::Trc::mnemonic, &macro::Trc::expand},
		{macro::Wed::mnemonic, &macro::Wed::expand},
		//{macro::XAxisym::mnemonic, &macro::XAxisym::expand},
		{macro::AxSym<math::AXIS::X>::mnemonic, &macro::AxSym<math::AXIS::X>::expand},
		{macro::AxSym<math::AXIS::Y>::mnemonic, &macro::AxSym<math::AXIS::Y>::expand},
		{macro::AxSym<math::AXIS::Z>::mnemonic, &macro::AxSym<math::AXIS::Z>::expand}
	};

	static std::unordered_map<std::string, macro::replacer_type> replacerMap {
		{macro::Arb::mnemonic, &macro::Arb::replace},
		{macro::Box::mnemonic, &macro::Box::replace},
        {macro::Ell::mnemonic, &macro::Ell::replace},
		{macro::Qua::mnemonic, &macro::Qua::replace},
        {macro::Rcc::mnemonic, &macro::Rcc::replace},
        {macro::Rec::mnemonic, &macro::Rec::replace},
        {macro::Rhp::mnemonic, &macro::Rhp::replace},
        {"hex",                 &macro::Rhp::replace},
        {macro::Rpp::mnemonic, &macro::Rpp::replace},
		{macro::Sph::mnemonic, &macro::Sph::replace},
        {macro::Tor::mnemonic, &macro::Tor::replace},
		{macro::Trc::mnemonic, &macro::Trc::replace},
		{macro::Wed::mnemonic, &macro::Wed::replace},
		{macro::AxSym<math::AXIS::X>::mnemonic, &macro::AxSym<math::AXIS::X>::replace},
		{macro::AxSym<math::AXIS::Y>::mnemonic, &macro::AxSym<math::AXIS::Y>::replace},
		{macro::AxSym<math::AXIS::Z>::mnemonic, &macro::AxSym<math::AXIS::Z>::replace}
	};


	/*
	 * Surfカードをスキャンしてマクロボディ発見したら…
	 * ・マクロボディインスタンスを作成
	 * ・MacroBody::replaceSurfaceCard()
	 * ・MacroBody::replaceCellCard()を実行
	 * というようにすればOO的にはわかりやすいが、マクロボディインスタンスごとに
	 * cellカード全体をスキャンする必要が出てしまうので採用しない
	 *
	 */
	// key:マクロボディが使われているsurface名、 val:使われているマクロボディの種類(ニモニック)のマップ
	std::unordered_map<std::string, std::string> macroBodySurfaces;
	/*
	 * まずsurfaceカードのマクロボディ展開を実行する。
	 *	S1 BOX -10 -10 -10   20 0 0  0 20 0  0 0 20 は
	 * 以下の6平面に展開される。
	 *	S1.1 P -1  0  0  10
	 *	S1.2 P  1  0  0 -10
	 *	S1.3 P  0 -1  0  10
	 *	S1.4 P  0  1  0 -10
	 *	S1.5 P  0  0 -1  10
	 *	S1.6 P  0  0  1 -10
	 */
//	mDebug() << "num surfInputlines===" << surfInputList->size();
//	for(auto it = surfInputList->begin(); it != surfInputList->end(); ++it) {
//	 mDebug() << "surfcard===" << it->data;
//	}
	for(auto it = surfInputList->begin(); it != surfInputList->end(); ++it) {
		try {
			auto scard = inp::SurfaceCard::fromString(it->data);
			if(expanderMap.find(scard.symbol) == expanderMap.end()) {
				continue;
				// macrobodyではないのでなにもしない
			}
			std::pair<std::string, std::string> nameSymbolPair;
			// ここでexpanderマップに格納したマップからマクロボディ展開関数を実行し、surfInputListを書き換える。
			nameSymbolPair = expanderMap.at(scard.symbol)(trMap, it, surfInputList);
			// 展開したマクロボディのサーフェイス面表裏と記号を登録しておく。例の場合(S1, BOX),(-S1,BOX)
			macroBodySurfaces.insert(nameSymbolPair);
			macroBodySurfaces.emplace(Surface::reverseName(nameSymbolPair.first), nameSymbolPair.second);
		} catch (std::exception &e) {
            std::stringstream sse;
            sse << it->pos() << " While expanding macrobody, " <<  e.what();
            throw std::invalid_argument(sse.str());
		}
    }
	//mDebug() << "macrobodies=\n" << macroBodySurfaces;


	/*
	 * 次にcellカードで使われているマクロボディを非マクロボディ平面の集合で置き換える。
	 * C1  0 -S1:S1
	 * S1 BOX .....
	 * は
	 * C1  0 (-S1.S1 S1.2 -S1.3 S1.4 -S1.5 S1.6):(S1.1:S1.2:S1.3:S1.4:S1.5:S1.6)
	 * に置き換えられる。
	 */
	for(auto it = cellInputList->begin(); it != cellInputList->end(); ++it) {
		inp::CellCard card;
		try{
			card = inp::CellCard::fromString(it->data);
		} catch (std::exception & e) {
			std::stringstream ss;
			ss << it->pos() << " While reading cell card, " << e.what();
			throw std::runtime_error(ss.str());
		}

		/*
		 * card.equationは論理多項式文字列なのでそこからsurface面を取り出す必要がある。
		 */
//        mDebug() << "macroBodies====" << macroBodySurfaces;
		std::vector<std::string> surfaceNames = Surface::extractSurfaceNames(card.equation);
        for(auto &surfaceName: surfaceNames) {
//            mDebug() << "checking macrobody surf====" << surfaceName;
			auto macroBodyItr = macroBodySurfaces.find(surfaceName);
			if(macroBodyItr != macroBodySurfaces.end()) {

				std::string macroBodyName = macroBodyItr->first;
				std::string macroBodySymbol = macroBodyItr->second;

				if(replacerMap.find(macroBodySymbol) == replacerMap.end()) {
                    std::stringstream sse;
                    sse << it->pos() << "Expansion of macrobody \"" << macroBodySymbol << "\" is not implimented.";
                    throw std::invalid_argument(sse.str());
				}
				// ここでreplacerマップに格納したマップからマクロボディ置換関数を実行し、itを通じてcellInputListを書き換える。
				replacerMap.at(macroBodySymbol)(macroBodyName, it);
			}// macroBodySurfacesの探索終わり
		}
	}

//	mDebug() << "macrobody置換後のcellInput=";
//	for(auto &l: *cellInputList) {
//		mDebug() << "cl=" << l.data;
//	}
	//	mDebug();
}

void geom::Geometry::setReservedPalette()
{
    palette_.registerColor(geom::Cell::UNDEF_CELL_NAME,   mat::Material::UNDEF_NAME,   img::Color("#FFFFFF", 0.0));
    palette_.registerColor(geom::Cell::VOID_CELL_NAME,    mat::Material::VOID_NAME,    img::Color("#FFFFFF", 1.0));
    palette_.registerColor(geom::Cell::UBOUND_CELL_NAME,  mat::Material::UBOUND_NAME,  img::Color("#FF0000"));
    palette_.registerColor(geom::Cell::BOUND_CELL_NAME,   mat::Material::BOUND_NAME,   img::Color("#000000"));
    palette_.registerColor(geom::Cell::DOUBLE_CELL_NAME,  mat::Material::DOUBLE_NAME,  img::Color("#666666"));
	palette_.registerColor(geom::Cell::OMITTED_CELL_NAME, mat::Material::OMITTED_NAME, img::Color("#330099"));

}



void geom::Geometry::clearUserDefinedPalette()
{
    palette_.clear();
    this->setReservedPalette();
}

// matMapを参考にしてパレットを作る。matMapに記載のない色は適当にimg::Color::getDefaultColorを使う
void geom::Geometry::createModifiedPalette(const std::map<std::string, img::MaterialColorData> &matMap)
{

    // 重複のない材料名setを作ってからindexを割り当てる。
    std::set<std::string> matNameSet;
    for(auto &cellpair:cells_) {
        matNameSet.emplace(cellpair.second->cellMaterialName());
    }
    std::unordered_map<std::string,int> matIndexMap;
    int matIndex = 0;
    for(auto it = matNameSet.cbegin(); it != matNameSet.cend(); ++it) {
        //mDebug() << "material===" << *it << ", index===" << matIndex;
        matIndexMap.emplace(*it, matIndex++);
    }

    for(const auto& cellPair: cells_) {
        const std::string &cellName = cellPair.first;
        const std::string &matName = cellPair.second->cellMaterialName();
        if(matMap.find(matName) != matMap.end()) {
            // matMapにエントリがある時
            const img::MaterialColorData &matColData = matMap.at(matName);
            auto color = matColData.color();
            if(color) palette_.registerColor(cellName, matName, matColData.aliasName(), matColData.printSize(), *color.get());
        } else {
            // 無い時は適当にデフォルトカラーマップから色を採る。
            //mDebug() << "registering color for material ===" << matName << "index===" << matIndexMap.at(matName);
            palette_.registerColor(cellName, matName, img::Color::getDefaultColor(matIndexMap.at(matName)));
        }
    }
}


void geom::Geometry::setDefaultPalette()
{
	/*
	 * cellリストのセルを登録していく
     * 一旦重複のない材料名セットを作って材料に一対一対応したindexを割り当て、
     * indexに応じた色を選択する。
     * palette構築はそれほど頻度が多くないので多少非効率でも問題ない。
	 */
    std::set<std::string> matNameSet;
    for(auto &cellpair:cells_) {
        matNameSet.emplace(cellpair.second->cellMaterialName());
    }
    std::unordered_map<std::string,int> matIndexMap;
    int matIndex = 0;
    for(auto it = matNameSet.cbegin(); it != matNameSet.cend(); ++it) {
        matIndexMap.emplace(*it, matIndex++);
    }

	std::map<std::string, std::shared_ptr<const Cell>> cellMap;
	for(auto &cellpair:cells_) {
		cellMap.emplace(cellpair.first, cellpair.second);
	}

    for(auto &cellpair:cells_) {
        auto color = img::Color::getDefaultColor(matIndexMap.at(cellpair.second->cellMaterialName()));
//        mDebug() << "registering to palette, cellName=" << cellpair.first
//                 << "materialName=" << cellpair.second->cellMaterialName()
//                 << "color ===" << color.toRgbString();
		palette_.registerColor(cellpair.first,
							   cellpair.second->cellMaterialName(),
                               color);
	}
}
