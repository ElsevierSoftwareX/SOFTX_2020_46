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
#ifndef CGBODY_HPP
#define CGBODY_HPP

#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace inp {
// CGBodyはQAD, MARS共通なので、名前空間inpの直下に置く
// CG定義構造体 CG Bodyの略か。
class CGBody {
public:

	CGBody(std::string &&mnemonic, int &&bodyNumber, std::vector<double> &&params)
		:mnemonic_(mnemonic), id_(bodyNumber), parameters_(params)
	{;}
	CGBody(const std::string &mnemonic, const int &bodyNumber, const std::vector<double> &params)
		:mnemonic_(mnemonic), id_(bodyNumber), parameters_(params)
	{;}

	// アクセサ
	const std::string &mnemonic() const {return mnemonic_;}
	const int &id() const {return id_;}
	const std::vector<double> &parameters() const {return parameters_;}

	// ENDカードならtrueを返す
	bool isEnd() const;
	/*
	 * TODO QADのCGはMCNPとほぼ互換なので基本的にそのまま出力すればOKだが、
	 * MARSのCGはそれ以外を含む。
	 *
	 * 既存マクロボディに回転が加わったもの
	 * ・BPP: 回転つき直方体 → RPPにTRSFを付けて対応させる。
	 * ・WPP: 回転つき楔形 → RPPにTRSFを付けて対応させる
	 * 名前だけが違うもの
	 * ・GEL: 一般楕円体 → ELLの第七引数正入力とおなじ。(MCNPにはバグがあるけど)
	 * 該当するものがない
	 * ・TOR:トーラスの一部 → TX/TY/TZとRPPの組み合わせで対処
	 * ・QUA:斜楕円錘台 → GQで対応
	 * ・PS:無限平面 → 引数がgeom::Planeクラスのコンストラクタとほぼ同じなので、Plane::toInputString()で対応。。
	 * ・C:無限円筒 → 一般円筒CAとしてgxsview独自拡張で実装済み
	 */
	// MCNP マクロボディ風の文字列にして返す
	std::string toInputString() const;

	// static
	static bool isEndString(const std::string& str); // 終了文字列(ENDしか含まない)ならtrue
	static int getUniqueID();
	static void initUniqueID();
	static bool isContData(const std::string &str);
	static CGBody fromQadFixedString(const std::string &str);
	static std::vector<CGBody> getCgBodiesFromQadFixed(std::stringstream &ss, int &lineNumber);
	//static CGBody fromMarsFreeString(const std::string &str);

private:
	std::string mnemonic_;  // 形状を表すニモニック ELL等. QAD-CGではITYPE
	int id_;           // 番号(一意の識別ID). QAD-CGではIALP
	std::vector<double> parameters_;  // パラメータ. QAD-CGではFPD

	// static
	static int seqID_;  // IDに一意な番号を自動的に割当に使う整数値
	static std::unordered_set<int> usedIDs_; // 使用済みID

};

}  // end namespace inp

#endif // CGBODY_HPP
