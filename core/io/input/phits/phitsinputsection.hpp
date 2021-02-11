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
#ifndef PHITSINPUTSECTION_HPP
#define PHITSINPUTSECTION_HPP

#include <list>
#include <map>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace inp{

class DataLine;

namespace phits{

/*
 * PhitsInputSectionで共通化されること
 * ・セクション名とstd::list<DataLine>でコンストラクトされる
 * ・文字列への変換(とstreamへの出力)
 * ・入力と拡張入力への分離
 *
 */
class PhitsInputSection
{
public:
	PhitsInputSection(const std::string &name, const std::list<inp::DataLine> inputData,
					  bool verbose, bool warnPhitsCompat);

    std::string toString() const;
	std::string sectionName() const {return sectionName_;}
	std::list<DataLine> input() const {return input_;}
	std::list<DataLine> extension() const {return extension_;}
	std::string getInputParameter(const std::string &paramName) const;
	std::string getInputArrayParameter(const std::string &paramName, int num) const;

	// {N-M}記法を展開する。これを使って良いセクション、パラメータは限られるので注意。
	static std::string expandBrace(const std::string & str);

protected:
    std::string sectionName_;
	std::list<DataLine> input_;  // phits入力データ
	std::list<DataLine> extension_; // 独自拡張入力
	std::unordered_map<std::string, std::string> inputParams_;  // phits入力中のパラメータ
	std::unordered_map<std::string, std::map<int, std::string>> inputArrayParams_;  // file(7)=xsdirのような配列型パラメータ
	std::unordered_map<std::string, std::string> extensionParams_; // 拡張入力中のパラメータ

};


}  // end namespace phits
}  // end namespace inp

#endif // PHITSINPUTSECTION_HPP
