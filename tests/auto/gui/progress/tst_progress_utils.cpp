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
#include <QtTest>
#include <QCoreApplication>

// add necessary includes here

#include <iostream>
#include <exception>
#include <string>
#include <thread>
#include "core/utils/progress_utils.hpp"
#include "core/utils/workerinterface.hpp"
class progress_utils : public QObject
{
	Q_OBJECT

public:
	progress_utils();
	~progress_utils();

private slots:
	void initTestCase();
	void cleanupTestCase();
	void test_case1();

};


void progress_utils::initTestCase()
{

}

void progress_utils::cleanupTestCase()
{

}


progress_utils::progress_utils() {}
progress_utils::~progress_utils() {}




template<> struct WorkerTypeTraits<class TestWorker> {
  typedef double result_type;
};

class TestWorker : WorkerInterface<TestWorker> {
public:
	typedef WorkerTypeTraits<TestWorker>::result_type result_type;
	explicit TestWorker(int a, double b, std::string c)
		:a_(a), b_(b), c_(c){

	}

    void operator()(std::atomic_size_t *counter,
                    std::atomic_bool *stopFlag,
					size_t threadNumber,
					size_t startIndex, size_t endIndex,
					result_type *result,
                    std::exception_ptr *ep,
                    bool quiet)
	{
		Q_UNUSED(threadNumber);
		Q_UNUSED(startIndex);
		Q_UNUSED(endIndex);
		Q_UNUSED(ep);
        Q_UNUSED(quiet);

		try {
			if(stopFlag->load()) return;
			std::cout << "a, b, c===" << a_ << ", " << b_ << ", " << c_ << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
			*result = 0;
			++(*counter);
		} catch (...) {
			*ep = std::current_exception();
		}
	}

	static result_type collect(std::vector<result_type> *results)
	{
		result_type retval = 0;
		for(const auto& val: *results) {
			retval += val;
		}
		return retval;
	}
	static OperationInfo info() {
		OperationInfo retinfo(
					"dialot title!!!",
					"operating!!!",
					"waiting cancel",
					"this is a cancel button"
					);
		return retinfo;
	}

private:
	int a_;
	double b_;
	std::string c_;
};


void progress_utils::test_case1()
{
//	std::vector<typename TestWorker::result_type> resVec;
//	std::exception_ptr ep;
//	std::atomic_size_t counter(0);
//	std::thread th(TestWorker(1,2,"aaa"), &counter, size_t(0), size_t(1), &resVec, &ep);
//	th.join();
//	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	OperationInfo info = TestWorker::info();
	info.dotLength = 10;
	info.sleepMsec = 200;
	info.numThreads = 3;
	info.numTargets = 2;
	TestWorker::result_type results = ProceedOperation<TestWorker>(info, 1, 2, "aaa");
	mDebug() << "\nresults===" << results;
}



QTEST_MAIN(progress_utils)

#include "tst_progress_utils.moc"
