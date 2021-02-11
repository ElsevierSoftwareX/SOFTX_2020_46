#ifndef ACEFILE_HPP
#define ACEFILE_HPP

#if (defined(_WIN32) || defined(_WIN64)) && !defined(DIRECTUSE_LIBACEXS)
	#define DECL_EXPORT     __declspec(dllexport)
	#define DECL_IMPORT     __declspec(dllimport)
#else
	#define DECL_EXPORT
	#define DECL_IMPORT

#endif


#if defined(SRC_LIBRARY)
	#define SRCSHARED_EXPORT DECL_EXPORT
#else
	#define SRCSHARED_EXPORT DECL_IMPORT
#endif

/*
 * SRCSHARED_EXPORTはwindows環境で
 * __declspec(dllexport) ライブラリビルド時
 * __declspec(dllimport) ライブラリ使用時
 * に定義される。そうしないとdllを外部から参照できない
 * unixの場合、あるいは直接hpp,cppをインクルードして使用する場合は必要ない。
 */


#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

#include "mt.hpp"

namespace ace {
enum class NTY {
				CONTINUOUS_NEUTRON = 1,       // 中性子用連続エネルギーライブラリ
				MULTIGROUP_NEUTRON = 2,       // 中性子用多群ライブラリ。ただし本来１と２は区別されない。らしい。
				DISCRETE_NEUTRON = 7,             // 離散中性子ライブラリ
				DOSIMETRY = 3,                // ドシメトリファイル
				THERMAL = 4,                  // 熱中性子 未実装
				CONTIUNOUS_PHOTOATOMIC = 5,  // 光子原子相互反応
                PHOTONUCLEAR = 6,              // 光核反応
                ELECTRON,
                PROTON
};

std::string ntyToString(NTY nty);
// ZAID/SZAXのclass文字列からNTYに変換
NTY classStrToNty(const std::string &str);
NTY getNtyFromZaidx(const std::string &zaidx);

// ZAIDXからclass文字列を取得する
std::string getClassStr(const std::string &zaidxStr);
// ZAIDXからZA文字列を取得する。
std::string getZaStr(const std::string &zaidxStr);
std::string getClassRegexStr(NTY nty);

bool isZAIDX(const std::string& str);
bool isSZAX(const std::string& str);

struct AngularDistribution {
	double energy; //入射エネルギー
	int interpolation; // 補間モード

	std::vector<double> angular_points; // 角度分点
	std::vector<double> pdf;
	std::vector<double> cdf;

	AngularDistribution(
			const double ene, const int itp, const std::vector<double> apoints,
			const std::vector<double> newpdf, const std::vector<double> newcdf
			)
			: energy(ene), interpolation(itp), angular_points(apoints), pdf(newpdf), cdf(newcdf){;}
};


struct CrossSection {
	std::vector<double> epoints;    //! エネルギー分点
	std::vector<double> xs_value;   //! 断面積値
    ace::Reaction mt;         //! MT番号
	int release_n;                  //! 放出中性子数
	double Qval;                    //! Q値
	long int e_offset;              //! フルのE分点の何番目から断面積が与えられているか。total, elastic等はoffset=0.

	/*!  角度分布データの格納位置
	 * 0：等方分布。 LABかCMどちらで等方かはTYRブロックで決められる
	 * -1：ANDブロックにデータなし。散乱方向はDLWブロックで決められる。
	 * n：角度分布データの開始位置。ANDブロックの先頭を基準とする。
	 */
	int angular_flag;
	std::vector<AngularDistribution> angular_dists;

	CrossSection();
	CrossSection(
	        const std::vector<double>& ep,      // エネルギー分点
	        const std::vector<double>& xs,      // 分点での断面積値
            const ace::Reaction & react,  // 断面積の反応種
	        const int &r_n,                     // 放出中性子数
	        const double &qv,                   // Q値
            const long &eoff,               // Eオフセット(閾値のある反応では低Eにデータが与えられていない)
	        const int &af                       // angular_flag：角度分布の有無等の判定データ
	        );

	void setAngularFlag(int af){ this->angular_flag = af; }
	void dump();
	void dump(std::ostream &ost);
	double getValue(double energy) const;
};


// AceFileはTransportAceFile, DosimetryAceFile, PhotoatomicAceFileの共通基底クラス。
class SRCSHARED_EXPORT AceFile
{
public:
	//! MTナンバーをキーとしたマップに断面積データを格納する。これはそれの型のイテレータ
	typedef std::unordered_map<ace::Reaction, CrossSection> XSmap_type;
	typedef XSmap_type::iterator  MTmap_iterator;

	// コンストラクタは輸送/ドシメトリ/光子原子相互反応共通の処理を実施する。
	// ・一体型ファイルから目的核種の部分までseekする。
	// ・読み取り終了時に目的核種の最後まで読み込んだかチェック
	//
	AceFile();
	AceFile(std::ifstream& ifs, const std::string id, std::size_t startline);
	virtual ~AceFile(){;}
	void dump();
	// reaction断面積を返す
	const CrossSection &getCrossSection(ace::Reaction reaction) const;
	const XSmap_type &getXsMap() const {return XSmap_;}

	virtual void DumpNXS(std::ostream& os) = 0;
	virtual void DumpJXS(std::ostream& os) = 0;

protected:
	static const std::size_t NXS_SIZE=16;
	static const std::size_t JXS_SIZE=32;

	// header部分を読み飛ばす
    void getAceHeader(std::ifstream& is);
	// ifsを当該核種の部分まで進める
	void seek(std::ifstream& ifs, const std::string& id, size_t startline);
	bool checkEndOfData(std::ifstream& ifs, const bool &outputResidualData = true);
	std::vector<long int> getNXS(std::istream& is);
	std::vector<long int> getJXS(std::istream& is);  // JXS array をifstreamから取得


	// SZAX= SSSZZZAAA.dddCC, s:励起状態, z:原子番号, a:質量数, d:ライブラリ識別子, c:ライブラリクラス
	// 1027058.710nc = 励起状態のCo-58 ENDF/B-VII連続エネルギー中性子ライブラリ
	std::string ID_; // ZAIDXかSZAXが保存される
	XSmap_type XSmap_;

	std::vector<std::string> xss_;  // ACEファイルの断面積データ部分
	std::vector<long int> nxs_;     // ACEファイルのNXSヘッダ
	std::vector<long int> jxs_;     // ACEファイルのJXSヘッダ

// static
public:
	// aceファイル名、 対象zaidx, start行によってaceファイルを作成。zaidxはidentifier,classを完備していなければならない。
	static std::unique_ptr<AceFile> createAceFile(const std::string &filename,
												  const std::string &zaidx, std::size_t startline);

};


}  // end namespace ace









std::vector<ace::AngularDistribution> ReadAngularTable(const std::vector<std::string>& xss, const std::size_t andBlockPos, const size_t angularArrayPos);



#endif // ACEFILE_HPP

