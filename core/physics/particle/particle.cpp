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
#include "particle.hpp"

#include <mutex>
#include <iomanip>
#include "core/physics/particleexception.hpp"

namespace {
const int MAX_DEPTH = 1000;
const int MAX_SURFACES_PER_CELL = 1000;
std::mutex mtx;
}


/*
 * ・movetToCellBoundは粒子をcell境界まで移動させ、nextSurfaceをセットする。
 * ・enterCellは粒子を次のcell内部へ移動させる。 nextSurfaceはnullptrで初期化される。
 */

/* 未定義領域は全てのsurfaceに隣接するとして扱う
 * その結果何が起こるか？
 *
 * moveToCellBoundを未定義セル内部で実行した場合
 *	→ 通常粒子は未定義セルに入らないので考慮する必要なし
 *
 * enterCellで未定義セルから出る場合。
 *	→ 通常の粒子は未定義セルから出てこないので考慮する必要なし
 *
 * enterCellで未定義セルに入る場合
 *	→ cellが未定義セルになる。通常粒子はここでterminateされるべし。
 */

/*
 * エラー/例外処理
 *
 * moveToCellboundでエラーを起こす条件
 * 1．次の交差点が見つからない
 *	・無限大セルに入った
 *		→ 粒子追跡可能な無限大セルを作ってはならない。ユーザーエラー。
 *			→ 例外を投げる
 *
 * enterCellでエラーを起こす条件
 * ・cell境界近傍にいない(nextSurface == nullptr)場合
 *		→ (moveToCellBoundが実行されていないetc)前提条件が満たされていない。プログラムエラー。
 *			→assertなどで即終了
 *
 *
 */


int phys::Particle::COUNTER = 0;

phys::Particle::Particle(double w, const math::Point &p, const math::Vector<3> &v, const double &e,
						 //const std::shared_ptr<const geom::Cell> &startCell,
						 const geom::Cell* startCell,  // ナマポ版
						 const phys::Particle::cell_list_type &cellList,
						 bool record, bool guessStrict)
	:weight_(w), position_(p), direction_(v.normalized()), energy_(e), time_(0),
	  recordEvent_(record), currentCell_(startCell), cellList_(&cellList)
{
	{
	std::lock_guard<std::mutex> lk(mtx);
	ID_ = ++COUNTER;
	}
	// 発生セルと発生位置に矛盾があれば例外発生
	// TODO もう少し物理的な情報を含んだデータを投げたい。
	if(currentCell_ != nullptr) {
		if(!currentCell_->isInside(position_)) {
//			throw std::invalid_argument(std::string("Initial pos (") + position_.toString()
//										+ ")is out of the cell " + currentCell_->toString());
			throw InvalidSource("initial position is in the undefined region",
								currentCell_, position_, direction_);
		}
	} else {
		// 発生セルがnullptrなら自動推定する。
		//currentCell_ = geom::Cell::guessCell(cellList_, position_, guessStrict, true);
		currentCell_ = geom::Cell::guessCell(*cellList_, position_, guessStrict, true).get();  // ナマポ版
	}
	if(recordEvent_) {
//		mDebug() << "cellname=" << currentCell_->cellName();
		std::string note = std::string("cell=") + currentCell_->cellName();
		events_.emplace_back(createEventRecord("Source production", note));
	}
}

// 次のsurfaceまで移動する。ただしこのsurfaceはセル境界かセル内部のsurfaceかはわからない。
void phys::Particle::moveToSurface()
{
//	mDebug() << "###EEnter Particle::moveToSurface  current cell=" << currentCell_->cellName() << "pos=" << position_;

	auto positionBeforeMove = position_;
	/*
	 * 粒子の移動
	 *	1. cellを構成するsurfaceとの交点をもとめる。
	 *	2. 交点の位置から実際に次に移動する交点を選ぶ
	 */
	auto surfacePositionPair = currentCell_->getNextIntersections(position_, direction_);
//	for(const auto& surf: surfacePositionPair.first) {
//		mDebug() << "Next intersection , name=" << surf->name() << ", pos=" << surfacePositionPair.second;
//	}

	if(surfacePositionPair.first.empty()) {
		// 交点が見つからない場合 NOTE exceptionを継承した no_intersectionみたいな例外作ろう
//		std::stringstream ss;
//		ss << "Error:No intersection has found.\n"
//		   << "Cell = " << currentCell_->cellName() << ", position =" << position_ << ", direction =" << direction_;
//		throw std::runtime_error(ss.str());
		throw NoIntersection("No intersection has found", currentCell_, position_, direction_);

	} else {
		// 交点が見つかった場合。
		nextSurfaces_ = surfacePositionPair.first;
		position_ = surfacePositionPair.second;
	}


	if(recordEvent_) {
		std::string note = std::string("trackL=") + utils::toString((position_ - positionBeforeMove).abs())
				+ std::string(", next_s=");
		for(size_t i = 0; i < nextSurfaces_.size(); ++i) {
			if(i != 0) note += ", ";
			note += nextSurfaces_.at(i)->name();
		}
		events_.emplace_back(createEventRecord("Move to surface", note));
	}
	//mDebug() << "end moveToSurface";
}



