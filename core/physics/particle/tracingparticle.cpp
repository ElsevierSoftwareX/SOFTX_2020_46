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
#include "tracingparticle.hpp"

#include <iostream>
#include <stdexcept>

#include "core/physics/particleexception.hpp"
#include "core/utils/message.hpp"

/*
 *  cell境界まで移動する。
 * 普通の粒子と異なる点は
 * ・無限セルへ入射しても終了しない
 * ・寿命長(lifelength)がある
 * のでその点はこのクラスで処理する。
 */
void phys::TracingParticle::moveToBound()
{
	if(recordEvent_) mDebug() << "Enter to TracingParticle::MoveToCellBound current pos="
							  << position_ << "current cell=" << currentCell_->cellName();

	if(lifeLength_ < math::Point::eps()) return;
	auto beforeMovedPosition = position_;
	/*
	 *
	 * 通常の粒子と同様に移動させる。
	 * 但しTracingParticleは無限大セルに入ってもlifeLength_が残っていたら死なないので
	 * std::runtime_errorは捕捉して続行する。
	 */
	try {
		// 再帰呼び出しなどでParticle::moveToCellBoundが確実に(内部surfaceではなく)隣接セル境界まで
		// 動いてくれるのでmoveToCellBoundは一回呼ぶだけで良い
		Particle::moveToCellBound();
	} catch (phys::ParticleException &pe) {
		// 寿命が尽きる前に体系を突き抜けたらNoNewCell/NoIntersectionが発生する。
		//mDebug() << "particle exception what ===" << pe.what();
		(void) pe;
		// 無限セルで移動しようとした場合のこりの寿命の位置まで移動して終了
		position_ += lifeLength_*direction_;
		// 初回以外のtrackLength追加時にはenterCellで動いた分の補償が加わる
		double correctedLength = trackLengths_.empty() ? lifeLength_ : lifeLength_ + math::Point::DELTA;
		trackLengths_.emplace_back(correctedLength);
		passedCells_.push_back(currentCell_->cellName());
		if(recordEvent_) events_.emplace_back(createEventRecord("Expired in infcell", "" ));
		lifeLength_ = 0;
		return;
	} catch (std::exception &e) {
        std::stringstream ess;
        ess << __FILE__<< ":" << __LINE__ << "ProgramError. Unexpected exception happend. whawt=" << e.what();
        throw std::invalid_argument(ess.str());
	}

	// Particle::moveToCellBound();で境界へ移動できたケースの処理
	double length = math::distance(position_, beforeMovedPosition);  // 移動量
	if(lifeLength_ > length) {
		// 移動量が残りのlifeより短ければ移動量をpushback
		lifeLength_ -= length;  // 寿命を消費
		//mDebug() << "TrackLengthを追加=" << length;
		// 初回以外の飛程長追加時にはenterCellによる移動分の補償が加わる。
		if(!trackLengths_.empty()) length += math::Point::DELTA;
	} else {
		// length >= maxLength_
		// 移動量が残りのmaxLengthを超えていれば位置を戻して、修正した移動量をpushback
		position_ += (lifeLength_ - length)*direction_;
		if(recordEvent_) events_.emplace_back(
								createEventRecord("Expired in finitcell",
												std::string("back to EoL point, trackL-=")
												+ utils::toString(lifeLength_ - length)) );
		lifeLength_ = 0;
		// 初回以外のtracklengthには最後のenterCell移動分の補償が加わる
		length = trackLengths_.empty() ? math::distance(position_, beforeMovedPosition)
									   : math::distance(position_, beforeMovedPosition) + math::Point::DELTA;
	}
	trackLengths_.emplace_back(length);
	passedCells_.push_back(currentCell_->cellName());
	return;
}


void phys::TracingParticle::enterCellTr() {
//	mDebug() << "enterCellTrに入った pos=" << position_ << "セル変更まえのcell=" << currentCell_->cellName() << "lifelength===" << lifeLength_;
//	auto oldpos = position_;
	if(lifeLength_ < math::Point::eps()) return; // 寿命が付きていたら何もしない
	assert(!trackLengths_.empty());
	Particle::enterCell();


	//mDebug() << "Particle::enterCellを出た cell=" << currentCell_->cellName() <<  "移動量は=" << position_ - oldpos;
	// enterCell()を実行するとdelta()分だけ位置が動くが、その分はtrackLength, lifeLength_には反映されないので補償する
	// trackLengthの補償は前のセルではなく、次のセルの飛程長に加えるべき。
	double offset = math::Point::DELTA;
	//trackLengths_.back() += offset ;
	lifeLength_ -= offset;

}

void phys::TracingParticle::trace()
{
	if(recordEvent_) {
		std::cout << "初期位置===" << position_ << "初期方向" << direction_
				  << "初期セル" << currentCell()->cellName() << std::endl;

		int nmove = 0;
		math::Point oldpos;
		while(!expired()) {
			++nmove;
			if(nmove == 2) {
				mDebug() << "nmove===" << nmove;
			}
			mDebug() << "\nTraceルーチンループ開始 pos===" << position_ << "cell=" << currentCell_->cellName();
			try {
				oldpos = position_;
				this->moveToBound();
				//std::cout << "moveToBoundでの移動量 =" << std::scientific << position_ - oldpos << std::endl;
				mDebug() << nmove << "回目の移動後位置 pos=" << position_ << ", cell=" << currentCell()->cellName() << "life=" << lifeLength_;
			} catch (std::exception & e) {
				std::cerr << e.what();
				abort();
			}
			//oldpos = position_;
			auto oldName = currentCell_->cellName();
			enterCellTr();
			//std::cout << "enterCell()での移動量" << std::scientific << position_.x() - oldx << "\n" << std::endl;
			mDebug() << "セル入射(enterCellTr)後の位置=" << position_ << "セルは" << oldName << "から" << currentCell_->cellName() << "へ変更";
			//assert(oldName != currentCell_->cellName());

		}

	} else {
		while(!expired()) {
//			try {
//				this->moveToBound();
//				// this->moveToBoundが発生させ得る例外は今の所存在しない。
//			} catch (std::exception & e) {
//				std::cerr << "Exception occured!" << std::endl;
//				std::cerr << e.what();
//				throw e;
//			}
			this->moveToBound();
			enterCellTr();
		}
	}
}
