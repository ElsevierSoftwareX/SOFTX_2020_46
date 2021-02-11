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
#include "xyzmeshtallydata.hpp"

#include <algorithm>
#include <bitset>
#include <limits>
#include <fstream>
#include <sstream>
#include <vector>

#include "core/utils/container_utils.hpp"
#include "core/utils/system_utils.hpp"
#include "core/utils/string_utils.hpp"


namespace {

const double MIN_VALUE = 1e-30;
const double NEAREST_DISTANCE = 1e-15;

}  // end anonymous namespace


std::shared_ptr<const fd::XyzMeshTallyData>
fd::XyzMeshTallyData::createXyzMeshTallyData(const std::string &fileName)
{
    std::ifstream ifs(utils::utf8ToSystemEncoding(fileName.c_str()));
    if(ifs.fail()) 	throw std::invalid_argument(std::string("No such a file = ") + fileName);
    static const std::regex xgridReg(R"(X *direction:([-+.0-9 ]+))", std::regex_constants::icase);
    static const std::regex ygridReg(R"(Y *direction:([-+.0-9 ]+))", std::regex_constants::icase);
    static const std::regex zgridReg(R"(Z *direction:([-+.0-9 ]+))", std::regex_constants::icase);
    static const std::regex headerReg(R"(^ *X +Y +Z +Result +Rel *Error)", std::regex_constants::icase);
    static const std::regex headerEReg(R"(^ *energy *X +Y +Z +Result +Rel *Error)", std::regex_constants::icase);
    std::string buff;
    size_t line = 0;

    std::smatch sm;
    while(++line, std::getline(ifs, buff), !std::regex_search(buff, sm, xgridReg)) {
            if(ifs.eof()) throw std::invalid_argument(std::string("While seeking x grid metadata, unexpected EOF, ") +
                                 fileName + ":" + std::to_string(line));
    }
    std::vector<double> xgrid = utils::stringVectorTo<double>(utils::splitString(" ", sm.str(1), true));

    while(++line, std::getline(ifs, buff), !std::regex_search(buff, sm,  ygridReg)) {
            if(ifs.eof()) throw std::invalid_argument(std::string("While seeking x grid metadata, unexpected EOF, ") +
                                 fileName + ":" + std::to_string(line));
    }
    std::vector<double> ygrid = utils::stringVectorTo<double>(utils::splitString(" ", sm.str(1), true));

    while(++line, std::getline(ifs, buff), !std::regex_search(buff, sm,  zgridReg)) {
            if(ifs.eof()) throw std::invalid_argument(std::string("While seeking x grid metadata, unexpected EOF, ") +
                                 fileName + ":" + std::to_string(line));
    }
    std::vector<double> zgrid = utils::stringVectorTo<double>(utils::splitString(" ", sm.str(1), true));

    while(++line, std::getline(ifs, buff),
          !(std::regex_search(buff, headerReg) || std::regex_search(buff, headerEReg))) {
        if(ifs.eof()) throw std::invalid_argument("While seeking meshtal header, unexpected EOF, line=" + std::to_string(line));
    }
    size_t dataIndex = 3, rerrIndex = 4;
    if(std::regex_search(buff, headerEReg)) {
        ++dataIndex;
        ++rerrIndex;
    }

    std::vector<double> dvec, data, rerrs;
    const size_t numData = (xgrid.size()-1)*(ygrid.size()-1)*(zgrid.size()-1);
    data.reserve(numData);
    rerrs.reserve(numData);
    // ここから具体的なデータが始まる
    for(size_t i = 0; i < numData; ++i) {
        std::getline(ifs, buff);
        utils::sanitizeCR(&buff);
        //mDebug() << "buf===" << buff;
        ++line;
        if(ifs.eof()) throw std::invalid_argument("While reading meshtal data, unexpected EOF, line=" + std::to_string(line));
        dvec = utils::stringVectorTo<double>(utils::splitString(" ", buff, true));
        data.emplace_back(dvec.at(dataIndex));
        rerrs.emplace_back(dvec.at(rerrIndex));
    }


    return std::make_shared<const fd::XyzMeshTallyData>(std::move(xgrid),
                                                        std::move(ygrid),
                                                        std::move(zgrid),
                                                        std::move(data),
                                                        std::move(rerrs));
}

