#ifndef NEUTRONDOSIMETRYFILE_HPP
#define NEUTRONDOSIMETRYFILE_HPP

#include "acefile.hpp"
#include <fstream>
#include <string>

namespace ace {

class SRCSHARED_EXPORT NeutronDosimetryFile : public AceFile
{
public:
	NeutronDosimetryFile(std::ifstream& ifs, const std::string& id, std::size_t startline);

	void read(std::ifstream &ifs);
	void readXss();
	void DumpNXS(std::ostream& os) final;
	void DumpJXS(std::ostream& os) final;

private:
	enum NSXDATA { LENGTH2ndBLOCK=0, ZA=1, /*NES=2,*/ NTR=3, /*NR=4, NTRP=5, NPCR=7,*/ NT=14, PHOTOPROD=15 };
	enum JSXDATA { LONE=0, /*NU=1,*/ MTR=2, /*LQR=3, TYR=4,*/
					LSIG=5, SIGD=6, /*LAND=7, AND=8, LDLW=9,
					DLW=10, GPD=11, MTRP=12, LSIGP=13, SIGP=14,
					LANDP=15, ANDP=16, LDLWP=17, DLWP=18, YP=19,
					FIS=20,*/ END=21, /*LUNR=22, DNU=23, BDD=24,
					DNEDL=25, DNED=26*/};

	int nLENGTH2ndBLOCK, nZA, nNTR, nNT, nPHOTOPROD;
	int nLONE, nMTR, nLSIG, nSIGD, nEND;

	void initNXS() {
		nLENGTH2ndBLOCK = nxs_.at(static_cast<int>(NSXDATA::LENGTH2ndBLOCK));
		nZA = nxs_.at(static_cast<int>(NSXDATA::ZA));
		nNTR = nxs_.at(static_cast<int>(NSXDATA::NTR));
		nNT = nxs_.at(static_cast<int>(NSXDATA::NT));
		nPHOTOPROD = nxs_.at(static_cast<int>(NSXDATA::PHOTOPROD));
	}
	void initJXS() {
		nLONE = jxs_.at(static_cast<int>(JSXDATA::LONE));
		nMTR = jxs_.at(static_cast<int>(JSXDATA::MTR));
		nLSIG = jxs_.at(static_cast<int>(JSXDATA::LSIG));
		nSIGD = jxs_.at(static_cast<int>(JSXDATA::SIGD));
		nEND = jxs_.at(static_cast<int>(JSXDATA::END));
	}
};

}  // end namespace ace

#endif // NEUTRONDOSIMETRYFILE_HPP
