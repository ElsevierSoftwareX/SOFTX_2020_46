#include "neutrondosimetryfile.hpp"

#include <cassert>
#include "aceutils.hpp"
#include "utils/string_util.hpp"
#include "mt.hpp"
#include "acefile.hpp"

ace::NeutronDosimetryFile::NeutronDosimetryFile(std::ifstream &ifs, const std::string &id, std::size_t startline):
	AceFile(ifs, id, startline)
{
	initNXS();
	initJXS();
//	DumpNXS(std::cout);
//	DumpJXS(std::cout);
	readXss();
}

void ace::NeutronDosimetryFile::readXss()
{
	// ########################### MTR ブロック
	long int MTl_len = nNTR;
	// MT番号vectorの読み込み
	std::vector<int> MTn_list = ace::getXssData<int>(xss_, MTl_len, 1);

	// ※1 このMT番号の出現順番は後々の断面積の並び順となるので極めて重要。
	// MT番号化する。
	std::vector<ace::Reaction> MT_list(MTn_list.size());
	for(std::size_t i = 0; i < MTn_list.size(); i++) {
		MT_list.at(i) = mt::toReaction(MTn_list.at(i));
		if(MT_list.at(i) == ace::Reaction::NOT_DEFINED) {
			std::cerr << "Warning: Not defined MT number detected = " << MTn_list.at(i) << std::endl;
		}
	}

	// ########################### LSIG ブロック
	std::vector<long int> Pos_XS_list    = ace::getXssData<long int>(xss_, MTl_len, nLSIG);

	assert(MTn_list.size() == Pos_XS_list.size());
	// データに間違いがなければこの位置リストは実は使わなくてもいける。
//	std::cout << "MT, pos list" << std::endl;
//	for(size_t i = 0; i < Pos_XS_list.size(); i++) {
//		std::cout << "MT=" << static_cast<int>(MT_list.at(i)) << ", pos=" << Pos_XS_list.at(i) << std::endl;
//	}

	// ここからLSIGD断面積配列を読み取る。
	// ########################### SIGブロック
	for(std::size_t i = 0; i < MT_list.size(); i++) {
		// MT番号別の断面積取得
		std::size_t refPos = nSIGD + Pos_XS_list.at(i);
		std::size_t NR = ace::getXssData<std::size_t>(xss_, refPos - 1);  // 1個目は補間する領域数
		if(NR != 0) {
			std::cerr << "Warning: other interpolation is recorded but only lin-lin interpolation is implimented." << std::endl;
		}
		// 意味はあるけど未使用パラメータ
//		std::vector<double> NBT_parameters = ace::getXssData<double>(xss_, NR, refPos);
//		std::vector<double> INT_parameters = ace::getXssData<double>(xss_, NR, refPos + NR);;

		std::size_t NE = ace::getXssData<std::size_t>(xss_, refPos + NR*2);  // データ数=エネルギー点数NE
		std::vector<double> epoints_mt = ace::getXssData<double>(xss_, NE, refPos + NR*2 + 1);  // エネルギー分点の取得
		std::vector<double> xs_vals = ace::getXssData<double>(xss_, NE, refPos + NR*2 + 1 + NE);     // 断面積値を取得

		std::vector<int> N_release_list(MTl_len, 0);  // ドシメトリライブラリは中性子発生はないので０をダミーとして入れておく
		std::vector<double> Qval_list(MTl_len, 0);
        const long epoint_lower_bound = 0;  // ドシメトリライブラリでは断面積は必ずエネルギーとペアになっているのでオフセットは０
        CrossSection tmpxs(epoints_mt, xs_vals, MT_list.at(i),  N_release_list.at(i),
						   Qval_list.at(i), epoint_lower_bound, 0  );
		XSmap_.emplace(tmpxs.mt, tmpxs);
	}
	//checkEndOfData();
}