// cell境界まで粒子を移動させる。(moveToSurfaceはsurfaceまで動くがセル境界とは限らない)
void phys::Particle::moveToCellBound() {
//	if(recordEvent_) {
//		std::cout << "\n\n##### Particle will move to cell bound!  "
//				  <<"current pos =" << position_ << ", direction =" <<direction_
//				 << ", currentcell=" << currentCell_->cellName() << std::endl;
//	}
	auto positionBeforeMove = position_;
	/*
	 * moveToSurfaceで次の面への交差点へと位置を更新できる。
	 * しかしセル境界と面はかならずしも一致しない。 セル境界∋面
	 * 交点の先が同じセルならセル境界として使われていない面との交点なので
	 * 位置を更新し、セルが変わるまでmoveToSurfaceを呼ぶ。
	 */

	this->moveToSurface();

	int numLoop = 0;
	while (currentCell_->isInside(position_ + direction_ * math::Point::delta())) {
		position_ = position_ + direction_ * math::Point::delta(); // ここで位置をdelta進めてsurfaceを跨ぐ
		this->moveToSurface();                                       // 次の面との交点の手前まで移動

		if(++numLoop > MAX_SURFACES_PER_CELL) {
			throw NoNewCell("No new cell found in forward direction(MAX_SURFACES_PER_CELL)", currentCell_, position_, direction_);
		}
	}

	if(recordEvent_) {
		std::string note = std::string("cell trackL=") + utils::toString((position_ - positionBeforeMove).abs())
				+ std::string(", next_s=");
		for(size_t i = 0; i < nextSurfaces_.size(); ++i) {
			if(i != 0) note += ", ";
			note += nextSurfaces_.at(i)->name();
		}
		events_.emplace_back(createEventRecord("Move to cell bound", note));
	}

}

