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
#ifndef CELLCARD_HPP
#define CELLCARD_HPP

#include <atomic>
#include <map>
#include <list>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <vector>




namespace geom {
class SurfaceMap;
class CellCreator;

}

namespace inp {

// 「#で指定されているセル名」のvectorを返す。
std::vector<std::string> GetComplimentCellNames(const std::string &str);
std::set<std::string> GetSurfaceNames(std::string str);


struct CellCard {
	typedef std::unordered_map<std::string, std::string> map_type;
	static constexpr int NOT_USED_ORDER = -1;

    CellCard(){}

	// TODO このコンストラクタは廃止したい。
	CellCard(const std::string &cellName, const std::string &matName,
			 double dens, const std::string &equationStr, const std::string &lk,
			 const map_type &params, const std::vector<std::string> &dep, const std::string &trStr)
		:name(cellName), materialName(matName), density(dens),
		 equation(equationStr), likeCell(lk), parameters(params),
	     depends(dep), trcl(trStr), order(0)
    {}
	CellCard(const std::string &fname, unsigned long lnum,
			 const std::string &cellName, const std::string &matName,
			 double dens, const std::string &equationStr, const std::string &lk,
			 const map_type &params, const std::vector<std::string> &dep, const std::string &trStr)
		:file(fname), line(lnum),name(cellName), materialName(matName), density(dens),
		 equation(equationStr), likeCell(lk), parameters(params),
		 depends(dep), trcl(trStr), order(0)
    {}

	std::string file;
	unsigned long line;
	std::string name;
	std::string materialName;
	double density;
	std::string equation;
	std::string likeCell;           // like-butを使っている場合対象のセル名
	map_type parameters;            // VOL, RHO等のパラメータを格納
	std::vector<std::string> depends;  // like but及びcomplimentでの依存先セル名(univ依存は含めない)
	std::string trcl;  // trcl文字列
	// 入力読み取り後にセルカード間依存を解決するために導入される変数
	int order; // セル構築順序。コンプリメントなどを使っているセルは後になる。

    std::string pos() const{ return file + ":" + std::to_string(line);}  // file:line 文字列を返す
    std::string getHeaderString() const; // セル名、材料、密度をを出力する。
    std::string getParamsString() const; // パラメータを文字列化して出力する。
    std::string toString() const;       // セル情報文字列の出力
    std::string toInputString() const;  // セルカード入力を出力
    std::unordered_multimap<std::string, std::string> fillingUniverses() const;  // fill=で使用しているuniverse名のセット
        // TRCLによってセルカードの多項式中に含まれるsurface名をTR後のものに変更し、
	// "TR対象surface名, TRCLしたセル名、TRSF文字列"のタプルリストを返す。
	std::list<std::tuple<std::string, std::string, std::string>> solveTrcl();
	// TRCLを追加する。第二引数がtrueなら末尾に、そうでなければ先頭に追加する。第一引数が空ならなにもしない
	void addTrcl(const std::string &newTrStr, bool isBack);
	bool hasTrcl() const {return !trcl.empty();}


	static CellCard fromString(const std::string &str, bool checkValidUserInput = false);
	static CellCard fromString(const std::string &file, size_t line, const std::string &str, bool checkValidUserInput = false);
	static const std::regex &getDeclPattern();  // Dimension declaratorの正規表現を返す。
	static bool solveCellCardDependency(const std::unordered_map<std::string, int> &nameIndexMap,
										const std::unordered_map<std::string, CellCard> &solvedCards,
										CellCard *dcard);
	// FILLされているouterCellのカードを展開してfillされた内部セル群を取得する。
//	static std::multimap<int, CellCard> getFilledCards(const std::unordered_map<size_t, math::Matrix<4>> &trMap,
//													   const std::unordered_multimap<std::string, CellCard> &univMap,
//													   const CellCard &outerCellCard,
//													   geom::SurfaceMap* surfMap, const int maxThreadCount);
        static std::vector<CellCard> getFilledCards(geom::CellCreator *creator,
                                                    const std::unordered_map<std::string, std::vector<CellCard> > &univMap,
                                                    const CellCard &outerCellCard,
                                                    const int maxThreadCount, const int depth, std::atomic_bool *cancelFlag, std::atomic_int *counter);

};

bool operator < (const CellCard& c1, const CellCard &c2);


}  // end namespace cell
#endif // CELLCARD_HPP
