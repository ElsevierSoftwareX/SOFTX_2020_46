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
#ifndef PHYSCONSTANTS_HPP
#define PHYSCONSTANTS_HPP

#include <iostream>
#include <string>
#include <vector>

namespace ace {
enum class NTY;
}


namespace phys {

enum class ParticleType {NOT_DEFINED, PHOTON, NEUTRON, U_PHOTON, TRACING, PROTON, ALPHA, ELECTRON};

constexpr double ELEMENTARY_CHARGE = 1.602176620898E-19;       // 電荷素量(C)
constexpr double NA                 = 6.022140857E+23;          // アボガドロ数
constexpr double PLANK_CONSTANT    = 6.62607004081E-34;        // プランク定数Js
constexpr double NEUTRON_MASS_AMU  = 1.0086649158849;          // 中性子質量(amu)
constexpr double NEUTRON_MASS_KG   = 1e-3*NEUTRON_MASS_AMU/NA; // 中性子質量(kg)

// 中性子の波長(オングストローム)→MeV変換
constexpr double nAngToMeV(double waveLengthAng){
	return 1.0E-6*0.5*PLANK_CONSTANT*PLANK_CONSTANT
	        /(waveLengthAng*waveLengthAng*1e-20*NEUTRON_MASS_KG*ELEMENTARY_CHARGE);
}


ParticleType strToParticleType(const std::string &str);
std::string particleTypeTostr(ParticleType ptype);
std::ostream &operator << (std::ostream &os, ParticleType ptype);

#ifndef NO_ACEXS
std::vector<ace::NTY> ptypesToNtys(const std::vector<phys::ParticleType> &ptypes);
#endif

}  // end namespace phys


#endif // PHYSCONSTANTS_HPP
