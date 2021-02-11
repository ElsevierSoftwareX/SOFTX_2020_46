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
#include "physconstants.hpp"

#include <iostream>
#include <unordered_map>
#include "component/libacexs/libsrc/acefile.hpp"



// TODO particle typeの変換はもっと充実させる必要がある。
phys::ParticleType phys::strToParticleType(const std::string &str)
{
	static std::unordered_map<std::string, phys::ParticleType> ptypeMap {
		{"photon", phys::ParticleType::PHOTON},
		{"22", phys::ParticleType::PHOTON},
		{"p", phys::ParticleType::PHOTON},
		{"neutron", phys::ParticleType::NEUTRON},
		{"2112", phys::ParticleType::NEUTRON},
		{"n", phys::ParticleType::NEUTRON},
		{"tracing", phys::ParticleType::TRACING},
		{"uncollide_photon", phys::ParticleType::U_PHOTON},
		{"proton", phys::ParticleType::PROTON},
		{"2212", phys::ParticleType::PROTON},
		{"alpha", phys::ParticleType::ALPHA},
		{"a", phys::ParticleType::ALPHA},
		{"e", phys::ParticleType::ELECTRON},
		{"electron", phys::ParticleType::ELECTRON}
	};
	auto it = ptypeMap.find(str);
	//
	return (it == ptypeMap.end()) ? ParticleType::NOT_DEFINED : it->second;
}

std::string phys::particleTypeTostr(phys::ParticleType ptype)
{
	static std::unordered_map<phys::ParticleType, std::string> strMap {
		{phys::ParticleType::NEUTRON, "neutron"},
		{phys::ParticleType::NOT_DEFINED, "unknown or not implemented"},
		{phys::ParticleType::PHOTON, "photon"},
		{phys::ParticleType::TRACING, "tracing"},
		{phys::ParticleType::U_PHOTON, "uncollide_photon"},
		{phys::ParticleType::PROTON, "proton"},
		{phys::ParticleType::ALPHA, "alpha"},
		{phys::ParticleType::ELECTRON, "electron"}
	};
	return strMap.at(ptype);
}

std::ostream &phys::operator <<(std::ostream &os, phys::ParticleType ptype)
{
	os << particleTypeTostr(ptype);
	return os;
}

//double phys::nAngToMeV(double waveLengthAng)
//{
//	constexpr  double factor = 0.5*NA*1e-3*PLANK_CONSTANT*PLANK_CONSTANT/(1e-20*NEUTRON_MASS_AMU*ELEMENTARY_CHARGE);
//	//return 0.5*NA*1e-3*PLANK_CONSTANT*PLANK_CONSTANT/(lang*lang*1e-20*NEUTRON_MASS_AMU*ELEMENTARY_CHARGE);
//	return factor/(waveLengthAng*waveLengthAng);
//}

#ifndef NO_ACEXS
std::vector<ace::NTY> phys::ptypesToNtys(const std::vector<phys::ParticleType> &ptypes)
{
	std::vector<ace::NTY> retVec;
	for(auto &ptype: ptypes) {
		if(ptype == phys::ParticleType::PHOTON || ptype == phys::ParticleType::U_PHOTON) {
			retVec.emplace_back(ace::NTY::CONTIUNOUS_PHOTOATOMIC);
			retVec.emplace_back(ace::NTY::PHOTONUCLEAR);
		} else if (ptype == phys::ParticleType::NEUTRON) {
			retVec.emplace_back(ace::NTY::CONTINUOUS_NEUTRON);
			retVec.emplace_back(ace::NTY::DISCRETE_NEUTRON);
			retVec.emplace_back(ace::NTY::MULTIGROUP_NEUTRON);
			retVec.emplace_back(ace::NTY::DOSIMETRY);
		}
	}
	return retVec;
}
#endif
