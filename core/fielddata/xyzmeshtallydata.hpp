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
#ifndef XYZMESHTALLYDATA_H
#define XYZMESHTALLYDATA_H

#include <array>
#include <memory>
#include <vector>
#include <string>

namespace fd {

// 直交xyzメッシュタリーの結果を格納するクラス。
class XyzMeshTallyData {
public:
    static std::shared_ptr<const XyzMeshTallyData> createXyzMeshTallyData(const std::string &fileName);

    XyzMeshTallyData(){}
    XyzMeshTallyData(std::vector<double> &&xg, std::vector<double> &&yg, std::vector<double> &&zg,
                     std::vector<double> &&data, std::vector<double> &&rErrs);
	const std::vector<double> &xgrid() const {return grid_[0];}
	const std::vector<double> &ygrid() const {return grid_[1];}
	const std::vector<double> &zgrid() const {return grid_[2];}
    const std::vector<double> &data() const {return data_;}
    const std::vector<double> &rerr() const {return rerrs_;}
    double getValue(double x, double y, double z, bool enableInterpolation, bool isLog) const;
	double getMin() const {return minValue_;}
	double getMax() const {return maxValue_;}
    void clear();
    std::string info() const;
    bool valid() const;
private:
	// grid_[0]がxgrid
	std::array<std::vector<double>, 3> grid_, gridCenter_;
    std::vector<double> data_;
    std::vector<double> rerrs_;
	double maxValue_, minValue_;

	// xyz方向のgrid負側のindex(=メッシュのインデックス)からdata_のindexを取得
	//inline
	size_t getElementIndex(size_t xindex, size_t yindex, size_t zindex) const;
	//inline
	size_t getElementIndex(const std::array<size_t, 3> &indices) const;
};

}


#endif // XYZMESHTALLYDATA_H
