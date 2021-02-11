#include "photoatomicfile.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "aceutils.hpp"
#include "utils/string_util.hpp"
#include "mt.hpp"
#include "acefile.hpp"
namespace {
const double SMALL = 1e-30;
}

template <class T>
void dumpVec(const std::vector<T>& vec) {
	for(size_t i = 0; i < vec.size(); i++) {
		std::cout << "i=" << i << ", v = " << vec.at(i) << std::endl;
	}
}

struct ComptonProfile {
	ComptonProfile(int jjdata, std::vector<double> mom, std::vector<double> p, std::vector<double> c):
		jj(jjdata), momentum(mom), pdf(p), cdf(c){;}
	int jj;
	std::vector<double> momentum;
	std::vector<double> pdf;
	std::vector<double> cdf;

	void dump(std::ostream& os, bool printHeader = true) {
		if(printHeader) {
			os << std::setw(10) << "#momentum" << std::setw(20) <<  "pdf" << std::setw(20) << "cdf" << std::endl;
		}
		for(size_t i = 0; i < momentum.size(); i++) {
			os << std::scientific;
			os << std::setprecision(3)  << std::setw(10) <<  momentum.at(i)
			   << std::setprecision(10) << std::setw(20) << pdf.at(i)
			   << std::setprecision(10) << std::setw(20) << cdf.at(i) << std::endl;
		}
	}
};

// AceFile(ifs, id)を呼ぶと対象ID核種あるいは最初の核種がseekされ、
// xss_, nxs_, jxs_が読み込まれる。
/*
 *  FIXME ここがACEファイル読み取りの核心。AceFileコンストラクタはヘッダの共通処理のみ。
 * ヘッダ(NXS, JXS含む)の量はたかがしれているから適当でよい。
 */
ace::PhotoatomicAceFile::PhotoatomicAceFile(std::ifstream &ifs, const std::string &id, std::size_t startline):
	AceFile(ifs, id, startline)
{
	initNXS();
	initJXS();
	readXss();
}

