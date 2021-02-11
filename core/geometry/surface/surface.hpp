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
#ifndef SURFACE_HPP
#define SURFACE_HPP

#include <iostream>
#include <limits>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "surfacemap.hpp"
#include "core/math/nvector.hpp"
#include "core/math/nmatrix.hpp"

#include "vtksurfaceheaders.hpp"




namespace geom {

class Cell;
class Plane;
class BoundingPlaneInfo;
class BoundingBox;

/*
 * 境界上の内外について
 * ・表裏が通常の面の境界直上は法線側=表(forward)側とする。
 * ・裏返された(reversed)面の境界上も法線側=表(forward)とする。
 * この結果境界上の点は複数のセルに所属し得る。
 * ・境界上の点がどちらのセルに所属しても物理的には有意な差は無い
 * ・セルに所属しない点が発生するよりはまし
 * なため、この扱いを採用する。
 */
class Surface
{
public:
	/*
	 * shared_ptr<T> → shared_ptr<const T>は T* → const T*のように振る舞うよう作られているが
	 * map<T> → map<const T> は別の型として扱われるため同様の振る舞いにはできない。
	 * Surface, Surface共に構築後変更するのでshared_ptr<Surface>, shared_ptr<Cell> のみ使う。
	 */
	typedef std::string name_type;
	typedef SurfaceMap map_type;
	typedef std::unordered_map<std::string, const Cell*> cell_map_type;

	// コンストラクタ
    explicit Surface(const std::string& typestr, const std::string& name);
    virtual ~Surface();
	// 明示的コピー。コピーコンストラクタは削除しておく？
	std::shared_ptr<const Surface> makeCopy();

	// ゲッター
    const cell_map_type &contactCellsMap() const;

    // 面しているcellを登録する。
    //void registerContactCell(const std::shared_ptr<const Cell> &cell);
    void registerContactCell(const std::string& cellName, const Cell* cellPtr);

	virtual std::string toString() const;
	// toInputStringは1．マクロボディ展開、2．TRCLセルの作成に使われる。
	virtual std::string toInputString() const = 0;
	// 表裏逆の面を生成する。
	virtual std::unique_ptr<Surface> createReverse() const = 0;
    virtual void transform(const math::Matrix<4> &matrix);  // 変換行列(回転と並進)による変換方法を規定
	virtual bool isForward(const math::Point& point) const = 0;
	virtual math::Point getIntersection(const math::Point& point, const math::Vector<3>& direction) const = 0;
    virtual std::shared_ptr<Surface> makeDeepCopy(const std::string &newName) const = 0;

    /*
	 * generateBoundingBox → 軸平行面に基づく簡易BB計算
	 * boundingPlanes → 詳細BB計算用の(非)軸平行面を返す。
	 */
	// BoundingBox作成のためのBoundingPlaneを返すstd::vector<plane>同士はORで vector内のPlaneはANDで連結されると想定する。
    std::vector<std::vector<Plane>> boundingPlaneVectors() const;



	const name_type & name()const {return name_;}
	bool isReversed() const {return reversed_;}
	void setID(int newID) {ID_ = newID;}
	int getID() const {return ID_;}
	void setFileName(const std::string &f) {fileName_ = f;}

	// static
    static const double MAX_LENTGH;
	static const std::string &reversedChars();
	static std::string removeSign(const std::string &surfName);
	static std::string reverseName(const std::string &name);
	static std::vector<std::string> extractSurfaceNames(const std::string &nameEquation);
	static const std::regex &getSurfaceNamePattern();
	//static std::string getTransformedSurfaceName(const std::string &tredCellName, const std::string &oldSurfaceName); // TRCLで生成される面の名前
	static std::string getLatticeIndexedName(const std::string& basename, int index);
	// ID番号を再度0から振り直すようにする。(テストで必要)
	static void initID() {surfaceCount = 0;}


protected:
    virtual std::vector<std::vector<Plane>> boundingPlanes() const = 0;

	std::string type_;  // plane, sphere等
	// surfaceインスタンスに割り当てられるID番号。cellからアクセスしない(できない)無名SurfacenにはNO_IDが割り当てられ重複しうる
	int ID_;   // cellからはこのID番号で参照するようにする。(string nameはアクセスコストが高い)
	name_type name_;
	bool reversed_;  // reversed_= trueなら表方向(法線方向)を通常の逆にしたsurfaceとして扱う
	// surfaceに隣接しているcellのマップ。このmapを通してcellを変更しないので const geom::Cell
	cell_map_type contactCellsMap_;
	// PolyHedron等ではファイルから読み込むのでその場合のファイル名を保存する。
	std::string fileName_;
    std::vector<std::vector<Plane>> boundingPlaneVectors_;

	template <class T>
	static void CheckParamSize(size_t idealSize,
							   const std::string & surfaceTypeName,
                               const std::vector<T> &params)
	{
		if(params.size() != idealSize) {
			std::stringstream ss;
			ss << "Number of parameters should be " << idealSize << " for " << surfaceTypeName << ", params=";
			for(size_t i = 0; i < params.size(); ++i) {
				if(i != 0) ss << ", ";
				ss << params.at(i);
			}
			ss << ". size = " << params.size();
			throw std::invalid_argument(ss.str());
		}
	}
	template <class T>
	static void CheckParamSize(std::vector<size_t> idealSizes,
							   const std::string & surfaceTypeName,
                               const std::vector<T> &params)
	{
		bool isValidSize = false;
		for(auto sz:idealSizes) {
			if(params.size() == sz) {
				isValidSize = true;
				break;
			}
		}
		if(!isValidSize) {
			std::stringstream ss;
			ss << "Number of parameters should be {";
			for(size_t i = 0; i < idealSizes.size(); ++i) {
				ss << idealSizes.at(i);
				if(i != idealSizes.size()-1) {
					ss << ", ";
				} else {
					ss << "}";
				}
			}
			ss  << " for " << surfaceTypeName << " , actual  = " << params.size();
			throw std::invalid_argument(ss.str());
		}
	}

// static
public:
	// surfaceの名前によるソート関数。自動生成面は後ろに回し、数値化できる場合は数字順、あとは辞書順
	static bool  surfaceNameLess(const std::string &name1, const std::string &name2);

private:
	static std::mutex mtx;
	static int surfaceCount;
	static constexpr int NO_ID = std::numeric_limits<int>::max();

// その他
	friend std::ostream &operator << (std::ostream &os, Surface::map_type map);

// boundingBox計算用
public:
	virtual	geom::BoundingBox generateBoundingBox() const;

#ifdef ENABLE_GUI
protected:
	static constexpr double LARGE_DOUBLE = std::numeric_limits<double>::max()*1e-6;
	static vtkSmartPointer<vtkImplicitFunction> getVtkCompliment(vtkSmartPointer<vtkImplicitFunction> func);
public:
    virtual vtkSmartPointer<vtkImplicitFunction> generateImplicitFunction() const
    {
        throw std::invalid_argument(std::string("surface =") +  name() + ", generateImplicitFunction is not implimented.");
        abort();
        //return vtkSmartPointer<vtkImplicitFunction>();
	}
#endif
};


// surfaceの順序比較関数
class SurfaceLess{
public:
    bool operator()(const std::shared_ptr<const geom::Surface> &s1,
                    const std::shared_ptr<const geom::Surface> &s2) const
    {
        return geom::Surface::surfaceNameLess(s1->name(), s2->name());
    }
};




}  // end namespace geom
#endif // SURFACE_HPP
