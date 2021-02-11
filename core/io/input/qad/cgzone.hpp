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
#ifndef CGZONE_HPP
#define CGZONE_HPP

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace inp {
namespace qad {

class CGZone
{
public:
	CGZone(const std::string &name, int numZone, const std::vector<std::pair<bool, int>> &eqElements)
		:ialp_(name), naz_(numZone), jtyPairs_(eqElements)
	{;}

	// アクセサ
	const std::string &ialp() const {return ialp_;}
	const int &naz() const {return naz_;}
	const std::vector<std::pair<bool, int>> &jtyPairs() const {return jtyPairs_;}

	bool isEnd() const;
	// mcnpのセルカード風に変換…といきたいがmaterialデータがzoneには含まれていない。
	//std::string toInputString() const;
	std::string toString() const; // このクラスの情報を文字列化する。
	std::string equation() const;  // MCNP形式の論理式を返す

	static CGZone fromString(const std::string &str);
	// strが継続データならtrueを返す。
	static bool isContData(const std::string &str);

	static void initUniqueID();
	static int getUniqueID();

private:
	std::string ialp_;  // ialp部分が空なら継続行
	int naz_;
	// iiblasとjtyをペアにしたもの ORがあればfirstはtrue
	std::vector<std::pair<bool, int>> jtyPairs_;

	static int seqID_;
	static std::unordered_set<int> usedIDs_;

};


}  // end namespace qad
}  // end namespace inp

#endif // CGZONE_HPP
