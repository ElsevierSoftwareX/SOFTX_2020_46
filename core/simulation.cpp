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
#include "simulation.hpp"

#include <deque>
#include <regex>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef ENABLE_GUI
#include <atomic>
#include <QApplication>
#include <QProgressDialog>
#endif


#include "geometry/geometry.hpp"
#include "image/bitmapimage.hpp"
#include "image/matnamecolor.hpp"
#include "image/cellcolorpalette.hpp"
#include "material/materials.hpp"
#include "physics/particle/uncollidedphoton.hpp"
#include "source/phits/phitssource.hpp"
#include "tally/pktally.hpp"
#include "utils/utils.hpp"
#include "utils/system_utils.hpp"
#include "utils/time_utils.hpp"
#include "utils/progress_utils.hpp"
#include "option/config.hpp"
#include "io/input/mcnp/mcnpinput.hpp"
#include "io/input/phits/phitsinput.hpp"
#include "io/input/phits/phitsinputsection.hpp"
#include "io/input/qad/qadinput.hpp"
#include "io/input/mars/marsinput.hpp"
#include "io/input/common/trcard.hpp"
#ifndef NO_ACEXS
#include "component/libacexs/libsrc/xsdir.hpp"
#endif
namespace {

// 空間座標位置から画像平面のピクセル位置(左上原点)への変換
std::pair<int, int> convertToPixPosition(const math::Point &pos, const math::Point leftBottom,
										 const math::Vector<3> hDir, const math::Vector<3> vDir,
										 size_t hReso, size_t vReso)
{
	double hpos = math::dotProd(hDir.normalized(), pos - leftBottom);
	double vpos = math::dotProd(vDir.normalized(), pos - leftBottom);
	int hpix = static_cast<int>(std::round(hReso*hpos/hDir.abs()));
	int vpix = static_cast<int>(vReso) - static_cast<int>(std::round(vReso*vpos/vDir.abs()));
	return std::make_pair(hpix, vpix);
}



}  // end anonymous namespace



Simulation::Simulation()
#ifdef ENABLE_GUI
    : QObject(nullptr)
#endif
{}

Simulation::~Simulation(){}

void Simulation::setTallies(const std::vector<std::shared_ptr<tal::PkTally> > &tals)
{
	tallies_ = tals;
//	for(auto &tally: tals) {
//		const std::shared_ptr<const tal::Tally> ctally = tally;
//		tallies_.emplace_back(ctally);
//	}
}

void Simulation::setSources(const std::vector<std::shared_ptr<const src::PhitsSource> > &srcs)
{
//	// constに変換しておく
//	for(auto &source: srcs) {
//		const std::shared_ptr<const src::PhitsSource> csource = source;
//		sources_.emplace_back(csource);
//	}
	sources_ = srcs;
}

void Simulation::run(int numThread)
{
	if(sources_.empty()) {
        throw std::invalid_argument("No source was defined.");
	} else if(tallies_.empty()) {
		mWarning() << "No tally was defined.";
		std::exit(EXIT_SUCCESS);
	}

	/*
	 * ソースループ→ソースは数個
	 * タリーループ→タリーも数個レベル
	 * 粒子ループ→ 粒子は線源離散化で大量のメッシュが発生しうるので大量
	 *
	 * よって粒子ループをマルチスレッド実行したい。
	 */
	(void) numThread;
	// Point Detector
	for(std::shared_ptr<tal::PkTally> &tal: tallies_) {
		for(std::shared_ptr<const src::PhitsSource> &src: sources_) {
			mDebug() << "######### SOURCE";
			mDebug() << src->toString();
			bool recordEvent = true;
			for(auto &detPoint: tal->detectionPoints()) {
				auto particles = src->generateUncollideParticles(detPoint, geometry_->cells(), recordEvent);
				tal::TallyPointData tallyPointData(detPoint);
				for(auto &par: particles) {
					par->trace();
//					mDebug() << "cells=" << par->passedCells();
//					mDebug() << "tracks=" << par->trackLengths();
//					mDebug() << "weight=" << par->weight();
//					par->dumpEvents(std::cout);
					tal::UncollidedFluxData ufdata(par->uncollidedFlux(), par->passedCells(), par->mfpTrackLengths());
					tallyPointData.appendFluxData(ufdata);
				}
				tal->appendTalliedData(tallyPointData);
			}  // ここでタリー点1個計算終わり
		}
	}// ソースの数だけ
}


void Simulation::clear() noexcept
{
    defaultPalette_.reset();
	materials_.reset();
	geometry_.reset();
	tallies_.clear();
	sources_.clear();
}



