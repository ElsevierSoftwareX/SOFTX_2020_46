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
#include "mcnp_metacards.hpp"
#include <vector>
#include "core/utils/string_utils.hpp"

const std::regex &inp::mcnp::getReadCardPattern()
{
	static const std::regex readCardPattern = std::regex(R"(^ {0,4}read)", std::regex_constants::icase);
	return readCardPattern;
}



const std::regex &inp::mcnp::getFilePattern()
{
	static const std::regex filePattern = std::regex(R"([fF][iI][lL][eE] *=* *)");
	return filePattern;
}

const std::regex &inp::mcnp::getNoechoPattern()
{
	static const std::regex noechoPattern = std::regex(R"([eE][cC][hH][oO])");
	return noechoPattern;
}

#include "core/io/input/common/commoncards.hpp"
std::pair<std::string, bool> inp::mcnp::procReadCard(std::string readArgStr)
{
	// 次はREADカードの、パラメータ, FILEとその引数、 NOECHOを探す。
	// 空白及び=で区切ればパラメータ＋parm引数の混合vectorになる。
	// 後置コメントをまず削除する。
	comm::removeMcnpPostComment(&readArgStr);
	comm::removePhitsPostCommentNotSharp(&readArgStr);
	comm::removePhitsPostCommentSharp(&readArgStr);

	std::vector<std::string> parameters = utils::splitString(" =" , readArgStr);

	//mDebug() << "parameters vec=" << parameters;
	bool echoFlag = true;
	std::string includedFileName;
	std::smatch sm;
	for(std::size_t i = 0; i < parameters.size(); i++) {
		if(std::regex_search(parameters.at(i), sm, mcnp::getFilePattern()) && i != parameters.size()-1) {
			includedFileName = parameters.at(i+1);
		} else if(std::regex_search(parameters.at(i), sm, mcnp::getNoechoPattern())) {
			echoFlag = false;
		}
	}
	includedFileName = utils::dequote('\"', includedFileName);

	return std::make_pair(includedFileName, echoFlag);
}
