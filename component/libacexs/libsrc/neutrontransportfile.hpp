#ifndef NEUTRONTRANSPORTFILE_HPP
#define NEUTRONTRANSPORTFILE_HPP


#include "acefile.hpp"
#include "fissionneutrondata.hpp"

//class FissionNeutronData;
class PrecursorData;


namespace ace {


class SRCSHARED_EXPORT NeutronTransportFile : public AceFile
{
public:
	NeutronTransportFile(std::ifstream &ifs, const std::string& id, std::size_t startline);
	void readXss();
	void DumpNXS(std::ostream& os) final;
	void DumpJXS(std::ostream& os) final;

private:
	std::unique_ptr<FissionNeutronData> totalFissionNeutronData_;
	std::unique_ptr<FissionNeutronData> promptFissionNeutronData_;
	std::unique_ptr<FissionNeutronData> delayedFissionNeutronData_;  // 遅発核分裂中性子
	std::vector<PrecursorData> precursorData_; // precursor distribution data.

	enum class NXSPOS { LENGTH2ndBLOCK=0, ZA=1, NES=2, NTR=3, NR=4, NTRP=5, NPCR=7, NT=14, PHOTOPROD=15 };
	enum class JXSPOS { ESZ=0, NU=1, MTR=2, LQR=3, TYR=4,
					LSIG=5, SIG=6, LAND=7, AND=8, LDLW=9,
					DLW=10, GPD=11, MTRP=12, LSIGP=13, SIGP=14,
					LANDP=15, ANDP=16, LDLWP=17, DLWP=18, YP=19,
					FIS=20, END=21, LUNR=22, DNU=23, BDD=24,
					DNEDL=25, DNED=26};

	int LENGTH2ndBLOCK, ZA, NES, NTR, NR, NTRP, NPCR, NT, PHOTOPROD;
	int ESZ, NU, MTR, LQR, TYR, LSIG, SIG, LAND, AND, LDLW, DLW,
		GPD, MTRP, LSIGP, SIGP, LANDP, ANDP, LDLWP, DLWP, YP,
		FIS, END, LUNR, DNU, BDD, DNEDL, DNED;

	void initNXS();
	void initJXS();
};


}  // end namespace ace
#endif // NEUTRONTRANSPORTFILE_HPP