fd::XyzMeshTallyData::XyzMeshTallyData(std::vector<double> &&xg, std::vector<double> &&yg, std::vector<double> &&zg,
                                   std::vector<double> &&data, std::vector<double> &&rErrs)
	:data_(data), rerrs_(rErrs)
{
	grid_[0] = xg;
	grid_[1] = yg;
	grid_[2] = zg;
    std::string errName;
	if(!utils::isAscendant(xg)) {
        errName = "xgrid";
	} else if(!utils::isAscendant(yg)) {
        errName = "ygrid";
	} else if(!utils::isAscendant(zg)) {
        errName = "zgrid";
    }
    if(!errName.empty()) {
        throw std::invalid_argument(errName + " data should be ascendant.");
    }

	// グリッドの中央位置もコンストラクタで計算しておく。
	for(size_t i = 0; i < 3; ++i) {
		gridCenter_[i].resize(grid_[i].size()-1);
		for(size_t j = 0; j < grid_[i].size()-1; ++j) {
			gridCenter_[i].at(j) = 0.5*(grid_[i].at(j) + grid_[i].at(j+1));
		}
	}
	// 最大最小も。
	minValue_ = *std::min_element(data_.begin(), data_.end());
	maxValue_ = *std::max_element(data_.begin(), data_.end());
}

double getDist(const std::array<std::vector<double>, 3> &gridCenter,
			   const std::array<size_t, 3> &indices,
			   double x, double y, double z)
{
	return
			std::sqrt(
			  (gridCenter[0].at(indices[0]) - x)*(gridCenter[0].at(indices[0]) - x)
			+ (gridCenter[1].at(indices[1]) - y)*(gridCenter[1].at(indices[1]) - y)
			+ (gridCenter[2].at(indices[2]) - z)*(gridCenter[2].at(indices[2]) - z));
}

double getDistProd(const std::array<std::vector<double>, 3> &gridCenter,
				   const std::array<size_t, 3> &indices,
				   double x, double y, double z, bool debugFlag = false)
{
	if(debugFlag) {
		mDebug() << "gridCenter ==="
				 << gridCenter[0].at(indices[0])
				<< gridCenter[1].at(indices[1])
				<< gridCenter[2].at(indices[2]);
		mDebug() << "element ==="
				 << (gridCenter[0].at(indices[0]) - x)
				<< (gridCenter[1].at(indices[1]) - y)
				<< (gridCenter[2].at(indices[2]) - z);
	}
    return
			std::abs( (gridCenter[0].at(indices[0]) - x)
			*(gridCenter[1].at(indices[1]) - y)
			*(gridCenter[2].at(indices[2]) - z));
}