void ace::PhotoatomicAceFile::readXss()
{
	// ########################### ESGZ ブロック
	auto epoints = ace::getXssData<double>(xss_, nNES, nESZG);
	std::for_each(epoints.begin(), epoints.end(), [](double &val){val = std::exp(val);});
	auto incoherent     = ace::getXssData<double>(xss_, nNES, nESZG + nNES);  // 非干渉性
	auto coherent       = ace::getXssData<double>(xss_, nNES, nESZG + nNES*2);  // 干渉性
	auto photoelectric  = ace::getXssData<double>(xss_, nNES, nESZG + nNES*3);  // 光電吸収
	auto pairproduction = ace::getXssData<double>(xss_, nNES, nESZG + nNES*4);  //  対生成
	if(epoints.size() != incoherent.size()) {
		std::cerr << "Error: Number of energy points and that of incoherent xs are different." << std::endl;
	}
	std::vector<double> totalxs(epoints.size());
	for(size_t i = 0; i < incoherent.size(); i++) {
		incoherent.at(i) = std::exp(incoherent.at(i));
		coherent.at(i) = std::exp(coherent.at(i));
		photoelectric.at(i) = std::exp(photoelectric.at(i));
		pairproduction.at(i) = (std::abs(pairproduction.at(i)) < SMALL)? 0:std::exp(pairproduction.at(i));
		totalxs.at(i) = incoherent[i] + coherent[i] + photoelectric[i] + pairproduction[i];
	}
	XSmap_.emplace(ace::Reaction::TOTAL_PHOTON_INTERACTION,
					CrossSection(epoints, totalxs, ace::Reaction::TOTAL_PHOTON_INTERACTION, 0, 0, 0, 0));
	XSmap_.emplace(ace::Reaction::PHOTON_INCOHERENT,
					CrossSection(epoints, incoherent, ace::Reaction::PHOTON_INCOHERENT, 0, 0, 0, 0));
	XSmap_.emplace(ace::Reaction::PHTON_COHERENT,
					CrossSection(epoints, coherent, ace::Reaction::PHTON_COHERENT, 0, 0, 0, 0));
	XSmap_.emplace(ace::Reaction::PHTOELECTRIC_ABSORPTION,
					CrossSection(epoints, photoelectric, ace::Reaction::PHTOELECTRIC_ABSORPTION, 0, 0, 0, 0));
	XSmap_.emplace(ace::Reaction::PAIR_PROD_TOTAL,
					CrossSection(epoints, pairproduction, ace::Reaction::PAIR_PROD_TOTAL, 0, 0, 0, 0));

	// ########################### JINC ブロック
	/* The scattering functions for all elements are tabulated on a fixed set of v(I), where v is the momentum
	of the recoil electron (in inverse angstroms). The grid is: v(I), I=1,21 / 0. , .005 , .01 , .05 , .1 , .15 , .2,
	.3 , .4 , .5 , .6 , .7 , .8 , .9 , 1. , 1.5 , 2. , 3. , 4. , 5. , 8. /
	Linear-linear interpolation is assumed between adjacent v(I).
	The constants v(I) are stored in the VIC array in common block RBLDAT.*/
	const int NUM_FF_INCOHERENT = 21;
	auto incoherentRecoilElectronMomentums = ace::getXssData<double>(xss_, NUM_FF_INCOHERENT, nJINC);

	// ########################### JCOH ブロック
	/*The form factors for all elements are tabulated on a fixed set of v(I), where v is the momentum transfer
	of the recoil electron (in inverse angstroms). The grid is: v(I), I=1,55 / 0., .01, .02, .03, .04, .05, .06,
	.08, .10, .12, .15, .18, .20, .25, .30, .35, .40, .45, .50, .55, .60, .70, .80, .90, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5,
	1.6, 1.7, 1.8, 1.9, 2.0, 2.2, 2.4, 2.6, 2.8, 3.0, 3.2, 3.4, 3.6, 3.8, 4.0, 4.2, 4.4, 4.6, 4.8, 5.0, 5.2, 5.4, 5.6,
	5.8, 6.0 /
	The integrated form factors are tabulated on a fixed set of v(I)2, where the v(I) are those defined above.
	See LA-5157-MS2 for a description of the integrated form factors and the sampling technique used in
	MCNP. The constants v(I) are stored in the VCO array. The constants v(I)2 are stored in the WCO array.
	Both arrays are in common block RBLDAT.*/
	const int NUM_FF_COHERENT = 55;
	auto coherentIntegratedRecoilElectronMomentums = ace::getXssData<double>(xss_, NUM_FF_COHERENT, nJCOH);
	auto coherentRecoilElectronMomentums = ace::getXssData<double>(xss_, NUM_FF_COHERENT, nJCOH + NUM_FF_COHERENT);
	// NOTE データ出力

	// ############################### JFLOブロック
	auto edgeEnergies           = ace::getXssData<double>(xss_, nNFLO, nJFLO);
	auto relativeProbabilities  = ace::getXssData<double>(xss_, nNFLO, nJFLO + nNFLO);
	auto yields                 = ace::getXssData<double>(xss_, nNFLO, nJFLO + nNFLO*2);
	auto flourescentEnergies    = ace::getXssData<double>(xss_, nNFLO, nJFLO + nNFLO*3);
	//dumpVec(edgeEnergies);
//	std::ofstream fos ("Fluorescence.dat");
//	const int FPREC = 5, FWID = 15;
//	fos << "# Flourescence Data" << std::endl;
//	fos << "#";
//	fos << std::setw(FWID-1) << "EdgeEnergy" << std::setw(FWID)<< "Probability"
//		<< std::setw(FWID)<< "Yield" << std::setw(FWID) << "FloEnergy" << std::endl;
//	fos << std::scientific;
//	for(std::size_t i = 0; i < edgeEnergies.size(); i++) {
//		fos << std::setprecision(FPREC) << std::setw(FWID) << edgeEnergies.at(i)
//			<< std::setprecision(FPREC) << std::setw(FWID) << relativeProbabilities.at(i)
//			<< std::setprecision(FPREC) << std::setw(FWID) << yields.at(i)
//			<< std::setprecision(FPREC) << std::setw(FWID) << flourescentEnergies.at(i) << std::endl;
//	}

	// ############################### LHNM Block
	XSmap_.emplace(ace::Reaction::TOTAL_HEATING_NUMBER,
					CrossSection(epoints, ace::getXssData<double>(xss_, nNES, nLHNM),
									ace::Reaction::TOTAL_HEATING_NUMBER, 0, 0, 0, 0));

	// ###############################LNEPS Block
	std::vector<int> electronsPerShell = ace::getXssData<int>(xss_, nNSH, nLHNM + nNES);
	//dDebug() << electronsPerShell;
	// NOTE データ出力

	// ############################### LBEPS  LPIPS Block
	std::vector<double> bindingEnergiesPerShell;
	std::vector<double> interactionProbabilitiesPerShell;
	std::size_t currentPosition = nNSH + nLHNM + nNES;
	if(nNSH >= 1) {
		bindingEnergiesPerShell          = ace::getXssData<double>(xss_, nNSH, currentPosition);
		interactionProbabilitiesPerShell = ace::getXssData<double>(xss_, nNSH, currentPosition + nNSH);
	}
	// NOTE データ出力
//	std::cout << "interactionProb \n";
//	dDebug() << (interactionProbabilitiesPerShell);
	// ############################# LSWD Block
	std::vector<int> comptonProfileDataLocations = ace::getXssData<int>(xss_, nNSH, nLSWD);
//	std::cout << "comptonProfileDataLocations \n";
//	dDebug() << comptonProfileDataLocations;
	// NOTEデータ出力

	// ############################## SWD Block
	std::vector<ComptonProfile> comptonProfiles;
	for(std::size_t i = 0; i < comptonProfileDataLocations.size(); i++) {

		std::size_t refPosition = nSWD + comptonProfileDataLocations.at(i);
		int jj = ace::getXssData<int>(xss_, refPosition -1);
		int ne = ace::getXssData<int>(xss_, refPosition);
		auto momentumEntries = ace::getXssData<double>(xss_, ne, refPosition + 1);
		auto pdf = ace::getXssData<double>(xss_, ne, refPosition + ne + 1);
		auto cdf = ace::getXssData<double>(xss_, ne, refPosition + 2*ne + 1);
		comptonProfiles.push_back(ComptonProfile(jj, momentumEntries, pdf, cdf));
	}
//	std::ofstream os("ComptonProfile.dat");
//	os << "# Compton profiles" << std::endl;
//	for(std::size_t i = 0; i< comptonProfiles.size(); i++) {
//		os << "# Shell " << i+1 << std::endl;
//		comptonProfiles.at(i).dump(os, true);
//	}

	//checkEndOfData()
}

