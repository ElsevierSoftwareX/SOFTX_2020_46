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
#ifndef PHITSINPUTSUBSECTION_HPP
#define PHITSINPUTSUBSECTION_HPP

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/io/input/dataline.hpp"

namespace inp {
namespace phits {


/*
 *  subsection読み取り用クラス
 * サブセクションは
 * 項目行
 * パラメータ行1
 * パラメータ行2
 * …
 *
 * という構造。項目列名のnonは無視される。
 */
class PhitsInputSubsection
{
public:
	// パラメータ行n行のサブセクションをlist<DataLine>から読み込んでサブセクションを作成する。
	PhitsInputSubsection(std::list<inp::DataLine>::const_iterator &it,
						   std::list<inp::DataLine>::const_iterator endIt,
						   size_t n);
	PhitsInputSubsection();

	// サブセクションで入力された値の数(=点数)
	size_t numberOfParameterTypes() const {return parameterVectorsMap_.size();}  // 入力されたパラメータの種類の数
	size_t numberOfParameters() const;  // 一つの種類あたりのパラメータが入力数
	// 項目名 titleのデータベクトルを返す。
	std::vector<std::string> getValueVector(const std::string &title) const;
	// title名がstrの入力が存在するか？
	bool hasTitle(const std::string &str) const;
	// 出力用
	std::string toString() const;

private:
	std::vector<std::string> indexToTitle_;
	std::unordered_map<std::string, std::vector<std::string>> parameterVectorsMap_;

};


}  // end namespace phits
}  // end namespace inp

#endif // PHITSINPUTSUBSECTION_HPP