// 第一引数は画面左下の座標
img::BitmapImage Simulation::plotSectionalImage(const math::Vector<3> &origin,
										   const math::Vector<3> &hDir,
										   const math::Vector<3> &vDir,
										   size_t hReso, size_t vReso,
										   int lineWidth, int pointSize,
                                           int numThread, bool verbose, bool quiet) const
{
	img::BitmapImage bitmap
            = geometry_->getSectionalImage(origin, hDir, vDir, hReso, vReso, numThread, verbose, quiet);

	// ジオメトリ描画は領域分割して走査した方が重複定義検出に敏感になる
	//  h1img, h2img, h3img...を連結してhimgを作り、
	//  v2img, v2img, v3img...を連結してvimgを作ってからhimg,vimgでimgを作成する。

	// 線幅の拡張
	bitmap.expandRegion(lineWidth, geom::Cell::UBOUND_CELL_NAME);
	bitmap.expandRegion(lineWidth, geom::Cell::BOUND_CELL_NAME);
	//bitmap.expandLineWidth(lineWidth);

	assert(hReso != 0 && vReso != 0);
	double pointSizeCm = pointSize* 0.5*(hDir.abs()/hReso + vDir.abs()/vReso);
	// NOTE hdir, vdirは方向と長さを別にしておかないとexで領域を狭めた時に↓でzerodivになる。[hv]dirがゼロになることはある？
	math::Vector<3> normalVec = math::crossProd(hDir, vDir).normalized();

	/*
	 * ソース点(in グローバル座標系)の表示にはグローバル座標系から画面内pixel座標系へ変換する
	 * 座標変換に必要な情報は
	 * ・画面の水平・垂直方向ベクトル(グローバル座標系、画面のサイズを含む)、
	 * ・画面の水平・垂直方向解像度
	 * ・原点（画面左下の座標）
	 * convert(srcPoint, imgOrigin, hdir, vdir, hreso, vreso);
	 */
	img::Color srcColor = img::Color("#aa0000");
	for(auto &source: sources_) {
		if(!source) continue;
		for(auto &srcPoint: source->sourcePoints()) {
			//mDebug() << "srcPts=" << srcPoint;
			double dist = std::abs(math::dotProd(srcPoint - origin, normalVec));  // ソース点とプロット平面の距離
			auto hv = convertToPixPosition(srcPoint, origin, hDir, vDir, hReso, vReso);
			bool fill = dist < pointSizeCm;
			bitmap.drawSquareMark(hv.first, hv.second, pointSize, srcColor, fill);
		}
	}
	// タリー位置プロット
	img::Color tallyColor = img::Color("#000099");
	for(auto &tally: tallies_) {

		for(auto detPoint: tally->detectionPoints()) {
			double dist = std::abs(math::dotProd(detPoint - origin, normalVec));
			auto hv = convertToPixPosition(detPoint, origin, hDir, vDir, hReso, vReso);
			if(dist < pointSizeCm) {
				bitmap.drawCrossMark(hv.first, hv.second, pointSize+1, tallyColor);
			} else {
				bitmap.drawSquareCrossMark(hv.first, hv.second, pointSize, tallyColor);
			}
		}
	}
	return bitmap;
}

std::string Simulation::finalInputText() const
{
	std::string input;

	if(geometry_) input += geometry_->toFinalInputString();  // まずジオメトリ(cell, surface)の出力
	//if(materials_) input += materials_->toFinalInputString();

	// input += tallies->toFinalInputString();
	// input += sources->toFinalInputString();
	return input;
}

const std::unique_ptr<const img::CellColorPalette> &Simulation::defaultPalette() const {return defaultPalette_;}





















