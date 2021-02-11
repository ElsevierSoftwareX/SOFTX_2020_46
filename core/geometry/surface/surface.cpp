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
#include "surface.hpp"

#include <cmath>
#include <limits>
#include <functional>
#include <regex>


#include "plane.hpp"
#include "core/geometry/cell/cell.hpp"
#include "core/utils/string_utils.hpp"




int geom::Surface::surfaceCount = 0;
std::mutex geom::Surface::mtx;


const double geom::Surface::MAX_LENTGH = std::pow(10, static_cast<int>(std::log10(std::numeric_limits<float>::max()))-3);
geom::Surface::Surface(const std::string &typestr, const std::string &name)
    :type_(typestr), name_(name), reversed_(false) {
    /*
         *  Surfaceは無名面、表面、裏面の3種類が存在する。
         * ・無名面は幾何演算のためだけに作成し、cellの定義には使われない
         * ・おもて面はセル定義及び粒子との交点決定のために使われる
         * ・裏面はセル定義のために使われる。
         *
         * 無名面にはIDを設定しない(NO_ID)、
         * 表面は一意にIDを設定する
         * 裏面はおもて面のID×-1を適用する。
         */
    if(name_.empty()) {
        ID_ = NO_ID;
        return ;
    }

    if(name_.front() == '-') {
        reversed_ = true;  // 最初が-なら表裏逆の面 "-s"と"s"は別の面として扱うので"-"は削除しない
        // 裏面にはIDの自動的付与は実施しない
    } else  {
        if(name_.front() == '+') {
            if(name_.size() <= 2) throw std::invalid_argument(name_ + " is invalid(empty) cell name");
            name_ = name_.substr(1);  // +はデフォルトなので付けてあっても省略
        }
        // surfaceCountはstatic変数なのでマルチスレッド実行のためには排他処理が必要
        std::lock_guard<std::mutex> lck(mtx);
        ID_ = ++surfaceCount;
    }
}

geom::Surface::~Surface(){}

const geom::Surface::cell_map_type &geom::Surface::contactCellsMap() const {return contactCellsMap_;}



// コンプリメントの引数はセル名なのでsurface名には含めない！！！！
std::vector<std::string> geom::Surface::extractSurfaceNames(const std::string &nameEquation)
{
	std::smatch sm;
	std::regex namePattern = Surface::getSurfaceNamePattern();
	auto it = nameEquation.cbegin() ;
	auto end = nameEquation.cend() ;
	std::vector<std::string> names;
	while(std::regex_search(it, end, sm, namePattern)) {
//		for(size_t i = 0 ; i < sm.size(); ++i) {
//			std::cout << "i=" << i << ", matched surfacename=\"" << sm.str(i) << "\"" << std::endl;
//		}

		names.emplace_back(sm.str(2));
		it = sm[0].second;
	}
	return names;
}



const std::regex &geom::Surface::getSurfaceNamePattern()
{
//	static std::regex namePattern(R"(([-+\.a-zA-Z0-9]+))"); これではコンプリメントにもヒットしてしまう
//	static std::regex namePattern(R"(([^#][-+\.a-zA-Z0-9]+))"); これでは^#に(や-などがヒットしてしまう
	static std::regex namePattern(R"((^|[ :)(])([-+\.a-zA-Z0-9]+))");
	return namePattern;
}








#define COMPARE_NPOS(POS1, POS2) \
	if((POS1) != std::string::npos && (POS2) == std::string::npos) {\
		return false;\
	} else if((POS1) == std::string::npos && (POS2) != std::string::npos) {\
		return true;\
    } (void)POS1

// surfaceの名前によるソート関数。自動生成面は後ろに回し、数値化できる場合は数字順、あとは辞書順
bool geom::Surface::surfaceNameLess(const std::string &name1, const std::string &name2)
{
	// @を含む面は無条件で後ろ
	auto pos1 = name1.find("@"), pos2 = name2.find("@");
	COMPARE_NPOS(pos1, pos2);
	// 次は_tを含む
	pos1 = name1.find("_t");
	pos2 = name2.find("_t");
	COMPARE_NPOS(pos1, pos2);

	// 次は[か]を含む
	pos1 = name1.find("[");
	pos2 = name2.find("[");
	COMPARE_NPOS(pos1, pos2);

	// 両方数値化可能なら数値化して比較
	if(utils::isArithmetic(name1) && utils::isArithmetic(name2)) {
		return utils::stringTo<double>(name1) < utils::stringTo<double>(name2);
	} else {
		return name1 < name2;
	}
}

geom::BoundingBox geom::Surface::generateBoundingBox() const {
//	mWarning() << "Generating bounding box for surface=" << name()
//			   << "is not implemented yet. Universal box is applied.";
	return BoundingBox::universalBox();
}








//void geom::Surface::registerContactCell(const std::shared_ptr<const Cell> &cell)
//{
//    // 同一keyエントリがなければどんどん追加していく。
//    if(contactCellsMap_.find(cell->cellName()) == contactCellsMap_.end()) {
//        //		contactCellsMap_[cell->cellName()] = cell;
//        contactCellsMap_[cell->cellName()] = cell.get();  // ナマポ版
//    }
//}

void geom::Surface::registerContactCell(const std::string &cellName, const geom::Cell *cellPtr)
{
    if(contactCellsMap_.find(cellName) == contactCellsMap_.end()) {
        contactCellsMap_[cellName] = cellPtr;  // ナマポ版
    }
}

std::string geom::Surface::toString() const
{
    std::stringstream ss;
    ss << type_;
    if(reversed_) ss << Surface::reversedChars();
    ss << ": name(ID) = " << name_ << "(" << ID_ << ")";
    return ss.str();
}

void geom::Surface::transform(const math::Matrix<4> &matrix)
{
    for(auto &planeVector: boundingPlaneVectors_) {
        for(auto &plane: planeVector) {
            plane.transform(matrix);
        }
    }
}




                                                                                      std::vector<std::vector<geom::Plane> > geom::Surface::boundingPlaneVectors() const
    {
                                                                                      // メンバ変数としてplaneVectorsを持ってしまうと、transformなどにも対応させる必要が出てしまう。
    return boundingPlaneVectors_;
}



const std::string &geom::Surface::reversedChars() {
	static std::string rev = "_r";
	return rev;
}

std::string geom::Surface::removeSign(const std::string &surfName)
{
	if(surfName.front() == '+' || surfName.front() == '-') {
		return surfName.substr(1);
	} else {
		return surfName;
	}
}

std::string geom::Surface::reverseName(const std::string &name) {
    //throw std::invalid_argument("Creating reveresed side of anonymous surface.");
    if(name.front() == '-') {
		return name.substr(1);
	} else {
		return std::string("-") + name;
	}
}

#ifdef ENABLE_GUI
#include <vtkImplicitBoolean.h>
#include <vtkSphere.h>
constexpr double geom::Surface::LARGE_DOUBLE;
vtkSmartPointer<vtkImplicitFunction> geom::Surface::getVtkCompliment(vtkSmartPointer<vtkImplicitFunction> func)
{
	auto boolOp = vtkSmartPointer<vtkImplicitBoolean>::New();
	auto universalSphere = vtkSmartPointer<vtkSphere>::New();
	universalSphere->SetCenter(0, 0, 0);
	universalSphere->SetRadius(LARGE_DOUBLE);
	boolOp->AddFunction(universalSphere);
	boolOp->AddFunction(func);
	boolOp->SetOperationTypeToDifference();
	return boolOp;
}
#endif

