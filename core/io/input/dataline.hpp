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
#ifndef DATALINE_HPP
#define DATALINE_HPP

#include <iostream>
#include <string>
#include <vector>

namespace inp {


class DataLine
{
public:
    DataLine():file("NOTSET"), line(0), echo(false){}
	DataLine(const std::string& fn, const std::size_t& ln, const std::string& dat, const bool& ec = true):
        file(fn), line(ln), data(dat), echo(ec) {}

	std::string file;
	std::size_t line;
	std::string data;
	bool echo;    // エコーバックするかを判定。include時にnoechoで非表示をセットする。

	std::string pos() const;

	/*
	 *  strに含まれるijmr表現を展開する。
	 * これを実行するには継続行処理、include処理が終わっていること。
	 * なのでファイル読み込みの最初にinclude処理を実施するようにする。
	 */
	static std::string expandIJMR(const DataLine &dataLine, const std::string &separators, bool warnPhitsCompat);
	//static void expandIjmrExpression(const std::string &pos, const std::string &separators, bool warnPhitsCompat, std::string *str);

};

std::ostream& operator<<(std::ostream& os, const DataLine& dl);



}  // end namespace inp
#endif // DATALINE_HPP
