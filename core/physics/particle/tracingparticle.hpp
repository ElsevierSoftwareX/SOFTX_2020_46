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
#ifndef TRACINGPARTICLE_HPP
#define TRACINGPARTICLE_HPP


#include <stdexcept>
#include <string>
#include "particle.hpp"
#include "core/math/nvector.hpp"
#include "core/geometry/cell/cell.hpp"

namespace phys {

// 断面画像作成用の相互作用を全くしない粒子
class TracingParticle : public Particle
{
public:

	TracingParticle(double w,
                        const math::Point &p,
                        const math::Vector<3> &v,
                        const double& e,
                        const geom::Cell *c,  // ナマポ版
                        const std::unordered_map<std::string, std::shared_ptr<const geom::Cell>>& cellList,
                        double maxLength,
                        bool recordEvent = false,
                        bool guessStrict = false)
		:Particle(w, p, v, e, c, cellList, recordEvent, guessStrict), lifeLength_(maxLength)
	{
		//passedCells_.push_back(currentCell_->cellName());  // 通過セルの追加はlengths_の追加時に同時に行う
		//passedCells_.emplace_back(currentCell_->cellMaterialName());
	}

	// 通過セルの情報を返す。
	const std::vector<std::string> &passedCells() const {return passedCells_;}
	// 通過セル内のtracklengthを返す
	const std::vector<double> &trackLengths() const {return trackLengths_;}
	// 飛行方向に沿って断面情報をトレースする。この関数で例外発生はあり得ない。
	void trace() ;

protected:
	double lifeLength_;
	// tracing粒子はセルが変わるごとにセル名とtracklengthを記録する
	std::vector<std::string> passedCells_;  // 通過したセル履歴
	std::vector<double> trackLengths_; // セル内のtrack length


	// 寿命が尽きていないかチェック
	virtual bool expired() const { return (lifeLength_ < math::Point::EPS) ? true : false;}
	// 現在セル中に居るとして、飛行方向にcell境界まで移動する。
	virtual void moveToBound();
	// 現在surface上にいるとして、Surfaceの向こう側のcellへ入る]
	virtual void enterCellTr();

};


}  // end namespace phys
#endif // TRACINGPARTICLE_HPP
