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
#include "dataline.hpp"

#include <regex>
#include "ijmr.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/message.hpp"
#include "core/io/input/common/commoncards.hpp"


namespace  {

}

std::ostream &inp::operator<<(std::ostream &os, const inp::DataLine &dl)
{
	if(dl.echo) {
		os << dl.pos()<< "  " << dl.data;
	} else {
		os << "NOECHO " << dl.file << ":" << dl.line << "  " << dl.data;
	}
	return os;
}

std::string inp::DataLine::pos() const
{
	return file + ":" + utils::toString(line);
}


std::string inp::DataLine::expandIJMR(const DataLine &dataLine, const std::string &separators, bool warnPhitsCompat)
{
	/*
	 * ijmrを展開するには入力パラメータを区切ってバラす必要がある。
	 *  separators は入力パラメータを区切る文字。
	 *  ・MCNPなら" " と"="なので " ="。
	 *  ・PHITSの場合区切りは空白のみ。"="は残す必要がある。
	 */
	std::string str = dataLine.data;
	bool hasExpandedIjmr = 	inp::ijmr::expandIjmrExpression(separators, &str);
	if(warnPhitsCompat && hasExpandedIjmr) {
		mWarning(dataLine.pos()) << "ijmr expansion is not phits compatible";
	}
	return str;
}




