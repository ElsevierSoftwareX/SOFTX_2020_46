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
#include "container_utils.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "core/utils/numeric_utils.hpp"
#include "core/io/input/dataline.hpp"

std::vector<double> utils::normalize(const std::vector<double> &vec)
{
	std::vector<double> retVec = vec;
	double total = std::accumulate(vec.begin(), vec.end(), 0.0);

	if(utils::isSameDouble(total, 0)) {
		throw std::invalid_argument("Total value are zero.");
	}
	if(!utils::isSameDouble(total, 1)) {
		for(auto &elem: retVec) {
			elem /= total;
		}
	}
	return retVec;
}

std::list<inp::DataLine>::const_iterator utils::findDataLine(const std::list<inp::DataLine> &inputList, const std::string targetStr)
{
	auto it = inputList.begin();
	for( ;it != inputList.end(); ++it) {
		if(it->data.find(targetStr) != std::string::npos) return it;
	}
	return it;
}

std::vector<std::list<inp::DataLine>>
	utils::splitDataLineListByTitle(const std::list<inp::DataLine> &dataLines,
							 const std::regex &titlePattern, const std::regex &excludePattern)
{
    std::vector<std::list<inp::DataLine>> retVec;
    std::smatch sm;
    std::list<inp::DataLine> tmpList;
    bool hasFoundTitle = false;
    for(auto &dl: dataLines) {
		if(std::regex_search(dl.data, sm, titlePattern) && !std::regex_search(dl.data, sm, excludePattern)) {
            if(hasFoundTitle) {
                // すでに一度パターンを発見している場合、
                // 再度パターンが出現したらそこで一旦区切りなのでtmpListをpush_backしてクリアする。
                retVec.emplace_back(tmpList);
                tmpList.clear();
                tmpList.emplace_back(dl);

            } else {
                // 初めてパターンを発見した場合、フラグをtrueにして
                // パターン部分もtmpListに追加する。
                hasFoundTitle = true;
                tmpList.emplace_back(dl);
            }
        } else {
            if(hasFoundTitle) tmpList.emplace_back(dl);
        }

    }
	if(!tmpList.empty()) retVec.emplace_back(tmpList);
	// 一度も区切りが見つからなかったら全体を返す
	if(retVec.empty()) retVec.emplace_back(dataLines);

    return retVec;
}