void ace::PhotoatomicAceFile::DumpNXS(std::ostream &os)
{
	assert(nxs_.size() == 16);
// ややこしい話だがACEフォーマットの解説で使われている配列は1から始まるのでずれる。
	os << "NXS( 1):Length of Second block data = " << nxs_.at(0) << std::endl;
	os << "NXS( 2):Z = " << nxs_.at(1) << std::endl;
	os << "NXS( 3):NES:Number of Energies = " << nxs_.at(2) << std::endl;
	os << "NXS( 4):NFLO:Length of the fluorescence data divided by 4 = " << nxs_.at(3) << std::endl;
	os << "NXS( 5):NSH:Number of electron shells = " << nxs_.at(4) << std::endl;
	if(nxs_.at(5)  != 0)	os << "NXS( 6) is not defined but used = " << nxs_.at(5) << std::endl;
	if(nxs_.at(6)  != 0) os << "NXS( 7) is not defined but used = " << nxs_.at(6) << std::endl;
	if(nxs_.at(7)  != 0) os << "NXS( 8) is not defined but used = " << nxs_.at(7) << std::endl;
	if(nxs_.at(8)  != 0) os << "NXS( 9) is not defined but used = " << nxs_.at(8) << std::endl;
	if(nxs_.at(9) != 0) os << "NXS(10) is not defined but used = " << nxs_.at(9) << std::endl;
	if(nxs_.at(10) != 0) os << "NXS(11) is not defined but used = " << nxs_.at(10) << std::endl;
	if(nxs_.at(11) != 0) os << "NXS(12) is not defined but used = " << nxs_.at(11) << std::endl;
	if(nxs_.at(12) != 0) os << "NXS(13) is not defined but used = " << nxs_.at(12) << std::endl;
	if(nxs_.at(13) != 0) os << "NXS(14) is not defined but used = " << nxs_.at(13) << std::endl;
	if(nxs_.at(14) != 0) os << "NXS(15) is not defined but used = " << nxs_.at(14) << std::endl;
	if(nxs_.at(15) != 0) os << "NXS(16) is not defined but used = " << nxs_.at(15) << std::endl;
}

