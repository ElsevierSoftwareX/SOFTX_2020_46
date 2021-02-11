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
#include "original_metacard.hpp"


const std::regex &inp::org::getOriginalCommandPattern()
{
	/*
	 *  オリジナルコマンドは
     *  c _
     *  C _
     * のように行頭から4文字以内に"c"か"C"で空白を開けて"_"でMCNP/Phitsでコメント扱いになる。
	 */
    static const std::regex orgComPattern = std::regex(R"(^ {0,4}[cC] _(.*))");
	return orgComPattern;
}