void ace::NeutronDosimetryFile::read(std::ifstream &ifs)
{
	using namespace ace;
	// ########################### header 処理 ###########################################
	getAceHeader(ifs);

	// ########################### NXS 配列取得 ##########################################
	std::vector<long int>  nxs = getNXS(ifs);
	std::cout << "########## NXS ##########" << std::endl;
	//DumpNXS(std::cout);
	std::cout << std::endl;


	// ########################### JXS 配列取得 ##########################################
	std::vector<long int> jxs = getJXS(ifs);
	std::cout << "########## JXS ##########" << std::endl;
	//DumpJXS(std::cout);
	std::cout << std::endl;

	nxs_ = nxs;
	jxs_ = jxs;
	initNXS();
	initJXS();
	// ########################### MTR ブロック
	long int MTl_len = nxs.at(NSXDATA::NTR);
	// MT番号vectorの読み込み
	std::vector<int> MTn_list = getData<int>(MTl_len, ifs);

	// ※1 このMT番号の出現順番は後々の断面積の並び順となるので極めて重要。
	// MT番号化する。
	std::vector<ace::Reaction> MT_list(MTn_list.size());
	for(std::size_t i = 0; i < MTn_list.size(); i++) {
		//MT_list.at(i) = ace::mt::ItoMT(MTn_list.at(i));
		MT_list.at(i) = ace::Reaction(MTn_list.at(i));
		if(MT_list.at(i) == ace::Reaction::NOT_DEFINED) {
			std::cerr << "Warning: Not defined MT number detected = " << MTn_list.at(i) << std::endl;
		}
	}

	// ########################### LSIG ブロック
	std::vector<long int> Pos_XS_list    = getData<long int>( MTl_len, ifs );

	assert(MTn_list.size() == Pos_XS_list.size());
	// データに間違いがなければこの位置リストは実は使わなくてもいける。
//	std::cout << "MT, pos list" << std::endl;
//	for(size_t i = 0; i < Pos_XS_list.size(); i++) {
//		std::cout << "MT=" << static_cast<int>(MT_list.at(i)) << ", pos=" << Pos_XS_list.at(i) << std::endl;
//	}

	// ここからLSIGD断面積配列を読み取る。
	// ########################### SIGブロック
	for(std::size_t i = 0; i < MT_list.size(); i++) {
		// MT番号別の断面積取得
		std::size_t number_of_interpolation_regions = getData<std::size_t>(ifs);  // 1個目は補間する領域数

		if(number_of_interpolation_regions != 0) {
			std::cerr << "Warning: other interpolation is recorded but only lin-lin interpolation is implimented." << std::endl;
		}
		std::vector<int> NBT_parameters(number_of_interpolation_regions);
		std::vector<int> INT_parameters(number_of_interpolation_regions);
		for(size_t j = 0; j < number_of_interpolation_regions; j++) {
			NBT_parameters.push_back(getData<int>(ifs));
		}
		for(size_t j = 0; j < number_of_interpolation_regions; j++) {
			INT_parameters.push_back(getData<int>(ifs));
		}


		std::size_t num_xs_points = getData<std::size_t>(ifs);  // データ数=エネルギー点数NE


		std::vector<double> epoints_mt = getData<double>(num_xs_points, ifs);  // エネルギー分点の取得
		std::vector<double> xs_vals = getData<double>(num_xs_points, ifs);     // 断面積値を取得

		std::vector<int> N_release_list(MTl_len, 0);  // ドシメトリライブラリは中性子発生はないので０をダミーとして入れておく
		std::vector<double> Qval_list(MTl_len, 0);
        const long epoint_lower_bound = 0;  // ドシメトリライブラリでは断面積は必ずエネルギーとペアになっているのでオフセットは０
        CrossSection tmpxs(epoints_mt, xs_vals, MT_list.at(i),  N_release_list.at(i), Qval_list.at(i), epoint_lower_bound, 0  );
		XSmap_.emplace(tmpxs.mt, tmpxs);
	}


	checkEndOfData(ifs, true);
}



