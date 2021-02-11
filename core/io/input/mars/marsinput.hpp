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
#ifndef MARSINPUT_HPP
#define MARSINPUT_HPP



#include "core/io/input/mcmode.hpp"
#include "core/io/input/inputdata.hpp"


// MARS: A MULTIPLE-ARRAY SYSTEM USING COMBINATORIAL GEOMETRY (NUREG/CR-0200)
// MARS形式の入力読み取り


namespace inp {

class DataLine;

namespace mars {


class MarsInput : public InputData
{
public:
    MarsInput(const conf::Config &conf):InputData(conf) {config_.mode = McMode::MARS;}
	virtual ~MarsInput(){;}
	// 仮想関数
	void init(const std::string &inputFileName) override final;
	std::vector<phys::ParticleType> particleTypes() const override final;
};




} // end namespace mars
} // end namespace inp
#endif // MARSINPUT_HPP
