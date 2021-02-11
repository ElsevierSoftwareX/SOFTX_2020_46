#ifndef PHOTOATOMICACEFILE_HPP
#define PHOTOATOMICACEFILE_HPP

#include "acefile.hpp"

namespace ace {


class SRCSHARED_EXPORT PhotoatomicAceFile : public AceFile
{
public:
	PhotoatomicAceFile(std::ifstream& ifs, const std::string& id, std::size_t startline);
	void readXss();
	void DumpNXS(std::ostream& os) final;
	void DumpJXS(std::ostream& os) final;

private:
	enum class NXSDATA {LENGTH2ndBLOCK=0, Z=1, NES=2, NFLO=3, NSH=4, NT=14, PHOTOPROD=15 };
	enum class JXSDATA {ESZG=0, JINC=1, JCOH=2, JFLO=3, LHNM=4,
					LNEPS=5, LBEPS=6, LPIPS=7, LSWD=8, SWD=9};

	int nZ, nNES, nNFLO, nNSH, nNT, nPHOTOPROD;
	int nESZG, nJINC, nJCOH, nJFLO, nLHNM, nLNEPS, nLBEPS, nLPIPS, nLSWD, nSWD;

	void initNXS();
	void initJXS();
};

}  // end namespace ace
#endif // PHOTOATOMICACEFILE_HPP