void ace::NeutronDosimetryFile::DumpNXS(std::ostream &os)
{
	assert(nxs_.size() == 16);
	os << "NXS( 1):Length of Second block data = " << nxs_.at(0) << std::endl;
	os << "NXS( 2):ZAID = " << nxs_.at(1) << std::endl;
	if(nxs_.at(2)  != 0) os << "NXS( 3):is not defined but used = " << nxs_.at(2) << std::endl;
	os << "NXS( 4):NTR:Number of Reactions = " << nxs_.at(3) << std::endl;
	if(nxs_.at(4)  != 0) os << "NXS( 5) is not defined but used = " << nxs_.at(4) << std::endl;
	if(nxs_.at(5)  != 0) os << "NXS( 6) is not defined but used = " << nxs_.at(5) << std::endl;
	if(nxs_.at(6)  != 0) os << "NXS( 7) is not defined but used = " << nxs_.at(6) << std::endl;
	if(nxs_.at(7)  != 0) os << "NXS( 8) is not defined but used = " << nxs_.at(7) << std::endl;
	if(nxs_.at(8)  != 0) os << "NXS( 9) is not defined but used = " << nxs_.at(8) << std::endl;
	if(nxs_.at(9)  != 0) os << "NXS(10) is not defined but used = " << nxs_.at(9) << std::endl;
	if(nxs_.at(10) != 0) os << "NXS(11) is not defined but used = " << nxs_.at(10) << std::endl;
	if(nxs_.at(11) != 0) os << "NXS(12) is not defined but used = " << nxs_.at(11) << std::endl;
	if(nxs_.at(12) != 0) os << "NXS(13) is not defined but used = " << nxs_.at(12) << std::endl;
	if(nxs_.at(13) != 0) os << "NXS(14) is not defined but used = " << nxs_.at(13) << std::endl;
	os << "NXS(15):NT: number of PIKMT reactions = " << nxs_.at(14) << std::endl;
	os << "NXS(16):0:normal photon producttion, -1: donot product photon = " << nxs_.at(15) << std::endl;
}

void ace::NeutronDosimetryFile::DumpJXS(std::ostream &os)
{
	assert( jxs_.size() == JXS_SIZE );

	os << "JSX( 1):LONE  = " << jxs_.at( 0) << std::endl;
	if(jxs_.at(1) != 0) os << "JSX( 2): not defined but used = " << jxs_.at( 1) << std::endl;
	os << "JSX( 3):MTR   = " << jxs_.at( 2) << std::endl;
	if(jxs_.at( 3) != 0) os << "JSX( 4): not defined but used = " << jxs_.at( 3) << std::endl;
	if(jxs_.at( 4) != 0) os << "JSX( 5): not defined but used = " << jxs_.at( 4) << std::endl;
	os << "JSX( 6):LSIG  = " << jxs_.at( 5) << std::endl;
	os << "JSX( 7):SIGD  = " << jxs_.at( 6) << std::endl;
	if(jxs_.at( 7) != 0) os << "JSX( 8): not defined but used = " << jxs_.at( 7) << std::endl;
	if(jxs_.at( 8) != 0) os << "JSX( 9): not defined but used = " << jxs_.at( 8) << std::endl;
	if(jxs_.at( 9) != 0) os << "JSX(10): not defined but used = " << jxs_.at( 9) << std::endl;
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
	os << "JSX(22):END   = " << jxs_.at(21) << std::endl;
	if(jxs_.at(22) != 0) os << "JSX(23): not defined but used = " << jxs_.at(22) << std::endl;
	if(jxs_.at(23) != 0) os << "JSX(24): not defined but used = " << jxs_.at(23) << std::endl;
	if(jxs_.at(24) != 0) os << "JSX(25): not defined but used = " << jxs_.at(24) << std::endl;
	if(jxs_.at(25) != 0) os << "JSX(26): not defined but used = " << jxs_.at(25) << std::endl;
	if(jxs_.at(26) != 0) os << "JSX(27): not defined but used = " << jxs_.at(26) << std::endl;
	if(jxs_.at(27) != 0) os << "JSX(28): not defined but used = " << jxs_.at(27) << std::endl;
	if(jxs_.at(28) != 0) os << "JSX(29): not defined but used = " << jxs_.at(28) << std::endl;
	if(jxs_.at(29) != 0) os << "JSX(30): not defined but used = " << jxs_.at(29) << std::endl;
	if(jxs_.at(30) != 0) os << "JSX(31): not defined but used = " << jxs_.at(30) << std::endl;
	if(jxs_.at(3) != 0) os << "JSX(32): not defined but used = " << jxs_.at(31) << std::endl;
}