void ace::PhotoatomicAceFile::DumpJXS(std::ostream &os)
{
	assert( jxs_.size() == JXS_SIZE );

	os << "JSX( 1):ESZ   location of energy table             = " << jxs_.at( 0) << std::endl;
	os << "JSX( 2):JINC  location of incoherent form factors = " << jxs_.at( 1) << std::endl;
	os << "JSX( 3):JCOH  location of coherent form factors   = " << jxs_.at( 2) << std::endl;
	os << "JSX( 4):JFLO  location of fluorescence data        = " << jxs_.at( 3) << std::endl;
	os << "JSX( 5):LHNM  location of heating number           = " << jxs_.at( 4) << std::endl;
	os << "JSX( 6):LNEPS location of the number of electrons per shell= " << jxs_.at( 5) << std::endl;
	os << "JSX( 7):LBEPS location of binding energy per shell = " << jxs_.at( 6) << std::endl;
	os << "JSX( 8):LPIPS location of probability of interaction per shell= " << jxs_.at( 7) << std::endl;
	os << "JSX( 9):LSWD  location of array of offsets to shellwise data = " << jxs_.at( 8) << std::endl;
	os << "JSX(10):SWD   location of shellwise data in PDF and CDF form= " << jxs_.at( 9) << std::endl;
	if(jxs_.at(10) != 0) os << "JSX(11): not defined but used = " << jxs_.at(10) << std::endl;
	if(jxs_.at(11) != 0) os << "JSX(12): not defined but used = " << jxs_.at(11) << std::endl;
	if(jxs_.at(12) != 0) os << "JSX(13): not defined but used = " << jxs_.at(12) << std::endl;
	if(jxs_.at(13) != 0) os << "JSX(14): not defined but used = " << jxs_.at(13) << std::endl;
	if(jxs_.at(14) != 0) os << "JSX(15): not defined but used = " << jxs_.at(14) << std::endl;
	if(jxs_.at(15) != 0) os << "JSX(16): not defined but used = " << jxs_.at(15) << std::endl;
	if(jxs_.at(16) != 0) os << "JSX(17): not defined but used = " << jxs_.at(16) << std::endl;
	if(jxs_.at(17) != 0) os << "JSX(18): not defined but used = " << jxs_.at(17) << std::endl;
	if(jxs_.at(18) != 0) os << "JSX(19): not defined but used = " << jxs_.at(18) << std::endl;
	if(jxs_.at(19) != 0) os << "JSX(20): not defined but used = " << jxs_.at(19) << std::endl;
	if(jxs_.at(20) != 0) os << "JSX(21): not defined but used = " << jxs_.at(20) << std::endl;
	if(jxs_.at(21) != 0) os << "JSX(22): not defined but used = " << jxs_.at(21) << std::endl;
	if(jxs_.at(22) != 0) os << "JSX(23): not defined but used = " << jxs_.at(22) << std::endl;
	if(jxs_.at(23) != 0) os << "JSX(24): not defined but used = " << jxs_.at(23) << std::endl;
	if(jxs_.at(24) != 0) os << "JSX(25): not defined but used = " << jxs_.at(24) << std::endl;
	if(jxs_.at(25) != 0) os << "JSX(26): not defined but used = " << jxs_.at(25) << std::endl;
	if(jxs_.at(26) != 0) os << "JSX(27): not defined but used = " << jxs_.at(26) << std::endl;
	if(jxs_.at(27) != 0) os << "JSX(28): not defined but used = " << jxs_.at(27) << std::endl;
	if(jxs_.at(28) != 0) os << "JSX(29): not defined but used = " << jxs_.at(28) << std::endl;
	if(jxs_.at(29) != 0) os << "JSX(30): not defined but used = " << jxs_.at(29) << std::endl;
	if(jxs_.at(30) != 0) os << "JSX(31): not defined but used = " << jxs_.at(30) << std::endl;
	if(jxs_.at(31) != 0) os << "JSX(32): not defined but used = " << jxs_.at(31) << std::endl;
}

void ace::PhotoatomicAceFile::initNXS() {
	nZ= nxs_.at(static_cast<int>(NXSDATA::Z));
	nNES = nxs_.at(static_cast<int>(NXSDATA::NES));
	nNFLO= nxs_.at(static_cast<int>(NXSDATA::NFLO));
	nNSH = nxs_.at(static_cast<int>(NXSDATA::NSH));
	nNT = nxs_.at(static_cast<int>(NXSDATA::NT));
	nPHOTOPROD= nxs_.at(static_cast<int>(NXSDATA::PHOTOPROD));
}

void ace::PhotoatomicAceFile::initJXS() {
	nESZG = jxs_.at(static_cast<int>(JXSDATA::ESZG));
	nJINC = jxs_.at(static_cast<int>(JXSDATA::JINC));
	nJCOH = jxs_.at(static_cast<int>(JXSDATA::JCOH));
	nJFLO = jxs_.at(static_cast<int>(JXSDATA::JFLO));
	nLHNM = jxs_.at(static_cast<int>(JXSDATA::LHNM));
	nLNEPS = jxs_.at(static_cast<int>(JXSDATA::LNEPS));
	nLBEPS = jxs_.at(static_cast<int>(JXSDATA::LBEPS));
	nLPIPS = jxs_.at(static_cast<int>(JXSDATA::LPIPS));
	nLSWD = jxs_.at(static_cast<int>(JXSDATA::LSWD));
	nSWD = jxs_.at(static_cast<int>(JXSDATA::SWD));
}
