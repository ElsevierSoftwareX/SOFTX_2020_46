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
#include "time_utils.hpp"


utils::SimpleTimer::SimpleTimer(size_t numReserve)
{
	splits_.reserve(numReserve);
	startTp_ = std::chrono::system_clock::now();
}

void utils::SimpleTimer::start()
{
	stopTp_ = Tp();
	auto cap = splits_.capacity();
	splits_.clear();
	splits_.reserve(cap);
	startTp_ = std::chrono::system_clock::now();
}


void utils::SimpleTimer::stop()
{
	stopTp_ = std::chrono::system_clock::now();
	// startせずにcountを呼んだらassert発動
	assert(startTp_ != Tp());
}

void utils::SimpleTimer::clear()
{
	startTp_ = Tp();
	stopTp_ = Tp();
	splits_.clear();
}

void utils::SimpleTimer::recordSplit()
{
	splits_.emplace_back(std::chrono::system_clock::now());
}
