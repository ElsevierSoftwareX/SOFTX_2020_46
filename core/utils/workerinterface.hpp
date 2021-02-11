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
#ifndef WORKERINTERFACE_HPP
#define WORKERINTERFACE_HPP

#include <atomic>
#include <stdexcept>
#include <string>
#include <thread>

#include "message.hpp"

// exception_ptrからwhatを取得する
std::string what(const std::exception_ptr &eptr);



/*
 * Workerファンクタに求められる条件(コンパイル時チェックされる)
 * ・operator()の引数は
 *		第1引数は処理したデータ数をカウントするatomic_size_tへのポインタ
 *      第2引数は停止制御用のboolフラグ
 *      第3引数はスレッドの通し番号
 *		第4引数は(スレッドの処理する)最小インデックス番号(size_t)
 *		第5引数は(スレッドの処理する)最大インデックス番号＋1(size_t)
 *		第6引数は結果回収コンテナへのポインタ(*result_type)
 *		第7引数は例外へのポインタ(*exception_ptr)
 * ・typedefでresult_typeを持つ
 * ・vector<result_type>を集計してresult_typeにするcollect関数を持つ
 *
 * ・WorkerはこのWorkerをtemplate引数としたインターフェイスクラスを継承・実装し、(CRTP)
 * ・スレッドが作成する結果の型をtemplate<> WorkerTypeTraits<Worker>::result_typeを定義する。
 *
 * 1．WorkerInterface::operator()内のimpl_operator()に合うような場合はDerivedでimpl_operator(size_t, int, result_type*)を定義しCRTPを使う
 * 2．そうでない場合はDerived::operator()とWorkerTypeTraits<Derived>::result_typeを定義する。
 *
 * 実体化されるのはどちらか一方だけなのでどちらかが定義できていれば良い。
 */
template<typename T> struct WorkerTypeTraits;

template <class Derived>
class WorkerInterface {
public:
	// 派生クラスのresult_typeを取得するためWorkerTypeTraitsを使っている。
	typedef typename WorkerTypeTraits<Derived>::result_type result_type;
//	using result_type = typename Derived::result_type;  // ここの時点ではDerivedはまだincompleteなので使えない

	// Derivedクラスではoperator()ではなく impl_operation()を実装し、
	// Derived::operator()が呼ばれた場合は基底クラスのoperator()↓が呼ばれる。
	// するとそこの中でDerived::operator()が呼ばれる
	void operator()(std::atomic_size_t *counter,
					std::atomic_bool *stopFlag,
					size_t threadNumber,
					std::size_t startIndex,
					std::size_t endIndex,
					result_type *results,
                    std::exception_ptr *exceptionPtr,
                    bool quiet = false)
	{
		assert(startIndex <= endIndex);
		auto tID = std::this_thread::get_id();
//		mDebug() << "Operatior() started, tID===" << tID
//				 << "start, endindex===" << startIndex << endIndex;

		size_t localCounter = 0;  // スレッドごとのカウンタ。counterはグローバルなカウンタ
		// i=startIndex から endIndex-1 まで
		try {
			for(size_t i = startIndex; i < endIndex; ++i) {

				if(stopFlag->load()) {
					mWarning() << "Thread " << threadNumber << "canceled.";
					// stopFlagがtrueの場合はProgressReceiverの方でキャンセルされているから
					// 本来counterの帳尻合わせは必要ないが念の為。
					std::atomic_fetch_add(counter, endIndex - startIndex - localCounter);
					break;
				}
				// ここが処理の本体。i 番目の処理を実行。
				static_cast<Derived*>(this)->impl_operation(i, threadNumber, results);


				++localCounter;
				(*counter)++;
			}
		} catch (...) {
			/*
				 * 例外が発生したら
				 * 0. 他のスレッドを止めるために、ストップフラグをtrueにする。
				 * 1. メインスレッドに伝えるためにepに代入
				 * 2．counterを残り処理分だけ増加（counterを監視しているprogressが正常にループを抜けるように。ループを抜けないとjoinできない）
				 * 3．リターン
				 */
			// counterの帳尻合わせ. localCounter個の処理が終わったところでエラー発生したので、
			// 残りの数だけcounterを増やして帳尻を合わせる。
			stopFlag->store(true);
			std::atomic_fetch_add(counter, endIndex - startIndex - localCounter);
            *exceptionPtr = std::current_exception();
            if(!quiet) mWarning() << "Worker exception. tID=" << tID << "wwhat=" << what(*exceptionPtr);
			return;
		}
        if(!quiet) mDebug() << "worker operator() ended. tID =" << std::this_thread::get_id();
	}
};







#endif // WORKERINTERFACE_HPP
