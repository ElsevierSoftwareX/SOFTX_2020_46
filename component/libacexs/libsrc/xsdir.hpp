#ifndef XSDIR_HPP
#define XSDIR_HPP

#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

#include "acefile.hpp"
namespace ace {

struct XsInfo
{
	std::string tableID;      // table名(ZAID/SZAX)
	double awr;				  // 中性子を1とする質量比
	std::string filename;     // tableの格納されているファイル名
	std::string accessRoute;  // アクセス不能な場合のアクセスパス。不使用なら0(使われている例を見たことがない）
	int filetype;             // 1:type-1(ascii) 2:type-2(binary)
	int address;              // テーブルの開始行番号(type1),あるいは最初のデータの位置(type2)
	int tableLength;          // テーブルのデータ数。ここでテーブルは純然たる数値データ部分だけでヘッダ等は含まない
	int recordLength;         // ＝0(type1) レコード長(type1)これは環境依存で倍精度なら4096とかそういう値
	int entriesPerRecord;     // = 0(type1), あるいはレコードあたりの入力データ数(type2)
	double temperature;       // 温度(MeV)
	bool hasPtable;           // 非分離共鳴での確率テーブルがあるか

	std::string toString() const;
};

// 40095.86c 94.0927 endf71x/Zr/40095.716nc 0 1 4 260181 0 0 2.1543E-08 ptable
class SRCSHARED_EXPORT XsDir
{
public:
	typedef std::string zaid_type;
	XsDir(){;}
	// 第二引数が空なら全てのNTYを対象とする。第三引数がfalseなら断面積ファイル情報は読み取らない
	XsDir(const std::string &xsdirFilename,
		  const std::vector<ace::NTY> &targetNtyVec = std::vector<ace::NTY>(),
		  bool readXsInfo = true);

	bool empty() const {return xsInfoMap_.empty();}
	// ZAIDをキーにした, awr(Atomic Weight Ratio)のマップを返す
	const std::unordered_map<zaid_type, double> awrMap() const {return awrMap_;}
	// xsdirファイルでdatapathが指定されている場合はそれを返す。
	const std::string &datapath() const {return datapath_;}
	const std::string &filename() const {return filename_;}
	//std::vector<std::pair<std::string, double>> getAtomicWeightRatio(const std::string &ZA);
	// id:ZAIDがSZAXを与えてその核種tableの情報を返す。
    XsInfo getNuclideInfo(const std::string &id, ace::NTY nty) const;
	//const std::string fileName() const {return filename_;}

private:
	std::string filename_;
	std::string datapath_;
	std::unordered_map<zaid_type, double> awrMap_;  // ZAIDをキーにした, awr(Atomic Weight Ratio)のマップ
	// 核種のidentifierを省略した場合、「最初に出てくる核種」が該当となるため、
	// xsdirファイルに出現した順序を保持する必要がある。(最初に出てきたものがデフォルトになるから)
	// このためにはXsInfoについてはunordered_map, mapは不可
	//std::unordered_map<std::string, XsInfo> xsInfoMap_; 不可
	std::unordered_map<zaid_type, std::vector<XsInfo>> xsInfoMap_; // ZAIDをキーにした 核種ファイル情報マップ

	static void registerXsInfo(const XsInfo &xsinfo, std::unordered_map<std::string, std::vector<XsInfo>>* infoMap);
};



}  // end namespace ace
#endif // XSDIR_HPP