double fd::XyzMeshTallyData::getValue(double x, double y, double z, bool enableInterpolation, bool isLog) const
{

    assert(data_.size() == rerrs_.size());
	assert(data_.size() == (grid_[0].size()-1)*(grid_[1].size()-1)*(grid_[2].size()-1));
	assert(!data_.empty() && !grid_[0].empty() && !grid_[1].empty() && !grid_[2].empty());

	// グリッド領域外判定
	// グリッド上はグリッド外と判定する。とloser_bound結果とかの取扱が楽。
	// 但しグリッド境界と面が位置するとそこのデータがなくなるので少し見栄えは悪くなる。
//	if(x < grid_[0].front() || y < grid_[1].front() || z < grid_[2].front()) return MIN_VALUE;
//	if(x > grid_[0].back() || y > grid_[1].back() || z > grid_[2].back()) return MIN_VALUE;
    if(x < grid_[0].front() || y < grid_[1].front() || z < grid_[2].front()) return std::numeric_limits<double>::quiet_NaN();
    if(x > grid_[0].back() || y > grid_[1].back() || z > grid_[2].back()) return std::numeric_limits<double>::quiet_NaN();

	/*
	 *  二分探索でx,y,zインデックスを求める。
	 * が、多分thread_localに前回のindexをキャッシュしておいて、
	 * そこから線形探索したほうが多分速い
	 */

	// indexはdistanceから1引く(グリッド境界に一致しなければ)
	size_t xindex = std::distance(grid_[0].begin(), std::lower_bound(grid_[0].begin(), grid_[0].end(), x));
	size_t yindex = std::distance(grid_[1].begin(), std::lower_bound(grid_[1].begin(), grid_[1].end(), y));
	size_t zindex = std::distance(grid_[2].begin(), std::lower_bound(grid_[2].begin(), grid_[2].end(), z));
	if(grid_[0].front() != x) xindex -= 1;
	if(grid_[1].front() != y) yindex -= 1;
	if(grid_[2].front() != z) zindex -= 1;
    size_t index0 = getElementIndex(xindex, yindex, zindex);
	// 補間しないならここで終わり
	if(!enableInterpolation) return data_.at(index0);

	int dx = (x > gridCenter_[0].at(xindex)) ? 1 : -1;
	int dy = (y > gridCenter_[1].at(yindex)) ? 1 : -1;
	int dz = (z > gridCenter_[2].at(zindex)) ? 1 : -1;
	std::array<std::array<size_t, 3>, 8> indices {
		std::array<size_t, 3>{xindex,    yindex,    zindex},
		std::array<size_t, 3>{xindex+dx, yindex,    zindex},
		std::array<size_t, 3>{xindex,    yindex+dy, zindex},
		std::array<size_t, 3>{xindex+dx, yindex+dy, zindex},
		std::array<size_t, 3>{xindex,    yindex,    zindex+dz},
		std::array<size_t, 3>{xindex+dx, yindex,    zindex+dz},
		std::array<size_t, 3>{xindex,    yindex+dy, zindex+dz},
		std::array<size_t, 3>{xindex+dx, yindex+dy, zindex+dz}
	};
	// もろメッシュ中心上ならそのままリターン
	double dist0 = getDist(gridCenter_, indices[0], x, y, z);
	if(dist0 < NEAREST_DISTANCE) return data_.at(index0);

    bool hasXneighbor = !(xindex == 0 && dx < 0) && !(xindex == gridCenter_[0].size()-1 && dx > 0);
    bool hasYneighbor = !(yindex == 0 && dy < 0) && !(yindex == gridCenter_[1].size()-1 && dy > 0);
    bool hasZneighbor = !(zindex == 0 && dz < 0) && !(zindex == gridCenter_[2].size()-1 && dz > 0);
	std::bitset<3> hasNeighbor;
	const size_t X=0, Y=1, Z=2;
	hasNeighbor[X] = hasXneighbor;
	hasNeighbor[Y] = hasYneighbor;
	hasNeighbor[Z] = hasZneighbor;

	/*
	 * ここで
	 * 1．3方向何れにも隣接データない→ 所属グリッドのデータを返す。
	 * 2．1方向だけにデータがある →
	 * 3．2方向にデータがある
	 * 4．3方向にデータがある
	 * の4通りが考えられる。
	 */
	std::vector<std::pair<size_t, size_t>> counterIndices;
	std::array<double, 8> distProds{-1, -1, -1, -1, -1, -1, -1, -1}, values{0, 0, 0, 0, 0, 0, 0, 0};
	if(hasNeighbor.count() == 0) {
		// 隣接なしの孤立
		return data_.at(index0);
	} else if(hasNeighbor.count() == 1) {
		// 1次元
		counterIndices.resize(1);
		if(hasNeighbor[X]) {
			counterIndices.at(0) = std::make_pair(0, 1);
		} else if (hasNeighbor[Y]) {
			counterIndices.at(0) = std::make_pair(0, 2);
		} else {
			counterIndices.at(0) = std::make_pair(0, 4);
		}
	} else if(hasNeighbor.count() == 2) {
		// 2次元 xy xz yzの三通り
		counterIndices.resize(2);
		if(hasNeighbor[X] && hasNeighbor[Y]) {
			counterIndices.at(0) = std::make_pair(0, 3);
			counterIndices.at(1) = std::make_pair(1, 2);
		} else if(hasNeighbor[X] && hasNeighbor[Z]) {
			counterIndices.at(0) = std::make_pair(0, 5);
			counterIndices.at(1) = std::make_pair(1, 4);
		} else {
			counterIndices.at(0) = std::make_pair(0, 6);
			counterIndices.at(1) = std::make_pair(2, 4);
		}
	} else {
		// 3次元
		counterIndices.resize(4);
		counterIndices.at(0) = std::make_pair(0, 7);
		counterIndices.at(1) = std::make_pair(1, 6);
		counterIndices.at(2) = std::make_pair(2, 5);
		counterIndices.at(3) = std::make_pair(3, 4);
	}


	for(const auto &p: counterIndices) {
		distProds[p.first] = getDistProd(gridCenter_, indices[p.second], x, y, z);
        values[p.first] = data_.at(getElementIndex(indices[p.first]));
		distProds[p.second] = getDistProd(gridCenter_, indices[p.first], x, y, z);
		values[p.second] = data_.at(getElementIndex(indices[p.second]));
        if(isLog) {

            if(std::abs(values[p.first]) < MIN_VALUE) values[p.first] = MIN_VALUE;
            if(std::abs(values[p.second]) < MIN_VALUE) values[p.second] = MIN_VALUE;
            values[p.first] = std::log(values[p.first]);
            values[p.second] = std::log(values[p.second]);
            //mDebug() << "logvalue,=====" << values[p.first];


        }
	}
	double retval = 0, wtot = 0;
	for(size_t i = 0; i < distProds.size(); ++i) {
		if(distProds[i] >= 0) {
			retval += distProds[i]*values[i];
			wtot += distProds[i];
		}
	}
    //mDebug() << "retval, expretval ===" << retval/wtot << std::exp(retval/wtot);
    return isLog ? std::exp(retval/wtot) : retval/wtot;
}

