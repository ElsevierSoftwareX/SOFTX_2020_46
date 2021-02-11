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

#include <string>
#include <unordered_set>
#include <vector>

namespace inp {
namespace qad {

// CG定義構造体 CG Bodyの略か。
class CGBody {
public:

	CGBody(std::string &&mnemonic, int &&bodyNumber, std::vector<double> &&params)
		:itype_(mnemonic), ialp_(bodyNumber), fpd_(params)
	{;}
	CGBody(const std::string &mnemonic, const int &bodyNumber, const std::vector<double> &params)
		:itype_(mnemonic), ialp_(bodyNumber), fpd_(params)
	{;}

	// アクセサ
	const std::string &itype() const {return itype_;}
	const int &ialp() const {return ialp_;}
	const std::vector<double> &fpd() const {return fpd_;}

	// ENDカードならtrueを返す
	bool isEnd() const;
	// MCNP マクロボディ風の文字列にして返す
	std::string toInputString() const;
	// static
	static int getUniqueID();
	static void initUniqueID();
	static bool isContData(const std::string &str);
	static CGBody fromString(const std::string &str);

private:
	std::string itype_;
	int ialp_;
	std::vector<double> fpd_;

	static int seqID_;  // 自動番号割当に使う番号
	static std::unordered_set<int> usedIDs_; // 使用済みID

};



}  // end namespace qad
}  // end namespace inp

#endif // CGBODY_HPP
