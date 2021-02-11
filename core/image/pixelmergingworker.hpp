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
#ifndef PIXELMERGINGWORKER_HPP
#define PIXELMERGINGWORKER_HPP

#include <atomic>
#include "pixelarray.hpp"
#include "core/utils/progress_utils.hpp"
#include "core/utils/workerinterface.hpp"


template<> struct WorkerTypeTraits<class PixelMergingWorker> {
  typedef img::PixelArray result_type;
};

class PixelMergingWorker: public WorkerInterface<PixelMergingWorker>
{
public:
	typedef WorkerTypeTraits<PixelMergingWorker>::result_type result_type;
	static OperationInfo info();
	static result_type collect(std::vector<result_type> *results);

	PixelMergingWorker(const img::PixelArray &arr1,
					   const img::PixelArray &arr2,
					   const std::vector<img::PixelArray::pixel_type> &priorPattern, // 画素の優先度を定義するvector
					   const img::PixelArray::pixel_type &conflictedPixel)
		:srcArr1_(arr1), srcArr2_(arr2), priorPattern_(priorPattern), conflictedPixel_(conflictedPixel)
	{;}

	// pixelのマージは一次元vectorのマージと異なるので、
	// void impl_operationではなくoperator()を実装する。(CRTPしない)
	void operator()(std::atomic_size_t *counter, std::atomic_bool *stopFlag,
					size_t threadNumber,
					size_t startIndex, size_t endIndex,
					result_type *resultPixelArray,
					std::exception_ptr *ep)
	{
        (void) threadNumber;
		size_t localCounter = 0;
		try {

			mDebug() << "threadNumber===" << threadNumber << "is, ie===" << startIndex << endIndex;
			// 2次元アクセスしていると速度が出ないので1次元アクセサを使う。
			size_t localVsize = srcArr1_.verticalSize();
			size_t localHsize = (endIndex-startIndex)/localVsize;
			resultPixelArray->resize(localHsize, localVsize);
			img::PixelArray::pixel_type val1, val2;
			bool isPriorPattern;
			for(size_t i = startIndex; i < endIndex; ++i) {
				val1 = srcArr1_.at(i);
				val2 = srcArr2_.at(i);
				if(val1 == val2) {
					resultPixelArray->at(i-startIndex) = val1;
				} else {
					isPriorPattern = false;
					// priorPattern内では先頭に近いほうが優先されるのでループは後ろから回す。
					for(auto it = priorPattern_.rbegin(); it != priorPattern_.rend(); ++it) {
						if(val1 == *it || val2 == *it) {
							resultPixelArray->at(i-startIndex) = *it;
							isPriorPattern = true;
							break;
						}
					}
					if(!isPriorPattern)  {
						resultPixelArray->at(i-startIndex) = conflictedPixel_;
					}
				}
				++localCounter;
				++(*counter);
				if(stopFlag->load()) return;
			}

//			// 処理するvsizeはsrcArr1_.verticalsize()
//			// 処理するhsizeはendIndex-startIndex/vsize
//            size_t localVsize = srcArr1_.verticalSize();
//            size_t localHsize = (endIndex-startIndex)/localVsize;
//            resultPixelArray->resize(localHsize, localVsize);
//			img::PixelArray::pixel_type val1, val2;
//			bool isPriorPattern;


//			// PixelArrayはcolumn-majorなので列方向に連結するのが楽。よってhorizontal方向に分割する。
//            size_t globalHindex;
//            for(size_t hindex = 0; hindex < localHsize; ++hindex) {
//                globalHindex = hindex + startIndex/srcArr1_.verticalSize();
//                for(size_t vindex = 0; vindex < localVsize; ++vindex) {
//					//mDebug() << "arraysize=" << parray1.horizontalSize() << parray1.verticalSize() << "i,j=" <<i << j;

//					// 注意！！srcArr1,2は画像全体のピクセル配列でvindex,hindexも全体の中のインデックス番号
//                    val1 = srcArr1_(globalHindex, vindex);
//                    val2 = srcArr2_(globalHindex, vindex);
//                    //mDebug() << "ghi, hi, vi, val1, val2===" << globalHindex << hindex << vindex << val1 << val2;
//					/*
//                     * 領域データが矛盾する→未定義領域
//                     * 境界データと領域データが食い違う → 境界を優先
//                     * 領域データと領域データが食い違う → 未定義セル
//                     */
//                    if(val1 == val2) {
//                        (*resultPixelArray)(hindex, vindex) = val1;
//                    } else {
//						isPriorPattern = false;
//						// priorPattern内では先頭に近いほうが優先されるのでループは後ろから回す。
//						for(auto it = priorPattern_.rbegin(); it != priorPattern_.rend(); ++it) {
//							if(val1 == *it || val2 == *it) {
//                                (*resultPixelArray)(hindex, vindex) = *it;
//								isPriorPattern = true;
//								break;
//							}
//						}
//						if(!isPriorPattern)  {
//                            (*resultPixelArray)(hindex, vindex) = conflictedPixel_;
//						}
//					}
//                    ++(*counter);
//                    if(stopFlag->load()) return;
//				}
//			}
		} catch (...) {
			*ep = std::current_exception();
			// 例外が発生したら残りのループ回数分だけカウンタを増加させる。
			std::atomic_fetch_add(counter, endIndex - startIndex - localCounter);

		}
	}

private:
	const img::PixelArray &srcArr1_;
	const img::PixelArray &srcArr2_;
	const std::vector<img::PixelArray::pixel_type> &priorPattern_;
	const img::PixelArray::pixel_type &conflictedPixel_;
};

#endif // PIXELMERGINGWORKER_HPP
