#include "mt.hpp"

#include <iostream>
#include <regex>
namespace {

// ReactionをキーにしたReaction説明文字列のマップを取得
const std::map<ace::Reaction, std::string> &getMtInfoMap()
{
	static const std::map<ace::Reaction, std::string> mtMap_{
		{ace::Reaction::NOT_DEFINED,         "Not reaction"},
		{ace::Reaction::TOTAL,          "MT1: Total"},
		{ace::Reaction::ELASTIC,         "MT2: Elastic"},
		{ace::Reaction::NONELASTIC,      "MT3: Non-Elastic"},
		{ace::Reaction::PRODUCT_1N,      "MT4: Production of a neutron"},
		{ace::Reaction::ANYTHING_OTHER,  "MT5: (, anything other)"},
		{ace::Reaction::TOTAL_CONTINUUM, "MT10: Total continuum reaction"},
		{ace::Reaction::PRODUCT_2N1D,    "MT11: (, 2nd)"},
		{ace::Reaction::PRODUCT_2N,      "MT16: (, 2n)"},
		{ace::Reaction::PRODUCT_3N,      "MT17: (, 3n)"},
		{ace::Reaction::FISSION,                  "MT18: (, fission)"},
		{ace::Reaction::N_FIRST_CHANCE_FISSON,   "MT19: (n,f)"},
		{ace::Reaction::N_SECOND_CHANCE_FISSION, "MT20: (n,nf)"},
		{ace::Reaction::N_THIRD_CHANCE_FISSION,  "MT21: (n,2nf)"},
		{ace::Reaction::PRODUCT_1N1A,   "MT22: (, na)"},
		{ace::Reaction::N_PRODUCT_1N3A, "MT23: (n, n3a)"},
		{ace::Reaction::PRODUCT_2N1A,   "MT24: (n, 2na)"},
		{ace::Reaction::PRODUCT_3N1A,   "MT25: (n, 3na)"},
		{ace::Reaction::N_ABSORPTION,   "MT27: absorption (MT18+MT102)"},  // sum of FISSION and CAPTURE
		{ace::Reaction::PRODUCT_1N1P,   "MT28: (, np)"},
		{ace::Reaction::PRODUCT_1N2A,   "MT29: (, n2a)"},
		{ace::Reaction::PRODUCT_2N2A,   "MT30: (, 2n2a)"},
		{ace::Reaction::PRODUCT_1N1D,   "MT32: (, nd)"},
		{ace::Reaction::PRODUCT_1N1T,   "MT33: (, nt)"},
		{ace::Reaction::PRODUCT_1NHe3,  "MT34: (, nHe3)"},
		{ace::Reaction::PRODUCT_1N1D2A, "MT35: (, nd2a)"},
		{ace::Reaction::PRODUCT_1N1T2A, "MT36: (, nt2a)"},
		{ace::Reaction::PRODUCT_4N,     "MT37: (, 4n)"},
		{ace::Reaction::N_FOURTH_CHANCE_FISSION, "MT38: (n, 3nf)"},
		{ace::Reaction::PRODUCT_2N1P,   "MT41: (, 2np)"},
		{ace::Reaction::PRODUCT_3N1P,   "MT42: (, 3np)"},
		{ace::Reaction::PRODUCT_1N2P,   "MT44: (, n2p)"},
		{ace::Reaction::PRODUCT_1N1P1A, "MT45: (, npa)"},
		{ace::Reaction::PRODUCT_1Nground, "MT50: (, n)ground"},
		{ace::Reaction::PRODUCT_1Nex1,  "MT51: (, n') 1st"},
		{ace::Reaction::PRODUCT_1Nex2,  "MT52: (, n') 2nd"},
		{ace::Reaction::PRODUCT_1Nex3,  "MT53: (, n') 3rd"},
		{ace::Reaction::PRODUCT_1Nex4,  "MT54: (, n') 4th"},
		{ace::Reaction::PRODUCT_1Nex5,  "MT55: (, n') 5th"},
		{ace::Reaction::PRODUCT_1Nex6,  "MT56: (, n') 6th"},
		{ace::Reaction::PRODUCT_1Nex7,  "MT57: (, n') 7th"},
		{ace::Reaction::PRODUCT_1Nex8,  "MT58: (, n') 8th"},
		{ace::Reaction::PRODUCT_1Nex9,  "MT59: (, n') 9th"},
		{ace::Reaction::PRODUCT_1Nex10, "MT60: (, n') 10th"},
		{ace::Reaction::PRODUCT_1Nex11, "MT61: (, n') 11th"},
		{ace::Reaction::PRODUCT_1Nex12, "MT62: (, n') 12th"},
		{ace::Reaction::PRODUCT_1Nex13, "MT63: (, n') 13th"},
		{ace::Reaction::PRODUCT_1Nex14, "MT64: (, n') 14th"},
		{ace::Reaction::PRODUCT_1Nex15, "MT65: (, n') 15th"},
		{ace::Reaction::PRODUCT_1Nex16, "MT66: (, n') 16th"},
		{ace::Reaction::PRODUCT_1Nex17, "MT67: (, n') 17th"},
		{ace::Reaction::PRODUCT_1Nex18, "MT68: (, n') 18th"},
		{ace::Reaction::PRODUCT_1Nex19, "MT69: (, n') 19th"},
		{ace::Reaction::PRODUCT_1Nex20, "MT70: (, n') 20th"},
		{ace::Reaction::PRODUCT_1Nex21, "MT71: (, n') 21st"},
		{ace::Reaction::PRODUCT_1Nex22, "MT72: (, n') 22nd"},
		{ace::Reaction::PRODUCT_1Nex23, "MT73: (, n') 23rd"},
		{ace::Reaction::PRODUCT_1Nex24, "MT74: (, n') 24th"},
		{ace::Reaction::PRODUCT_1Nex25, "MT75: (, n') 25th"},
		{ace::Reaction::PRODUCT_1Nex26, "MT76: (, n') 26th"},
		{ace::Reaction::PRODUCT_1Nex27, "MT77: (, n') 27th"},
		{ace::Reaction::PRODUCT_1Nex28, "MT78: (, n') 28th"},
		{ace::Reaction::PRODUCT_1Nex29, "MT79: (, n') 29th"},
		{ace::Reaction::PRODUCT_1Nex30, "MT80: (, n') 30th"},
		{ace::Reaction::PRODUCT_1Nex31, "MT81: (, n') 31st"},
		{ace::Reaction::PRODUCT_1Nex32, "MT82: (, n') 32nd"},
		{ace::Reaction::PRODUCT_1Nex33, "MT83: (, n') 33rd"},
		{ace::Reaction::PRODUCT_1Nex34, "MT84: (, n') 34th"},
		{ace::Reaction::PRODUCT_1Nex35, "MT85: (, n') 35th"},
		{ace::Reaction::PRODUCT_1Nex36, "MT86: (, n') 36th"},
		{ace::Reaction::PRODUCT_1Nex37, "MT87: (, n') 37th"},
		{ace::Reaction::PRODUCT_1Nex38, "MT88: (, n') 38th"},
		{ace::Reaction::PRODUCT_1Nex39, "MT89: (, n') 39th"},
		{ace::Reaction::PRODUCT_1Nex40, "MT90: (, n') 40th"},
		{ace::Reaction::PRODUCT_1Ncont, "MT91: (, n') cont"},
		{ace::Reaction::N_DISAPPEARANCE, "MT101: neutron disappearance, sum of 102-117"},
		{ace::Reaction::CAPTURE,         "MT102: (, g)"},
		{ace::Reaction::PRODUCT_1P,   "MT103: (, p)"},
		{ace::Reaction::PRODUCT_1D,   "MT104: (, d)"},
		{ace::Reaction::PRODUCT_1T,   "MT105: (, t)"},
		{ace::Reaction::PRODUCT_1He3, "MT106: (, He3)"},
		{ace::Reaction::PRODUCT_1A,   "MT107: (, a)"},
		{ace::Reaction::PRODUCT_2A,   "MT108: (, 2a)"},
		{ace::Reaction::PRODUCT_3A,   "MT109: (, 3a)"},
		{ace::Reaction::PRODUCT_2P,   "MT111: (, 2p)"},
		{ace::Reaction::PRODUCT_1P1A, "MT112: (, pa)"},
		{ace::Reaction::PRODUCT_1T2A, "MT113: (, t2a)"},
		{ace::Reaction::PRODUCT_1D2A, "MT114: (, d2a)"},
		{ace::Reaction::PRODUCT_1P1D, "MT115: (, pd)"},
		{ace::Reaction::PRODUCT_1P1T, "MT116: (, pt)"},
		{ace::Reaction::PRODUCT_1D1A, "MT117: (, da)"},
		{ace::Reaction::PRODUCT_N,    "MT201: (, Xn)"},
		{ace::Reaction::PRODUCT_G,    "MT202: (, Xg)"},
		{ace::Reaction::PRODUCT_P,    "MT203: (, Xp)"},
		{ace::Reaction::PRODUCT_D,    "MT204: (, Xd)"},
		{ace::Reaction::PRODUCT_T,    "MT205: (, Xt)"},
		{ace::Reaction::PRODUCT_He3,  "MT206: (, XHe3)"},
		{ace::Reaction::PRODUCT_A,    "MT207: (, Xa)"},
		{ace::Reaction::PRODUCT_Pip,  "MT208: (, XPi+)"},
		{ace::Reaction::PRODUCT_Pi0,  "MT209: (, XPi0"},
		{ace::Reaction::PRODUCT_Pim,  "MT210: (, XPi-)"},
		{ace::Reaction::PRODUCT_Mup,  "MT211: (, XMu+)"},
		{ace::Reaction::PRODUCT_Mum,  "MT212: (, XMu-)"},
		{ace::Reaction::PRODUCT_Kap,  "MT213: (, XK+)"},
		{ace::Reaction::PRODUCT_Ka0l, "MT214: (, XK0long)"},
		{ace::Reaction::PRODUCT_Ka0s, "MT215: (, XK0short)"},
		{ace::Reaction::PRODUCT_Kam,  "MT216: (, XK-)"},
		{ace::Reaction::PRODUCT_aP,   "MT217: (, anti-p)"},
		{ace::Reaction::PRODUCT_aN,   "MT218: (, anti-n)"},
		{ace::Reaction::TOTAL_HEATING_NUMBER,	          "MT301: Total heating number"},
		{ace::Reaction::ELASTIC_HEATING_NUMBER,         "MT302: Elastig heating number"},
		{ace::Reaction::NONELASTIC_HEATING_NUMBER,      "MT303: Non-elastic heating number"},
		{ace::Reaction::N_DAMAGE_TOTAL, 	              "MT444: Neutron total damage"},
		{ace::Reaction::N_DAMAGE_ELASTIC,               "MT445: Neutron elastic damage"},
		{ace::Reaction::N_DAMAGE_INELASTIC,             "MT446: Neutron inelastic damage"},
		{ace::Reaction::N_DAMAGE_DISAPPEARANCE,         "MT447: Neutron disappearance damage"},
		{ace::Reaction::TOTAL_CHARGEDPARTICLE_STOPPING, "MT500:Total charged-particle stopping power"},
		{ace::Reaction::TOTAL_PHOTON_INTERACTION,       "MT501: Total photon interaction"},
		{ace::Reaction::PHTON_COHERENT,                  "MT502: Photon coherent"},
		{ace::Reaction::PHOTON_INCOHERENT,               "MT504: Photon incoherent"},
		{ace::Reaction::IMAGINARY_SCATTERING_FACTOR,    "MT505: Imaginary scattering factor"},
		{ace::Reaction::REAL_SCATTERING_FACTOR,         "MT506: Real scattering factor"},
		{ace::Reaction::PARI_PROD_ELECTROM_FIELD,       "MT515: Pair production, electron field"},
		{ace::Reaction::PAIR_PROD_TOTAL,			      "MT516: Pair production, total"},
		{ace::Reaction::PARI_PROD_NUCLEAR_FIELD,        "MT517: Pair production, nuclear field"},
		{ace::Reaction::PHTOELECTRIC_ABSORPTION,        "MT522: Photoelectric absorption"},
		{ace::Reaction::PHOTOEXCITATION,                "MT523: Photo-excitation cross section"},
		{ace::Reaction::ELECTROATOMIC_SCATTERING,       "MT526: Electro-atomic scattering"},
		{ace::Reaction::ELECTROATOMIC_BREMS,            "MT527: Electro-atomic bremsstrahlung"},
		{ace::Reaction::ELECTROATOMIC_EXCITATION,       "MT528: Electro-atomic excitation"},
		{ace::Reaction::ATOMIC_RELAXATION,              "MT533: Atomic relaxation data (photonuclear in ver.5)"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_K ,  "MT534: K subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_L1,  "MT535: L1 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_L2,  "MT536: L2 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_L3,  "MT537: L3 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_M1,  "MT538: M1 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_M2,  "MT539: M2 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_M3,  "MT540: M3 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_M4,  "MT541: M4 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_M5,  "MT542: M5 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_N1,  "MT543: N1 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_N2,  "MT544: N2 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_N3,  "MT545: N3 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_N4,  "MT546: N4 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_N5,  "MT547: N5 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_N6,  "MT548: N6 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_N7,  "MT549: N7 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_O1,  "MT550: O1 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_O2,  "MT551: O2 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_O3,  "MT552: O3 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_O4,  "MT553: O4 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_O5,  "MT554: O5 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_O6,  "MT555: O6 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_O7,  "MT536: O7 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_O8,  "MT557: O8 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_O9,  "MT558: O9 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_P1,  "MT559: P1 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_P2,  "MT560: P2 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_P3,  "MT561: P3 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_P4,  "MT562: P4 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_P5,  "MT563: P5 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_P6,  "MT564: P6 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_P7,  "MT565: P7 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_P8,  "MT566: P8 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_P9,  "MT567: P9 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_P10, "MT568: P10 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_P11, "MT569: P11 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_Q1,  "MT570: Q1 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_Q2,  "MT571: Q2 subshell reaction"},
		{ace::Reaction::PHOTOELECTRIC_SUBSHELL_Q3,  "MT572: Q3 subshell reaction"},
		{ace::Reaction::PRODUCT_Pex0,   "MT600: proton production, residual grand"},
		{ace::Reaction::PRODUCT_Pex1,   "MT601: proton production, residual  1st excited"},
		{ace::Reaction::PRODUCT_Pex2,   "MT602: proton production, residual  2nd excited"},
		{ace::Reaction::PRODUCT_Pex3,   "MT603: proton production, residual  3rd excited"},
		{ace::Reaction::PRODUCT_Pex4,   "MT604: proton production, residual  4th excited"},
		{ace::Reaction::PRODUCT_Pex5,   "MT605: proton production, residual  5th excited"},
		{ace::Reaction::PRODUCT_Pex6,   "MT606: proton production, residual  6th excited"},
		{ace::Reaction::PRODUCT_Pex7,   "MT607: proton production, residual  7th excited"},
		{ace::Reaction::PRODUCT_Pex8,   "MT608: proton production, residual  8th excited"},
		{ace::Reaction::PRODUCT_Pex9,   "MT609: proton production, residual  9th excited"},
		{ace::Reaction::PRODUCT_Pex10,  "MT610: proton production, residual 10th excited"},
		{ace::Reaction::PRODUCT_Pex11,  "MT611: proton production, residual 11th excited"},
		{ace::Reaction::PRODUCT_Pex12,  "MT612: proton production, residual 12th excited"},
		{ace::Reaction::PRODUCT_Pex13,  "MT613: proton production, residual 13th excited"},
		{ace::Reaction::PRODUCT_Pex14,  "MT614: proton production, residual 14th excited"},
		{ace::Reaction::PRODUCT_Pex15,  "MT615: proton production, residual 15th excited"},
		{ace::Reaction::PRODUCT_Pex16,  "MT616: proton production, residual 16th excited"},
		{ace::Reaction::PRODUCT_Pex17,  "MT617: proton production, residual 17th excited"},
		{ace::Reaction::PRODUCT_Pex18,  "MT618: proton production, residual 18th excited"},
		{ace::Reaction::PRODUCT_Pex19,  "MT619: proton production, residual 19th excited"},
		{ace::Reaction::PRODUCT_Pex20,  "MT620: proton production, residual 20th excited"},
		{ace::Reaction::PRODUCT_Pex21,  "MT621: proton production, residual 21th excited"},
		{ace::Reaction::PRODUCT_Pex22,  "MT622: proton production, residual 22th excited"},
		{ace::Reaction::PRODUCT_Pex23,  "MT623: proton production, residual 23th excited"},
		{ace::Reaction::PRODUCT_Pex24,  "MT624: proton production, residual 24th excited"},
		{ace::Reaction::PRODUCT_Pex25,  "MT625: proton production, residual 25th excited"},
		{ace::Reaction::PRODUCT_Pex26,  "MT626: proton production, residual 26th excited"},
		{ace::Reaction::PRODUCT_Pex27,  "MT627: proton production, residual 27th excited"},
		{ace::Reaction::PRODUCT_Pex28,  "MT628: proton production, residual 28th excited"},
		{ace::Reaction::PRODUCT_Pex29,  "MT629: proton production, residual 29th excited"},
		{ace::Reaction::PRODUCT_Pex30,  "MT630: proton production, residual 30th excited"},
		{ace::Reaction::PRODUCT_Pex31,  "MT631: proton production, residual 31th excited"},
		{ace::Reaction::PRODUCT_Pex32,  "MT632: proton production, residual 32th excited"},
		{ace::Reaction::PRODUCT_Pex33,  "MT633: proton production, residual 33th excited"},
		{ace::Reaction::PRODUCT_Pex34,  "MT634: proton production, residual 34th excited"},
		{ace::Reaction::PRODUCT_Pex35,  "MT635: proton production, residual 35th excited"},
		{ace::Reaction::PRODUCT_Pex36,  "MT636: proton production, residual 36th excited"},
		{ace::Reaction::PRODUCT_Pex37,  "MT637: proton production, residual 37th excited"},
		{ace::Reaction::PRODUCT_Pex38,  "MT638: proton production, residual 38th excited"},
		{ace::Reaction::PRODUCT_Pex39,  "MT639: proton production, residual 39th excited"},
		{ace::Reaction::PRODUCT_Pex40,  "MT640: proton production, residual 40th excited"},
		{ace::Reaction::PRODUCT_Pex41,  "MT641: proton production, residual 41th excited"},
		{ace::Reaction::PRODUCT_Pex42,  "MT642: proton production, residual 42th excited"},
		{ace::Reaction::PRODUCT_Pex43,  "MT643: proton production, residual 43th excited"},
		{ace::Reaction::PRODUCT_Pex44,  "MT644: proton production, residual 44th excited"},
		{ace::Reaction::PRODUCT_Pex45,  "MT645: proton production, residual 45th excited"},
		{ace::Reaction::PRODUCT_Pex46,  "MT646: proton production, residual 46th excited"},
		{ace::Reaction::PRODUCT_Pex47,  "MT647: proton production, residual 47th excited"},
		{ace::Reaction::PRODUCT_Pex48,  "MT648: proton production, residual 48th excited"},
		{ace::Reaction::PRODUCT_Pcont,  "MT649: proton production, residual cont excited"},
		{ace::Reaction::PRODUCT_Dex0,   "MT650: deuteron production, residual grand"},
		{ace::Reaction::PRODUCT_Dex1,   "MT651: deuteron production, residual  1st"},
		{ace::Reaction::PRODUCT_Dex2,   "MT652: deuteron production, residual  2nd"},
		{ace::Reaction::PRODUCT_Dex3,   "MT653: deuteron production, residual  3rd"},
		{ace::Reaction::PRODUCT_Dex4,   "MT654: deuteron production, residual  4th"},
		{ace::Reaction::PRODUCT_Dex5,   "MT655: deuteron production, residual  5th"},
		{ace::Reaction::PRODUCT_Dex6,   "MT656: deuteron production, residual  6th"},
		{ace::Reaction::PRODUCT_Dex7,   "MT657: deuteron production, residual  7th"},
		{ace::Reaction::PRODUCT_Dex8,   "MT658: deuteron production, residual  8th"},
		{ace::Reaction::PRODUCT_Dex9,   "MT659: deuteron production, residual  9th"},
		{ace::Reaction::PRODUCT_Dex10,  "MT660: deuteron production, residual 10th"},
		{ace::Reaction::PRODUCT_Dex11,  "MT661: deuteron production, residual 11st"},
		{ace::Reaction::PRODUCT_Dex12,  "MT662: deuteron production, residual 12nd"},
		{ace::Reaction::PRODUCT_Dex13,  "MT663: deuteron production, residual 13rd"},
		{ace::Reaction::PRODUCT_Dex14,  "MT664: deuteron production, residual 14th"},
		{ace::Reaction::PRODUCT_Dex15,  "MT665: deuteron production, residual 15th"},
		{ace::Reaction::PRODUCT_Dex16,  "MT666: deuteron production, residual 16th"},
		{ace::Reaction::PRODUCT_Dex17,  "MT667: deuteron production, residual 17th"},
		{ace::Reaction::PRODUCT_Dex18,  "MT668: deuteron production, residual 18th"},
		{ace::Reaction::PRODUCT_Dex19,  "MT669: deuteron production, residual 19th"},
		{ace::Reaction::PRODUCT_Dex20,  "MT670: deuteron production, residual 20th"},
		{ace::Reaction::PRODUCT_Dex21,  "MT671: deuteron production, residual 21st"},
		{ace::Reaction::PRODUCT_Dex22,  "MT672: deuteron production, residual 22nd"},
		{ace::Reaction::PRODUCT_Dex23,  "MT673: deuteron production, residual 23rd"},
		{ace::Reaction::PRODUCT_Dex24,  "MT674: deuteron production, residual 24th"},
		{ace::Reaction::PRODUCT_Dex25,  "MT675: deuteron production, residual 25th"},
		{ace::Reaction::PRODUCT_Dex26,  "MT676: deuteron production, residual 26th"},
		{ace::Reaction::PRODUCT_Dex27,  "MT677: deuteron production, residual 27th"},
		{ace::Reaction::PRODUCT_Dex28,  "MT678: deuteron production, residual 28th"},
		{ace::Reaction::PRODUCT_Dex29,  "MT679: deuteron production, residual 29th"},
		{ace::Reaction::PRODUCT_Dex30,  "MT680: deuteron production, residual 30th"},
		{ace::Reaction::PRODUCT_Dex31,  "MT681: deuteron production, residual 31st"},
		{ace::Reaction::PRODUCT_Dex32,  "MT682: deuteron production, residual 32nd"},
		{ace::Reaction::PRODUCT_Dex33,  "MT683: deuteron production, residual 33rd"},
		{ace::Reaction::PRODUCT_Dex34,  "MT684: deuteron production, residual 34th"},
		{ace::Reaction::PRODUCT_Dex35,  "MT685: deuteron production, residual 35th"},
		{ace::Reaction::PRODUCT_Dex36,  "MT686: deuteron production, residual 36th"},
		{ace::Reaction::PRODUCT_Dex37,  "MT687: deuteron production, residual 37th"},
		{ace::Reaction::PRODUCT_Dex38,  "MT688: deuteron production, residual 38th"},
		{ace::Reaction::PRODUCT_Dex39,  "MT689: deuteron production, residual 39th"},
		{ace::Reaction::PRODUCT_Dex40,  "MT690: deuteron production, residual 40th"},
		{ace::Reaction::PRODUCT_Dex41,  "MT691: deuteron production, residual 41st"},
		{ace::Reaction::PRODUCT_Dex42,  "MT692: deuteron production, residual 42nd"},
		{ace::Reaction::PRODUCT_Dex43,  "MT693: deuteron production, residual 43rd"},
		{ace::Reaction::PRODUCT_Dex44,  "MT694: deuteron production, residual 44th"},
		{ace::Reaction::PRODUCT_Dex45,  "MT695: deuteron production, residual 45th"},
		{ace::Reaction::PRODUCT_Dex46,  "MT696: deuteron production, residual 46th"},
		{ace::Reaction::PRODUCT_Dex47,  "MT697: deuteron production, residual 47th"},
		{ace::Reaction::PRODUCT_Dex48,  "MT698: deuteron production, residual 48th"},
		{ace::Reaction::PRODUCT_Dcont,  "MT699: deuteron production, residual cont"},
		{ace::Reaction::PRODUCT_Tex0,   "MT700: triton production, residual grand"},
		{ace::Reaction::PRODUCT_Tex1,   "MT701: triton production, residual  1st"},
		{ace::Reaction::PRODUCT_Tex2,   "MT702: triton production, residual  2nd"},
		{ace::Reaction::PRODUCT_Tex3,   "MT703: triton production, residual  3rd"},
		{ace::Reaction::PRODUCT_Tex4,   "MT704: triton production, residual  4th"},
		{ace::Reaction::PRODUCT_Tex5,   "MT705: triton production, residual  5th"},
		{ace::Reaction::PRODUCT_Tex6,   "MT706: triton production, residual  6th"},
		{ace::Reaction::PRODUCT_Tex7,   "MT707: triton production, residual  7th"},
		{ace::Reaction::PRODUCT_Tex8,   "MT708: triton production, residual  8th"},
		{ace::Reaction::PRODUCT_Tex9,   "MT709: triton production, residual  9th"},
		{ace::Reaction::PRODUCT_Tex10,  "MT710: triton production, residual 10th"},
		{ace::Reaction::PRODUCT_Tex11,  "MT711: triton production, residual 11st"},
		{ace::Reaction::PRODUCT_Tex12,  "MT712: triton production, residual 12nd"},
		{ace::Reaction::PRODUCT_Tex13,  "MT713: triton production, residual 13rd"},
		{ace::Reaction::PRODUCT_Tex14,  "MT714: triton production, residual 14th"},
		{ace::Reaction::PRODUCT_Tex15,  "MT715: triton production, residual 15th"},
		{ace::Reaction::PRODUCT_Tex16,  "MT716: triton production, residual 16th"},
		{ace::Reaction::PRODUCT_Tex17,  "MT717: triton production, residual 17th"},
		{ace::Reaction::PRODUCT_Tex18,  "MT718: triton production, residual 18th"},
		{ace::Reaction::PRODUCT_Tex19,  "MT719: triton production, residual 19th"},
		{ace::Reaction::PRODUCT_Tex20,  "MT720: triton production, residual 20th"},
		{ace::Reaction::PRODUCT_Tex21,  "MT721: triton production, residual 21st"},
		{ace::Reaction::PRODUCT_Tex22,  "MT722: triton production, residual 22nd"},
		{ace::Reaction::PRODUCT_Tex23,  "MT723: triton production, residual 23rd"},
		{ace::Reaction::PRODUCT_Tex24,  "MT724: triton production, residual 24th"},
		{ace::Reaction::PRODUCT_Tex25,  "MT725: triton production, residual 25th"},
		{ace::Reaction::PRODUCT_Tex26,  "MT726: triton production, residual 26th"},
		{ace::Reaction::PRODUCT_Tex27,  "MT727: triton production, residual 27th"},
		{ace::Reaction::PRODUCT_Tex28,  "MT728: triton production, residual 28th"},
		{ace::Reaction::PRODUCT_Tex29,  "MT729: triton production, residual 29th"},
		{ace::Reaction::PRODUCT_Tex30,  "MT730: triton production, residual 30th"},
		{ace::Reaction::PRODUCT_Tex31,  "MT731: triton production, residual 31st"},
		{ace::Reaction::PRODUCT_Tex32,  "MT732: triton production, residual 32nd"},
		{ace::Reaction::PRODUCT_Tex33,  "MT733: triton production, residual 33rd"},
		{ace::Reaction::PRODUCT_Tex34,  "MT734: triton production, residual 34th"},
		{ace::Reaction::PRODUCT_Tex35,  "MT735: triton production, residual 35th"},
		{ace::Reaction::PRODUCT_Tex36,  "MT736: triton production, residual 36th"},
		{ace::Reaction::PRODUCT_Tex37,  "MT737: triton production, residual 37th"},
		{ace::Reaction::PRODUCT_Tex38,  "MT738: triton production, residual 38th"},
		{ace::Reaction::PRODUCT_Tex39,  "MT739: triton production, residual 39th"},
		{ace::Reaction::PRODUCT_Tex40,  "MT740: triton production, residual 40th"},
		{ace::Reaction::PRODUCT_Tex41,  "MT741: triton production, residual 41st"},
		{ace::Reaction::PRODUCT_Tex42,  "MT742: triton production, residual 42nd"},
		{ace::Reaction::PRODUCT_Tex43,  "MT743: triton production, residual 43rd"},
		{ace::Reaction::PRODUCT_Tex44,  "MT744: triton production, residual 44th"},
		{ace::Reaction::PRODUCT_Tex45,  "MT745: triton production, residual 45th"},
		{ace::Reaction::PRODUCT_Tex46,  "MT746: triton production, residual 46th"},
		{ace::Reaction::PRODUCT_Tex47,  "MT747: triton production, residual 47th"},
		{ace::Reaction::PRODUCT_Tex48,  "MT748: triton production, residual 48th"},
		{ace::Reaction::PRODUCT_Tcont,  "MT749: triton production, residual cont"},
		{ace::Reaction::PRODUCT_He3ex0,   "MT750: He-3 production, residual grand"},
		{ace::Reaction::PRODUCT_He3ex1,   "MT751: He-3 production, residual  1st"},
		{ace::Reaction::PRODUCT_He3ex2,   "MT752: He-3 production, residual  2nd"},
		{ace::Reaction::PRODUCT_He3ex3,   "MT753: He-3 production, residual  3rd"},
		{ace::Reaction::PRODUCT_He3ex4,   "MT754: He-3 production, residual  4th"},
		{ace::Reaction::PRODUCT_He3ex5,   "MT755: He-3 production, residual  5th"},
		{ace::Reaction::PRODUCT_He3ex6,   "MT756: He-3 production, residual  6th"},
		{ace::Reaction::PRODUCT_He3ex7,   "MT757: He-3 production, residual  7th"},
		{ace::Reaction::PRODUCT_He3ex8,   "MT758: He-3 production, residual  8th"},
		{ace::Reaction::PRODUCT_He3ex9,   "MT759: He-3 production, residual  9th"},
		{ace::Reaction::PRODUCT_He3ex10,  "MT760: He-3 production, residual 10th"},
		{ace::Reaction::PRODUCT_He3ex11,  "MT761: He-3 production, residual 11st"},
		{ace::Reaction::PRODUCT_He3ex12,  "MT762: He-3 production, residual 12nd"},
		{ace::Reaction::PRODUCT_He3ex13,  "MT763: He-3 production, residual 13rd"},
		{ace::Reaction::PRODUCT_He3ex14,  "MT764: He-3 production, residual 14th"},
		{ace::Reaction::PRODUCT_He3ex15,  "MT765: He-3 production, residual 15th"},
		{ace::Reaction::PRODUCT_He3ex16,  "MT766: He-3 production, residual 16th"},
		{ace::Reaction::PRODUCT_He3ex17,  "MT767: He-3 production, residual 17th"},
		{ace::Reaction::PRODUCT_He3ex18,  "MT768: He-3 production, residual 18th"},
		{ace::Reaction::PRODUCT_He3ex19,  "MT769: He-3 production, residual 19th"},
		{ace::Reaction::PRODUCT_He3ex20,  "MT770: He-3 production, residual 20th"},
		{ace::Reaction::PRODUCT_He3ex21,  "MT771: He-3 production, residual 21st"},
		{ace::Reaction::PRODUCT_He3ex22,  "MT772: He-3 production, residual 22nd"},
		{ace::Reaction::PRODUCT_He3ex23,  "MT773: He-3 production, residual 23rd"},
		{ace::Reaction::PRODUCT_He3ex24,  "MT774: He-3 production, residual 24th"},
		{ace::Reaction::PRODUCT_He3ex25,  "MT775: He-3 production, residual 25th"},
		{ace::Reaction::PRODUCT_He3ex26,  "MT776: He-3 production, residual 26th"},
		{ace::Reaction::PRODUCT_He3ex27,  "MT777: He-3 production, residual 27th"},
		{ace::Reaction::PRODUCT_He3ex28,  "MT778: He-3 production, residual 28th"},
		{ace::Reaction::PRODUCT_He3ex29,  "MT779: He-3 production, residual 29th"},
		{ace::Reaction::PRODUCT_He3ex30,  "MT780: He-3 production, residual 30th"},
		{ace::Reaction::PRODUCT_He3ex31,  "MT781: He-3 production, residual 31st"},
		{ace::Reaction::PRODUCT_He3ex32,  "MT782: He-3 production, residual 32nd"},
		{ace::Reaction::PRODUCT_He3ex33,  "MT783: He-3 production, residual 33rd"},
		{ace::Reaction::PRODUCT_He3ex34,  "MT784: He-3 production, residual 34th"},
		{ace::Reaction::PRODUCT_He3ex35,  "MT785: He-3 production, residual 35th"},
		{ace::Reaction::PRODUCT_He3ex36,  "MT786: He-3 production, residual 36th"},
		{ace::Reaction::PRODUCT_He3ex37,  "MT787: He-3 production, residual 37th"},
		{ace::Reaction::PRODUCT_He3ex38,  "MT788: He-3 production, residual 38th"},
		{ace::Reaction::PRODUCT_He3ex39,  "MT789: He-3 production, residual 39th"},
		{ace::Reaction::PRODUCT_He3ex40,  "MT790: He-3 production, residual 40th"},
		{ace::Reaction::PRODUCT_He3ex41,  "MT791: He-3 production, residual 41st"},
		{ace::Reaction::PRODUCT_He3ex42,  "MT792: He-3 production, residual 42nd"},
		{ace::Reaction::PRODUCT_He3ex43,  "MT793: He-3 production, residual 43rd"},
		{ace::Reaction::PRODUCT_He3ex44,  "MT794: He-3 production, residual 44th"},
		{ace::Reaction::PRODUCT_He3ex45,  "MT795: He-3 production, residual 45th"},
		{ace::Reaction::PRODUCT_He3ex46,  "MT796: He-3 production, residual 46th"},
		{ace::Reaction::PRODUCT_He3ex47,  "MT797: He-3 production, residual 47th"},
		{ace::Reaction::PRODUCT_He3ex48,  "MT798: He-3 production, residual 48th"},
		{ace::Reaction::PRODUCT_He3cont,  "MT799: He-3 production, residual cont"},
		{ace::Reaction::PRODUCT_Aex0,   "MT800: alpha production, residual grand"},
		{ace::Reaction::PRODUCT_Aex1,   "MT801: alpha production, residual  1st"},
		{ace::Reaction::PRODUCT_Aex2,   "MT802: alpha production, residual  2nd"},
		{ace::Reaction::PRODUCT_Aex3,   "MT803: alpha production, residual  3rd"},
		{ace::Reaction::PRODUCT_Aex4,   "MT804: alpha production, residual  4th"},
		{ace::Reaction::PRODUCT_Aex5,   "MT805: alpha production, residual  5th"},
		{ace::Reaction::PRODUCT_Aex6,   "MT806: alpha production, residual  6th"},
		{ace::Reaction::PRODUCT_Aex7,   "MT807: alpha production, residual  7th"},
		{ace::Reaction::PRODUCT_Aex8,   "MT808: alpha production, residual  8th"},
		{ace::Reaction::PRODUCT_Aex9,   "MT809: alpha production, residual  9th"},
		{ace::Reaction::PRODUCT_Aex10,  "MT810: alpha production, residual 10th"},
		{ace::Reaction::PRODUCT_Aex11,  "MT811: alpha production, residual 11st"},
		{ace::Reaction::PRODUCT_Aex12,  "MT812: alpha production, residual 12nd"},
		{ace::Reaction::PRODUCT_Aex13,  "MT813: alpha production, residual 13rd"},
		{ace::Reaction::PRODUCT_Aex14,  "MT814: alpha production, residual 14th"},
		{ace::Reaction::PRODUCT_Aex15,  "MT815: alpha production, residual 15th"},
		{ace::Reaction::PRODUCT_Aex16,  "MT816: alpha production, residual 16th"},
		{ace::Reaction::PRODUCT_Aex17,  "MT817: alpha production, residual 17th"},
		{ace::Reaction::PRODUCT_Aex18,  "MT818: alpha production, residual 18th"},
		{ace::Reaction::PRODUCT_Aex19,  "MT819: alpha production, residual 19th"},
		{ace::Reaction::PRODUCT_Aex20,  "MT820: alpha production, residual 20th"},
		{ace::Reaction::PRODUCT_Aex21,  "MT821: alpha production, residual 21st"},
		{ace::Reaction::PRODUCT_Aex22,  "MT822: alpha production, residual 22nd"},
		{ace::Reaction::PRODUCT_Aex23,  "MT823: alpha production, residual 23rd"},
		{ace::Reaction::PRODUCT_Aex24,  "MT824: alpha production, residual 24th"},
		{ace::Reaction::PRODUCT_Aex25,  "MT825: alpha production, residual 25th"},
		{ace::Reaction::PRODUCT_Aex26,  "MT826: alpha production, residual 26th"},
		{ace::Reaction::PRODUCT_Aex27,  "MT827: alpha production, residual 27th"},
		{ace::Reaction::PRODUCT_Aex28,  "MT828: alpha production, residual 28th"},
		{ace::Reaction::PRODUCT_Aex29,  "MT829: alpha production, residual 29th"},
		{ace::Reaction::PRODUCT_Aex30,  "MT830: alpha production, residual 30th"},
		{ace::Reaction::PRODUCT_Aex31,  "MT831: alpha production, residual 31st"},
		{ace::Reaction::PRODUCT_Aex32,  "MT832: alpha production, residual 32nd"},
		{ace::Reaction::PRODUCT_Aex33,  "MT833: alpha production, residual 33rd"},
		{ace::Reaction::PRODUCT_Aex34,  "MT834: alpha production, residual 34th"},
		{ace::Reaction::PRODUCT_Aex35,  "MT835: alpha production, residual 35th"},
		{ace::Reaction::PRODUCT_Aex36,  "MT836: alpha production, residual 36th"},
		{ace::Reaction::PRODUCT_Aex37,  "MT837: alpha production, residual 37th"},
		{ace::Reaction::PRODUCT_Aex38,  "MT838: alpha production, residual 38th"},
		{ace::Reaction::PRODUCT_Aex39,  "MT839: alpha production, residual 39th"},
		{ace::Reaction::PRODUCT_Aex40,  "MT840: alpha production, residual 40th"},
		{ace::Reaction::PRODUCT_Aex41,  "MT841: alpha production, residual 41st"},
		{ace::Reaction::PRODUCT_Aex42,  "MT842: alpha production, residual 42nd"},
		{ace::Reaction::PRODUCT_Aex43,  "MT843: alpha production, residual 43rd"},
		{ace::Reaction::PRODUCT_Aex44,  "MT844: alpha production, residual 44th"},
		{ace::Reaction::PRODUCT_Aex45,  "MT845: alpha production, residual 45th"},
		{ace::Reaction::PRODUCT_Aex46,  "MT846: alpha production, residual 46th"},
		{ace::Reaction::PRODUCT_Aex47,  "MT847: alpha production, residual 47th"},
		{ace::Reaction::PRODUCT_Aex48,  "MT848: alpha production, residual 48th"},
		{ace::Reaction::PRODUCT_Acont,  "MT849: alpha production, residual cont"}
	};

	return mtMap_;
}


// MTnnn のnnnを抽出し、intで返す。 不正な文字列の場合0を返す
int stripMt(const std::string &mtStr)
{
	static std::regex mtregex("[mM][tT]([0-9]+)");
	std::smatch sm;
	if(std::regex_search(mtStr, sm, mtregex)) {
		// ここでmt番号が妥当な数値化検証を入れる。
		int num = std::stoi(sm.str(1));
		//return num;
		return (getMtInfoMap().find(static_cast<ace::Reaction>(num))) != getMtInfoMap().end()? num: 0;
	} else {
		return 0;
	}
}
}  // end anonymous namespace

// Reactionへの変換。 mt>1000対応済み(toReaction(int)が対応している)
ace::Reaction ace::mt::toReaction(const std::string &mtStr)
{
	return toReaction(stripMt(mtStr));
}

// Reactionへの変換。mt>1000対応済み
ace::Reaction ace::mt::toReaction(int mtNum)
{
	if(mtNum > 1000) {
		std::cerr << "MT number > 1000 found. MT = " << mtNum << " is interpreted as " << mtNum%1000 << std::endl;
		return toReaction(mtNum%1000);
	}
	if(getMtInfoMap().find(static_cast<Reaction>(mtNum)) == getMtInfoMap().end()) {
		return Reaction::NOT_DEFINED;
	} else {
		return static_cast<Reaction>(mtNum);
	}
}


// 文字列"MT12"から12を返す。 MT>1000対応済み
int ace::mt::toNumber(const std::string &mtStr)
{
	int mtNum = stripMt(mtStr);
	if(mtNum > 1000) {
		mtNum = mtNum%1000;
		std::cerr << "MT > 1000 found, " << mtStr << " is treated as MT" << mtNum << std::endl;
	}
	return static_cast<int>(toReaction(mtNum));
}

// IRDF2002ではMT* = MT +1000*(10+LFS) としている。LFSは励起順位。
// なので番号が1000以上なら下3桁を用いる。


// 15から"MT15"を返す。 MT>1000対応済み(toReaction(int)が対応)
std::string ace::mt::toMtString(int mtNum)
{
	int existingMtNumber = static_cast<int>(toReaction(mtNum));
	if(existingMtNumber == 0) {
		//std::cerr << "MT not defined = " << mtNum << std::endl;
		//abort();
		return "MT_NOT_DEFINED" + std::to_string(mtNum);
	} else {
		return std::string("MT") + std::to_string(existingMtNumber);
	}


}

std::string ace::mt::description(ace::Reaction react)
{
	auto itr = getMtInfoMap().find(react);
	if(itr != getMtInfoMap().end()) {
		return itr->second;
	} else {
		return getMtInfoMap().at(Reaction::NOT_DEFINED);
	}
}

std::string ace::mt::description(const std::string &mtStr)
{
	return description(toReaction(mtStr));
}

std::string ace::mt::description(int mtNum)
{
	return description(toReaction(mtNum));
}

int ace::mt::toNumber(ace::Reaction reaction)
{
	return static_cast<int>(reaction);
}

std::string ace::mt::toMtString(ace::Reaction reaction)
{
	return ace::mt::toMtString(static_cast<int>(reaction));
}
