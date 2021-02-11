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
#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <iostream>
#include <memory>
#include "core/math/nvector.hpp"
#include "core/geometry/cell/cell.hpp"
#include "core/utils/utils.hpp"

namespace {

const int MAX_LOOP = 20;

}

namespace geom {
class Surface;
}

namespace phys {



struct EventRecord{
	EventRecord(const std::string &eventStr, const math::Point &pos, const math::Vector<3> &dir,
				const double ene, const double tme, const std::string &cell, const std::string &note)
		:eventName(eventStr), position(pos), direction(dir), energy(ene), time(tme), cellName(cell), notice(note)
    {}
	std::string eventName;
	math::Point position;
	math::Vector<3> direction;
	double energy;
	double time;
	std::string cellName;
	std::string notice;
};


// 全ての粒子クラスの基底
/*
 * CRTPを使うと動的ポリモーフィズムは使えなくなる。
 * コードも見づらくなるのでとりあえず普通に継承をつかっておいて
 * パフォーマンスに問題があれば後でtemplate使うように変更する。
 */
class Particle
{
	typedef std::unordered_map<std::string, std::shared_ptr<const geom::Cell>> cell_list_type;
public:
	/*
	 * 引数は物理量(位置、方向、エネルギー)に加えて
	 * 発生セル、全セルリスト(未定義領域確認のため必要)、イベントを記録するかのフラグ
	 * となる。発生セルがnullptrの場合発生セルは自動推定になる。
	 */
	// 未定義領域でのParticle発生にはInvalidSource例外が投げられる。
	Particle(double w, const math::Point &p, const math::Vector<3> &v, const double& e,
			 const geom::Cell *startCell,
			 const cell_list_type & cellList, bool record = false, bool guessStrict = false);



	math::Point position() const {return position_;}
	math::Vector<3> direction() const {return direction_;}
	//std::shared_ptr<const geom::Cell> currentCell() const {return currentCell_;}
	const geom::Cell* currentCell() const {return currentCell_;}  // ナマポ版
	double energy() const {return energy_;}
	double time() const {return time_;}
	double weight() const {return weight_;}
	void setWeight(double w) {weight_ = w;}


	// 次のsurfaceの手前まで移動する。
	// 交差するsurfaceが見つからなければNoIntersectionが投げられる
	void moveToSurface();
	// 現在セル中に居るとして、飛行方向にcell境界まで移動する。(このときにnextSurface_が決定される)
	// セルが見つからなければNoNewCellを投げる。
	void moveToCellBound();
	// 現在surface上にいるとして、Surfaceの向こう側のcellへ入る。
	void enterCell();
	void dumpEvents(std::ostream &ost);

protected:
	EventRecord createEventRecord(const std::string & eventStr, const std::string &note = std::string()) const {
		return EventRecord(eventStr, position_, direction_, energy_, time_, currentCell_->cellName(), note);
	}
	// massが必要な粒子は、massをstatic const doubleで定義すれば良い。
	std::size_t ID_;
	double weight_;
	math::Point position_;   // 位置
	math::Vector<3> direction_;  // 方向ベクトル
	double energy_;
	double time_;
	bool recordEvent_;  // 履歴を記録するかどうか。
	std::vector<EventRecord> events_;  // 記録したイベント

	//std::shared_ptr<const geom::Cell> currentCell_;
	const geom::Cell* currentCell_;  // ナマポ版

	// 次に交差するsurface。セル境界まで移動したらセットされ、surfaceを跨いでcellに入ったらnullptr初期化される。
	std::vector<const geom::Surface*> nextSurfaces_;
	// 未定義領域確認のための全セルリスト
	const cell_list_type * const cellList_;

	static int COUNTER;
};




}  // end namespace phys
#endif // PARTICLE_HPP
