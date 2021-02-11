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
#ifndef PROGRESS_UTILS_HPP
#define PROGRESS_UTILS_HPP

#include <algorithm>
#include <chrono>
#include <exception>
#include <future>
#include <string>
#include <thread>
#include <vector>


#include "message.hpp"

#ifdef ENABLE_GUI
#include "gui/subdialog/messagebox.hpp"
#endif


// exception_ptrからwhatを取得する
std::string what(const std::exception_ptr &eptr);

// コンテナ型 にvectorの内容を集約する
// CONTAINER型はbegin,endが使えてmove_iteratorが使えることが条件になる。
// 大体CONTAINER型もvector
template <class CONTAINER>
CONTAINER collectVector(std::vector<CONTAINER> *resultVec)
{
	CONTAINER results;
	// 結果の回収.move_iteratorを使う。
    std::size_t size = 0;
    for(auto &eachResult: *resultVec) {
        size += eachResult.size();
    }
    results.reserve(size);
	for(auto &eachResult: *resultVec) {
		if(!eachResult.empty()) {
			results.insert(results.end(),
						   std::make_move_iterator(eachResult.begin()),
						   std::make_move_iterator(eachResult.end()));
		}
	}
	return results;
}

// progress表示する情報
/*
 * waitingOperationTextが空ならば進行状況を表示しない。
 * progressFileが指定されれば底へ現在の進行状況を0-1で書き込む
 *
 */
struct OperationInfo{
	std::string dialogTitle;      // 進捗ダイアログのタイトル
	std::string waitingOperationText;  // 実行中に表示するテキスト
	std::string waitingCancelText;  // キャンセル終了待機中に表示するテキスト
	std::string cancelButtonLabel;  // キャンセルボタンテキスト
    std::string progressFile;       // ファイル名を指定するとそこに現在の進捗割合を書き込む
	std::size_t numTargets;  // 処理対象オブジェクトの数
	std::size_t numThreads;
	std::size_t dotLength; // progress 表示中の....の長さ
	std::size_t sleepMsec;  // スピン中の待ち時間(ms)
    bool quiet;  // 出力を抑制

	OperationInfo() {;}
	OperationInfo(const std::string &title,
				  const std::string& operatingText,
				  const std::string& cancelingText,
				  const std::string &cancelLabel)
		: dialogTitle(title),
		  waitingOperationText(operatingText),
		  waitingCancelText(cancelingText),
		  cancelButtonLabel(cancelLabel),
		  // 以下の値は実行時に決めるもので型情報からは決まらず、どうせ個別にメンバアクセスして変更するから
          // コンストラクタではデフォルト値しか設定しない。
        numTargets(0), numThreads(1), dotLength(6), sleepMsec(0), quiet(false)
	{;}
};


#ifdef ENABLE_GUI
#include <QProgressDialog>
#endif

class ProgressReceiver
{
public:
	ProgressReceiver(const std::string &title,
                     const std::string &cancelLabel,
                     int numTargets,
                     const std::string &progressFile);
	void setValue(int count);
	void setLabelText(const std::string &str);
	bool wasCanceled() const;
	void close();
	void processEvents();
	void update();
    void writeProgressFile(double d);
private:
#ifdef ENABLE_GUI
	std::shared_ptr<QProgressDialog> dialog_;
#endif
    const std::string progressFileName_;
    std::ofstream ofs_;
};

/*
 * Workerファンクタに求められる条件(コンパイル時チェックされる)
 * ・operator()の引数は
 *		第1引数は処理したデータ数をカウントするatomic_size_tへのポインタ
 *      第2引数は停止制御用のboolフラグ
 *		第2引数は(スレッドの処理する)最小インデックス番号(size_t)
 *		第3引数は(スレッドの処理する)最大インデックス番号＋1(size_t)
 *		第4引数は結果回収コンテナへのポインタ(*vector<result_type>)
 *		第5引数は例外へのポインタ(*exception_ptr)
 * ・typedefでresult_typeを持つ(result_container_typeはどうせvector使うので決め打ちする。)
 * ・結果集約関数static result_type collect(std::vector<result_type>*);
 */
