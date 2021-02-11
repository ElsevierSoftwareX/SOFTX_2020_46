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
#ifndef MESHDATA_HPP
#define MESHDATA_HPP

#include <list>
#include <string>
#include <vector>


namespace inp{
class DataLine;

/*
 * Phitsでのメッシュ定義は5通りある。
 * が、内部表現としては境界値のvectorを保持し、
 * 適時メッシュ中心や幅を生成して返す。
 *
 * 離散分布に対する
 *
 */
class MeshData
{
public:
	// メッシュ中心算出方法の指定 LIN:線形、LOG:対数
	enum class InterPolate{LIN, LOG};
    enum class KIND{A, E, R, X, Y, Z};
	enum class PhitsType{TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, TYPE1m, TYPE2m};
    static std::string typeString(PhitsType meshType);
    static PhitsType strToType(const std::string& str);
    static std::string kindString(KIND kind);
	static KIND strToKind(const std::string &str);
    static MeshData fromPhitsInput(const std::list<inp::DataLine> &input, KIND kind);
	static MeshData fromPhitsInput(std::list<inp::DataLine>::const_iterator &it,
								   const std::list<inp::DataLine> &input,
								   KIND kind, const std::string &typeStr);
    static MeshData fromDiscreteValues(const std::vector<double> &values);

	/*
     * メッシュ生成方法は
	 * １．境界値をvectorあるいはinitialize_listそのまま入力
     * ２．size_t num, pair, 補間法指定で生成
	 *
	 * メッシュ中心は生成時に計算される。
	 * 生成時と異なる中心を補間で求めたくなったら別のメソッドを作る。
     */
    MeshData(){}
	explicit MeshData(const std::vector<double> &vec, InterPolate ipmode = InterPolate::LIN);
	explicit MeshData(const std::initializer_list<double> &dlist, InterPolate ipmode = InterPolate::LIN);
    MeshData(size_t num, const std::pair<double, double> &range, InterPolate interpolate);

    const std::vector<double> &bounds() const {return bounds_;}
	const std::vector<double> &centers() const {return centers_;}
	std::vector<std::pair<double, double>> widthPairs() const;
	bool empty() {return bounds_.empty();}
	std::string toString() const;

private:
	std::vector<double> centers_;
    std::vector<double> bounds_;

    static double center(double v1, double v2, InterPolate ipMode);

};


}  // end namespace src
#endif // MESHDATA_HPP