void fd::XyzMeshTallyData::clear()
{
	for(auto &gr: grid_) {
		gr.clear();
	}
    data_.clear();
    rerrs_.clear();
}

std::string fd::XyzMeshTallyData::info() const
{
    if(!valid()) return "";
	static const std::vector<std::string> xyz{"x", "y", "z"};
    std::stringstream ss;
	for(size_t i = 0; i < 3; ++i) {
		ss << xyz[i] << " = " << grid_[i].front() << "/" << grid_[i].back() << ", n = " << grid_[i].size();
		if(i != 2) ss << std::endl;
	}
    return ss.str();
}

bool fd::XyzMeshTallyData::valid() const
{
	return !(grid_[0].empty() || grid_[1].empty() || grid_[2].empty() || data_.empty() || rerrs_.empty());
}

size_t fd::XyzMeshTallyData::getElementIndex(size_t xindex, size_t yindex, size_t zindex) const
{
	return xindex*(grid_[1].size()-1)*(grid_[2].size()-1) + yindex*(grid_[2].size()-1) + zindex;
}

size_t fd::XyzMeshTallyData::getElementIndex(const std::array<size_t, 3> &indices) const
{
	return indices[0]*(grid_[1].size()-1)*(grid_[2].size()-1) + indices[1]*(grid_[2].size()-1) + indices[2];
}

