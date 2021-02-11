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
#ifndef MCNPINPUT_HPP
#define MCNPINPUT_HPP

#include <deque>
#include <list>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "core/io/input/inputdata.hpp"

namespace phys{
enum class ParticleType;
}

namespace inp{

class DataLine;

namespace mcnp{

// std::list<DataLine>を受け取ってMCNPのcellcards, surfacecards, datacardsへの分解などを行う。
class McnpInput : public InputData
{
public:
    McnpInput(const conf::Config &conf):InputData(conf) {config_.mode = McMode::MCNP;}
	virtual ~McnpInput(){;}

	const std::string &title() const {return title_;}
	const std::string &message() const {return message_;}
	// 入力ファイルのpathと（あれば）datapath加味してxsdirファイル名を返す。
	// 入力ファイルの位置を加味してdatapathを返す (絶対パスとは限らないがopen可能のはず)
	std::string datapath() const;
	//const std::list<DataLine> &dataCards() const; // cardsVec_.at(2)に過ぎないので廃止


	// 仮想関数
	void init(const std::string &inputFileName) override final;
	std::vector<phys::ParticleType> particleTypes() const override final;

private:
	std::string title_;
	std::string message_;  // message blockの"MESSAGE:"を除いた正味入力
	std::deque<std::list<inp::DataLine>> cardsVec_; //



	// message block中での xsdir指定がある場合のその引数
	const std::string xsdirArg() const;
	// message block中でのdatapath指定がある場合のその引数
	const std::string datapathArg() const;
	std::pair<DataLine, std::string> getDataCardAndArg(const std::string &cardName, bool warnDuplicated) const;
};

}  // end namespace mcnp
}  // end namespace inp
#endif // MCNPINPUT_HPP
