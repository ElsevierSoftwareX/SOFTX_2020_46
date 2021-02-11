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
#ifndef PHITSINPUT_HPP
#define PHITSINPUT_HPP

#include <list>
#include <map>
#include <string>
#include <vector>
#include "core/physics/particle/particle.hpp"
#include "core/math/nmatrix.hpp"
#include "core/io/input/mcmode.hpp"
#include "core/io/input/inputdata.hpp"

namespace conf{
struct Config;
}

namespace inp {

class DataLine;

namespace phits{

class PhitsInput :public InputData
{
public:
    PhitsInput(const conf::Config &conf):InputData(conf) {config_.mode = McMode::PHITS;}
    virtual ~PhitsInput() override {}

	std::vector<std::list<inp::DataLine>> tallySections() const;
	std::vector<std::list<inp::DataLine>> sourceSections() const;

	// 仮想関数
	void init(const std::string &inputFileName) override final;
	std::vector<phys::ParticleType> particleTypes() const override final;
	std::list<inp::DataLine> colorCards() const override final;


private:
	// ファイルを読み込んだら、セクションごとに↓に保存。
	std::multimap<inp::phits::Section, std::list<inp::DataLine>> sectionsMap_;
	// セクション列挙子を与えて最初に見つかった該当するセクションの入力リストを返す。
	std::list<DataLine> getPhitsSectionCards(const inp::phits::Section& sect) const;
	// セクション列挙子を与えて該当する全てのセクションの入力リストvectorを返す。
	std::vector<std::list<DataLine>> getPhitsSections(inp::phits::Section sect) const;
};



}  // end namespace phits
}  // end namesapce inp


#endif // PHITSINPUT_HPP