/*
 * ProceedOperationの引数は
 * 第一引数：OperationInfo
 * 第二引数以降：Workerファンクタのコンストラクタ引数
 */
template <class Worker, typename... Args>
typename Worker::result_type ProceedOperation(OperationInfo info, Args&&... args)
{
	if(info.numTargets == 0) return typename Worker::result_type();
    if(info.numThreads > info.numTargets) {
        if(!info.quiet) mWarning() << "Number of threads > that of targets. Set num threads from"
				   << info.numThreads << "to" << info.numTargets;
		info.numThreads = info.numTargets;
	}

	// デバッグしやすいように info.numThreads==0ならプログレスを出さずにメインスレッドでのシーケンシャル実行にする。
    if(info.numThreads == 0) {
        if(!info.quiet) mDebug() << "numThread == 0, run sequentially in main-thread for debugging purpose.";
		std::atomic_size_t counter0(0);
		std::atomic_bool stopFlag0(false);
		typename Worker::result_type result0;
		std::exception_ptr ep0;
		Worker worker(args...);
        worker(&counter0, &stopFlag0, 0, 0, info.numTargets, &result0, &ep0, info.quiet);
		try{
			if(ep0) std::rethrow_exception(ep0);
		} catch (std::exception &e) {
			// 実際致命的エラーが置きた時のハンドリングはどうするか考える必要がある。
			// 現状ここへ来るのはユーザーインプットではなくプログラムのバグのせいだし。
            throw std::invalid_argument(std::string("ProgramError: ") + e.what() + info.waitingOperationText);
		}
        if(!info.progressFile.empty()) {
            std::ofstream ofs(info.progressFile.c_str());
            if(!ofs) throw std::runtime_error(std::string("File cannot be opened. file =") + info.progressFile);
            ofs << 1 << std::endl;
        }
		return result0;
	}

    ProgressReceiver receiver(info.dialogTitle, info.cancelButtonLabel,
                              info.numTargets, info.progressFile);
	std::atomic_size_t counter(0);
	std::atomic_bool stopFlag(false);
	std::vector<std::thread> workers;
	std::vector<std::exception_ptr> epVec(info.numThreads);
	std::vector<typename Worker::result_type> resultsVector(info.numThreads);
	// スレッド生成
	for(size_t n = 0; n < info.numThreads; ++n) {
		size_t startIndex = info.numTargets/info.numThreads*n + std::min(info.numTargets%info.numThreads, n);
		size_t endIndex = info.numTargets/info.numThreads*(n+1) + std::min(info.numTargets%info.numThreads, n+1);
		workers.emplace_back(
					Worker(args...),
                    &counter, &stopFlag, n, startIndex, endIndex, &(resultsVector.at(n)), &(epVec.at(n)), info.quiet);
		// ここでoperator()の実行開始
	}

	// 進捗中に生きていることを示すdot文字列を生成する。
	std::vector<std::string> dots;
	for(size_t i = 0; i < info.dotLength; ++i) {
		std::string tmpStr(info.dotLength, '.');
		tmpStr.at(i) = ' ';
		dots.emplace_back(tmpStr);
	}

	// ここのループで待つ。
	size_t loop = 0;
    size_t tmpCounter = 0;

	while(counter.load() < info.numTargets) {
		receiver.processEvents();
        receiver.setValue(counter.load());
        if(!info.waitingOperationText.empty()) {
            receiver.setLabelText(info.waitingOperationText + " " + dots.at(++loop%dots.size()) + " "
                                  + std::to_string(counter.load()) + "/" + std::to_string(info.numTargets));
        }
        if(counter.load() != tmpCounter) {
            tmpCounter = counter.load();
            receiver.writeProgressFile(static_cast<double>(tmpCounter)/info.numTargets);
        }
		if(receiver.wasCanceled()) {
			receiver.close();
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(info.sleepMsec));
	}
	// 内部的に100％になっていても表示が100％にならない場合があるので、内部データと表示の整合性を取る。
	if(!receiver.wasCanceled()) {
        receiver.writeProgressFile(1);
        if(!info.waitingOperationText.empty()) {
            receiver.setLabelText(info.waitingOperationText + " " + dots.at(++loop%dots.size()) + " "
                                  + std::to_string(counter.load()) + "/" + std::to_string(info.numTargets));
        }
	}
	// CUIではコンソールの整形をするために一旦改行を入れる
#ifndef ENABLE_GUI
    if(!info.quiet) std::cerr << std::endl;
#endif
	// ここまでで最初のreceiverは一旦閉じる。(cancelされて明示的にcloseされるか進捗100%で自動的に閉じる。)

	/* join/detachの開始。
	 *
	 * メインスレッドでjoinすれば制御が戻らず、その結果GUIが固まってしまう。
	 * detachすれば制御は戻るがなぜかwindows環境ではクラッシュする
	 *
	 * なので結局サブスレッドでjoinして結果は捨てることになる
	 *
	 */

    if(!info.quiet) mDebug() << "Starting join/detach, operation = " << info.waitingOperationText ;
	if(receiver.wasCanceled()){
		// cancelされたらサブスレッドでjoinを待って、以後は結果を回収せずにすぐにリターンする
		// キャンセル用join開始
		stopFlag.store(true);
		std::atomic_int joinCounter{0};
		std::thread joiningThread([&](){
			std::vector<std::future<void>> futures;
			for(auto &worker: workers) {
				futures.emplace_back(std::async(std::launch::async, [&](){worker.join(); joinCounter++;}));
				/*
				 * NOTE detachしたいがdetachしたスレッドが終了したところくらいでクラッシュする場合があった。
				 * デタッチ後にローカル変数とか取るのが駄目だったのかもしれない。
				 * 最も重いsample->update()処理の直後にキャンセルすると問題が発生しなくなった。
				 * しかしこれはデタッチするタイミングによってはクラッシュが起こりうることを示している…。
				 * また、windowsではやはりクラッシュした。ので環境依存性も強そう。不可。
				 */
				//futures.emplace_back(std::async(std::launch::async, [&](){worker.detach(); joinCounter++;}));
			}
			for(auto &fut: futures) fut.get();
		});


		// キャンセル待機withダイアログ
		size_t waitJoinLoopCounter = 0;
        ProgressReceiver cancelReceiver(info.waitingCancelText + dots.at(++waitJoinLoopCounter%dots.size()), "", workers.size(), "");
		while(joinCounter.load() != static_cast<int>(workers.size())) {
			std::this_thread::sleep_for(std::chrono::milliseconds(info.sleepMsec));
			cancelReceiver.processEvents();
			cancelReceiver.setValue(joinCounter.load());
            if(!info.waitingOperationText.empty()) {
                cancelReceiver.setLabelText(info.waitingCancelText + dots.at(++waitJoinLoopCounter%dots.size()));
            }
            cancelReceiver.update();
		}
		// 表示が確実100%になるようにダメ押し。
        if(!info.waitingOperationText.empty()) {
            cancelReceiver.setLabelText(info.waitingCancelText + dots.at(++waitJoinLoopCounter%dots.size()));
        }
        cancelReceiver.close();
		joiningThread.join();
		return  typename Worker::result_type();
	}  // キャンセル処理終わり。

	// ここから通常join
	for(auto &worker: workers) worker.join();
	// 問題はキャンセル後にworkerスレッドで例外が発生した場合。
	//  → workerスレッドの結果は使わずに放棄するから、例外は握りつぶして問題ない。
	// ゆえにjoin後にだけに例外処理すれば良い。
	for(const auto &ep: epVec) {
		try {
            if(ep) throw std::invalid_argument(std::string("While ") +  info.waitingOperationText + what(ep));
		} catch (std::exception &e) {
#ifdef ENABLE_GUI
			GMessageBox::critical(Q_NULLPTR, "critical", QString::fromStdString(e.what()), true);
#else
            (void) e;
			std::rethrow_exception(ep);
#endif
		}
	}
	return Worker::collect(&resultsVector);

}




#endif // PROGRESS_UTILS_HPP