void Simulation::init(const std::string &inputFileName, conf::Config config)
{
    inputFileName_ = inputFileName;
    /*
     * Simulationインスタンスの構築方法を再度整理・リファクタリングする
	 *（ソース、タリーは後回しor今のまま）
	 *
	 * PHITSの場合
	 * 1．パラメータセクションの読み取り→xsdirファイル名の確定
	 * 2．particle typesの先読み → ptypes確定
	 * 3．Materialsの作成
	 * 4. Geometryの作成
	 *
	 * MCNPの場合
	 * 1．データカードの読み取り。Materialカード、MODEカードからのptypes読み取り
	 * 2．Materialsの作成
	 * 3. Geometryの作成
	 */

	/* メタカード適用表
	 *	セクション	定数置換  行連結 改行挿入 ijmr展開
	 * Parameter		○		×		○		×
	 * Surface			○		○		○		？
	 * Material			○		○		○		？
	 * Cell				○		○		○		？
	 * Transform		○		○		○		○
	 */

	// InputData:initはmcnp/phits共通処理(include,コメント除去)を実施
	std::unique_ptr<inp::InputData> input;


	McMode mode = (config.mode == McMode::AUTO) ? inp::guessMcModeFromSuffix(inputFileName) : config.mode;
	if(mode == McMode::AUTO)  mode = inp::guessMcModeFromFile(inputFileName);





	if(mode == McMode::QAD) {
		input.reset(new inp::qad::QadInput(config));
	} else if(mode == McMode::MARS) {
		input.reset(new inp::mars::MarsInput(config));
	} else if(mode == McMode::MCNP) {
		input.reset(new inp::mcnp::McnpInput(config));
	} else {
		input.reset(new inp::phits::PhitsInput(config));
	}
	assert(input);



#ifdef ENABLE_GUI
	connect(input.get(), &inp::InputData::fileOpenSucceeded, this, &Simulation::fileOpenSucceeded);
#endif

	input->init(inputFileName);
	std::vector<phys::ParticleType> ptypes;
    if(!config.noXs && input->confirmXsdir()) ptypes = input->particleTypes();


	// Material構築。 長いのでGUIの時はprogressDialogを出す。
	std::shared_ptr<const mat::Materials>  materials;


#ifdef ENABLE_GUI
	std::exception_ptr ep; // 別スレッドで読み込むので例外転送する必要がある。
	static const std::array<QString, 5> dots{".... ", " ....", ". ...", ".. ..", ".... "};
    const size_t numMaterials = input->materialCards().size();
	QString message = tr("Reading cross section files...");
	QProgressDialog progress(message + dots.at(0), "", 0, numMaterials, 0);
    progress.setCancelButton(nullptr);
	progress.setMinimumDuration(1000);
	std::atomic_size_t count(0);
	std::thread materialThread([&](){
		try {
            materials = std::make_shared<const mat::Materials>(input->materialCards(), input->xsdirFilePath(), ptypes, count, config.verbose);
		} catch (std::exception &e) {
            Q_UNUSED(e)
			ep = std::current_exception();
			count = numMaterials;
		}
	});
	size_t loopCounter = 0;
	while(count != numMaterials) {
		progress.setValue(count);
		progress.setLabelText(message + dots.at(++loopCounter%dots.size()));
		// バックグラウンドのレスポンスを生かすためにsleepは短めかで。
		QApplication::processEvents();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	progress.close();

	materialThread.join();
	if(ep) std::rethrow_exception(ep);
#else
	std::atomic_size_t count(0);
    materials = std::make_shared<const mat::Materials>(input->materialCards(), input->xsdirFilePath(), ptypes, count, config.verbose);
#endif






	auto trMap = inp::comm::TrCard::makeTransformMap(input->transformCards());
	auto geometry = geom::Geometry::createGeometry(trMap, input->cellCards(), input->surfaceCards(), materials, config);



	// ここからジオメトリの色設定
	utils::SimpleTimer tm;
	tm.start();

    // FIXME 再読込の時になぜかmat-color-nameセクションが読み込まれていない。
    // FIXME 再読込の時になぜか前回のパレット変更結果が残っている。→ configがクリアされていない
    //mDebug() << "before creating palette!!!!!!!";
	if(!config.colorMap.empty()) {
        geometry->clearUserDefinedPalette();
        geometry->createModifiedPalette(config.colorMap);
	} else {
		if(!input->colorCards().empty()) {
            geometry->clearUserDefinedPalette();
            geometry->createModifiedPalette(img::MaterialColorData::fromCardsToMap(input->colorCards()));
		}
	}

    defaultPalette_ = std::make_unique<img::CellColorPalette>(geometry->palette());
    //mDebug() << "palettesize===" << defaultPalette_->size();

    // ユーザー定義色以外は除去する。

    // paletteの予約済み色以外はguiConfig(のcuiConfig)の色部分に反映させる。
    // Config::ColorMapは検索しやすいようにmatNameをキーにしたマップになっているので
    // 形式を変えるだけ。
    // Config::colorMapはstd::map<std::string, img::MaterialColorData>
    // materialColorDataListはstd::vector<std::shared_ptr<MaterialColorData>>


    tm.stop();
    if(config.verbose) mDebug() << "Set user defined color in " << tm.msec() << "(ms)";

	// 最終的にはmaterials, geometry, source, tallyの4要素をthisにセットすれば良い。
	this->setMaterials(materials);
	this->setGeometry(geometry);
}