void phys::Particle::enterCell()
{

//	mDebug() << " ########## ENTER Particle::enterCell() cell===" << currentCell_->cellName()
//			 << "particleID=" << ID_ << "pos=" << position_ << "dir=" << direction_;
	if(nextSurfaces_.empty()) {
		// nextSurfaces_が設定されていない場合は境界上にいないとしてエラー終了する。
		std::cerr << "ProgramError: No next surface is set." << std::endl;
		std::exit(EXIT_FAILURE);
	}

	// ##### 位置の更新。現在位置からセルが変わるまで移動する。
	//mDebug() << "previous cell===" << currentCell_->cellName() << ", pos=" << position_;
	int numLoop = 0;
	do {
		++numLoop;
		position_ += direction_*math::Point::delta();
		if(numLoop > MAX_LOOP) {
			std::cerr << "Fatal: enterCell() is called inside cell, and never ends." << std::endl;
			std::exit(EXIT_FAILURE);
		}
		//mDebug() << "loop=" << numLoop;
	} while(currentCell_->isInside(position_));

//	mDebug() << "position updated to ===" << position_ << "old current Cell=" << currentCell_->cellName();




	/*
	 * 現在セルの更新
	 * valueはstd::pair<string, weak_ptr<const Cell>>
	 * nextSurface_は既にここまでのところで確定しているのでnextSurface_に面しているcellを調べる
	 */
	//mDebug() << "境界面" << nextSurface_->name() << "contact=" << utils::toString(nextSurface_->contactCellsMap());
	bool hasFoundCell = false;

//	mDebug() << "CELL=" << currentCell_->cellName() << "の次交差面は ";
//	for(auto &surf: nextSurfaces_) {
//		mDebug() << surf->name();
//	}

	/*
	 * TODO nextSurface_に面しているcellを走査する方式だとlatticeの時に遅くなる。階層構造を使えば良い
	 * なにかlattice構成面の場合は別の方法を適用したい。
	 * 理由は
	 * 1． 1つの面にあまりにも多数のcellが面してしまうため
	 *    → lattice情報をtreeで管理する
	 * 2． latticeで生成する面が多いと、SurfaceMap内のvectorが疎になってSurfaceMap::frontSurfaces()が遅くなる
	 *     → Lattice1個あたり6程度の欠落なのでそれほど気にする必要はなさそう。
	 *
	 *
	 */
	for(auto &surf: nextSurfaces_) {
		auto cellMap = surf->contactCellsMap();

//		mDebug() << "この交差予定面" << surf->name() << "に接触しているセルは＝";
//		int cmax = 0;
//		for(auto &value: cellMap) {
//			mDebug() << value.first;
//			if(++cmax > 15) {
//				mDebug() << "..................... +other cells";
//				break;
//			}
//		}
		for(auto &value: cellMap) {
			// 今の粒子追跡計算負荷の大半はここでweak_ptrのshared_ptr作成にある。 → そんなことなかった。
			// weak_ptrはlockのたびにshared_ptrを複製して非効率だからsurface->cellは通常ポインタでいいのでは
//			mDebug() << "内外判定中 pos=" << position_ << ", cell=" << value.first
//					 << "isinside=" << value.second->isInside(position_);


//			if(value.second.lock()->isInside(position_)) { // cellの持つsurfaceへのポインタはweak_ptr
//				currentCell_ = value.second.lock();  // weak_ptr
//				hasFoundCell = true;
//				break;
//			}
			// ナマポ版
			if(value.second->isInside(position_)) { // cellの持つsurfaceへのポインタはshared_ptrだと循環参照
				currentCell_ = value.second;  // ナマポ
				hasFoundCell = true;
				break;
			}
		}

		if(hasFoundCell) break;
	}


	// 次のセルが見つからなかった場合 currentCellはundefinedになる。
	if(!hasFoundCell) {
		/*
		 * 複数面の交差部分をまたぐ場合、新しいセルの探索には失敗する場合があり、
		 * well-definedな入力データであったもこれは避けられない。
		 * (deltaよりも薄い部分を飛び越す場合がある)
		 * なので、次のセルが見つからなかった場合、現在位置からcellを推定し、
		 * これが失敗した後に初めてundefinedとなる。
		 */
		// ここは主に微小セルを飛び越した場合が想定されているので、
		// 厳密な現在セル判定は不要。復帰できればどのセルからでも良い。
//		mDebug() << "\n########### Missing next cell, starting guess.";
//		mDebug() << "Previous cell=" << currentCell_->cellName();
//		mDebug() << "Current pos=" << position_ << "dir=" << direction_;
//		mDebug() << "Current guessed cell =" << geom::Cell::guessCell(*cellList_, position_, false, false)->cellName();
//		mDebug() << "ABORT for debugging.";
//		abort();
		currentCell_ = geom::Cell::guessCell(*cellList_, position_, false, false).get();
	}

//	mDebug() << "updated cell to " << currentCell_->cellName();
//	nextSurface_ = nullptr;
	nextSurfaces_.clear();
	if(recordEvent_) events_.emplace_back(createEventRecord("Entered to new cell", std::string("new_c=") + currentCell_->cellName()));
}


#define SCI_OUT(ARG1, ARG2) std::setw(ARG1) << std::setprecision(ARG2) << std::showpos << std::left
void phys::Particle::dumpEvents(std::ostream &ost)
{
	// 表示項目は event30文字+空白+pos12文字(有効六桁)＋空白＋dir12文字＋空白＋cell名7文字＋空白＋time12文字＋空白＋noteice適当
	ost << std::setw(20) << std::left << "Event"     << " "
		<< std::setw(13*3) << std::left << "position"
		<< std::setw(13*3) << std::left << "direction"
		<< std::setw(7)  << std::left << "cell"      << " "
		<< std::setw(13) << std::left << "time"      << " " << "notice" << std::endl;
	for(auto &evRecord: this->events_){
		ost << std::setw(20) << evRecord.eventName << " "
			<< SCI_OUT(12,5) << evRecord.position.x()  << " "
			<< SCI_OUT(12,5) << evRecord.position.y()  << " "
			<< SCI_OUT(12,5) << evRecord.position.z()  << " "
			<< SCI_OUT(12,5) << evRecord.direction.x() << " "
			<< SCI_OUT(12,5) << evRecord.direction.y() << " "
			<< SCI_OUT(12,5) << evRecord.direction.z() << " "
			<< std::setw(7)  << evRecord.cellName    << " "
			<< SCI_OUT(12,5) << std::scientific << evRecord.time      << " " << evRecord.notice << std::endl;
	}
	ost << std::fixed << std::noshowpos;
}

