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
#ifndef TIMER_HPP
#define TIMER_HPP

#include <cassert>
#include <chrono>
#include <iostream>
#include <vector>

namespace utils {

// 簡易タイマークラス
class SimpleTimer{
    // repは規格では45bit以上の符号付き整数
    typedef std::chrono::milliseconds::rep rep_type;
public:
	explicit SimpleTimer(size_t numReserve = 100);
	void start();  //計測開始。 最初にクリアするのでclear()は呼ばなくて良い。
	void stop();
	void clear();
	void recordSplit(); 	// スプリットタイム記録

//private:
	template<class DURATION>
    std::vector<rep_type> splitInvervals()
	{
		namespace stc = std::chrono;
		// とりあえずcountが呼ばれた時点を記録し、stop()が呼ばれていなければそれを使う
		auto tmpTp = stc::system_clock::now();
		if(stopTp_ == Tp())	stopTp_ = tmpTp;

        std::vector<rep_type> retVec;
		for(size_t i = 0; i < splits_.size(); ++i) {
			if(i == 0) {
				retVec.emplace_back(stc::duration_cast<DURATION>(splits_.at(i) - startTp_).count());
            } /*else if (i == splits_.size() -1) {
				retVec.emplace_back(stc::duration_cast<DURATION>(stopTp_ - splits_.at(i)).count());
            } */else {
				retVec.emplace_back(stc::duration_cast<DURATION>(splits_.at(i) - splits_.at(i-1)).count());
			}
		}
		return retVec;
	}

public:
    inline std::vector<rep_type> splitIntervalsMSec() {return splitInvervals<std::chrono::milliseconds>();}
    inline std::vector<rep_type> splitIntervalsMuSec() {return splitInvervals<std::chrono::microseconds>();}
    inline std::vector<rep_type> splitIntervalsSec() {return splitInvervals<std::chrono::seconds>();}


	// スタートからストップまでの秒数をduration_castして取得
    template<class DURATION> auto count()
	{
		namespace stc = std::chrono;
		// とりあえずcountが呼ばれた時点を記録し、stop()が呼ばれていなければそれを使う
		auto tmpTp = stc::system_clock::now();
		if(stopTp_ == Tp())	stopTp_ = tmpTp;

		// startせずにcountを呼んだらassert発動
		assert(startTp_ != Tp());

		auto countValue = stc::duration_cast<DURATION>(stopTp_ - startTp_).count();
		return countValue;
	}
    inline auto msec(){return count<std::chrono::milliseconds>();}
    inline auto musec(){return count<std::chrono::microseconds>();}
    inline auto sec(){return count<std::chrono::seconds>();}

private:
	using Tp = std::chrono::system_clock::time_point;
	std::vector<Tp> splits_;
	Tp startTp_;
	Tp stopTp_;
};
}  // end namespace tm

#endif // TIMER_HPP
