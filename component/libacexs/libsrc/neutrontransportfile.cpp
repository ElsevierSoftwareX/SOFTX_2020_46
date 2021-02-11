#include "neutrontransportfile.hpp"

#include <cassert>
#include <iomanip>
#include "aceutils.hpp"
#include "utils/string_util.hpp"
#include "mt.hpp"
#include "acefile.hpp"
#include "fissionneutrondata.hpp"

ace::NeutronTransportFile::NeutronTransportFile(std::ifstream &ifs, const std::string &id, std::size_t startline):
	AceFile(ifs, id, startline)
{
	initNXS();
	initJXS();
//	DumpNXS(std::cout);
//	DumpJXS(std::cout);
	readXss();
}


void ace::NeutronTransportFile::readXss()
{
	// ########################### ESZ ブロック
	// 最初の4データブロックは常に E分点、全、吸収、弾性、ヒーティング、と決まっている。
	std::vector<double> epoints = ace::getXssData<double>(xss_, NES, ESZ);
	// 弾性散乱は angular分布がある。放出中性子数は１となる。
	XSmap_.emplace(ace::Reaction::TOTAL,
				   CrossSection(epoints, ace::getXssData<double>(xss_, NES, ESZ + NES),
								ace::Reaction::TOTAL  , 0, 0, 0, 0));
	XSmap_.emplace(ace::Reaction::N_DISAPPEARANCE,
				   CrossSection(epoints, ace::getXssData<double>(xss_, NES, ESZ + NES*2),
								ace::Reaction::N_DISAPPEARANCE, 0, 0, 0, 0));
	XSmap_.emplace(ace::Reaction::ELASTIC,
				   CrossSection(epoints, ace::getXssData<double>(xss_, NES, ESZ + NES*3),
								ace::Reaction::ELASTIC,   1, 0, 0, 1));
	XSmap_.emplace(ace::Reaction::TOTAL_HEATING_NUMBER,
				   CrossSection(epoints, ace::getXssData<double>(xss_, NES, ESZ + NES*4),
								ace::Reaction::TOTAL_HEATING_NUMBER, 0, 0, 0, 0));


	// ########################### NU ブロック
	// JSX(2) → NU
	if(NU != 0 &&  NES*5 != NU -1) {
		std::cerr << "ERROR! Data table is discontinuous. NES*5 should be NU+1" << std::endl;
		std::cerr << "NES*5 = " << NES*5
				  << ", NU = " << NU << std::endl;
		std::exit(EXIT_FAILURE);
	}
	// XSSはファイルのヘッダを除いた部分を配列にしたもの。
	// データ順が正しければここでifsはXSS(NU)の1個前に到達している。
	// NUの意味は MCNP マニュアル App B table F.5参照

	if(NU != 0) {
		const int fissionFlag = ace::getXssData<int>(xss_, NU);  // XSS(NU)相当
		if(fissionFlag > 0) {
			const int KNU = NU;
			// promptとtotalが区別されていない場合。はtotal扱いにする。
			totalFissionNeutronData_ = std::unique_ptr<FissionNeutronData>(new FissionNeutronData(xss_, KNU));
		} else if (fissionFlag < 0) {
			const int KNU = NU + 1 ;
			/*
			 * 即発遅発両者が与えられている場合。
			 *  ・即発中性子は XSS(KNU=NU+1) から開始、
			 *  ・全中性子は   XSS(KNU=NU+ABS(XSS(NU))+1)から開始
			 */
			// 即発中性子の読み込み。 XSS(KNU=NU+1)から読むが、ここまででXSS(NU)まで読み込んでいるので、
			// XSS(NU+1)を取得するには巻き戻さず普通に読めば良い。
			promptFissionNeutronData_ = std::unique_ptr<FissionNeutronData>(new FissionNeutronData(xss_, KNU));
			totalFissionNeutronData_  = std::unique_ptr<FissionNeutronData>(new FissionNeutronData(xss_, KNU + std::abs(fissionFlag)));
			/*
			 * 即発を読み込んだあとに次にまだデータブロックがあるのは
			 *	1.DNUで指定されているから。
			 *	2.XSS(NU+1) < 0だから
			 * のどちらだろうか？あるいは両者とも正しい冗長なデータか?
			 *
			 * U_235.j40n_300Kの場合
			 * ・DNU=670142
			 * ・NU=161731, XSS(NU+1)=-39
			 * なのでここでは2.が正しい。ここにあるのはdelayedではなくtotal.
			 */
		} else if(DNU > 0) {
			// NU !=0 かつDNU:JXS(24) >0 なら遅発νのみ
			const int KNU = DNU;
			delayedFissionNeutronData_ = std::unique_ptr<FissionNeutronData>(new FissionNeutronData(xss_, KNU));
		}
	}
	// 遅発中性子データ
	if(DNU > 0) {
		delayedFissionNeutronData_ = std::unique_ptr<FissionNeutronData>(new FissionNeutronData(xss_, DNU));  // 遅発中性子数分布
		// 前駆分布
		for(int i = 0; i < NPCR; i++) {
			int offset;
			if (precursorData_.begin() == precursorData_.end()) {
				offset = 0;
			} else {
				auto itr = precursorData_.end() - 1;
                offset = static_cast<int>(3 + 2*itr->nbt().size() + 2*itr->epoints().size());
			}
			precursorData_.emplace_back(PrecursorData(xss_, BDD + offset));
		}

		std::vector<int> locationOfEnergyDistributions = ace::getXssData<int>(xss_, NPCR, DNEDL);
		//dDebug() << locationOfEnergyDistributions;
		std::vector<double> energyEistributions;
		for(int i = 0; i < NPCR; i++) {
			// TODO_xs 遅発中性子データの読み取り
			//energyEistributions.emplace_back(getXssData<double>(nDNED + locationOfEnergyDistributions.at(i), ))
		}

	}



	// ########################### MTR ブロック
	// MTテーブルの長さチェック。
	long int MTl_len = NTR;
	assert(MTl_len == LQR - MTR );
	assert(MTl_len == TYR - LQR );
	assert(MTl_len == LSIG - TYR );
	assert(MTl_len == SIG  - LSIG );

//	if(XSmap_.find(ace::Reaction::NOTSET) != XSmap_.end()) {
//		std::cerr << __FILE__ << ":" << __LINE__ << " XSmap has undefined element.!!!";
//		abort();
//	}

	// MT番号リストの読み込み
	std::vector<int> MTn_list = ace::getXssData<int>(xss_, MTl_len, MTR);

	// 整数をReaction列挙子に変換
	// ※1 このMT番号の出現順番は後々の断面積の並び順となるので極めて重要。
	// ※2 MT_listに弾性散乱(MT2)は含まれない。
	std::vector<ace::Reaction> MT_list( MTn_list.size());
	for(std::size_t i = 0; i < MTn_list.size(); i++) {
		MT_list.at(i) = ace::mt::toReaction(MTn_list.at(i));
		if(MT_list.at(i) == ace::Reaction::NOT_DEFINED) {
			std::cerr << "Warning: Not defined MT number detected = " << MTn_list.at(i) << std::endl;
		}
	}

	// ########################### LQRブロック
	// Q値、中性子放出数、断面積データ位置、リストの読み込み
	std::vector<double> Qval_list =  ace::getXssData<double>(xss_, MTl_len, LQR);

	// ########################### TYR ブロック
	std::vector<int> N_release_list = ace::getXssData<int>(xss_, MTl_len, TYR);
	// ########################### LSIG ブロック
	std::vector<long int> Pos_XS_list    = ace::getXssData<long int>(xss_, MTl_len, LSIG);

//	std::cout << "# MT Q n pos" << std::endl;
//	for(int i = 0; i < MTl_len; i++) {
//		std::cout << std::right << std::setw(6) << MTn_list.at(i) << " "  << std::setw(12) << std::left << Qval_list.at(i)
//				  << std::right << std::setw(5) << N_release_list.at(i) << std::setw(8) << Pos_XS_list.at(i) << std::endl;
//	}


	// ########################### SIGブロック
	for(std::size_t i = 0; i < MT_list.size(); i++) {
		// MT番号別の断面積取得
		// 1個目は下限エネルギー分点番号
		std::size_t epoint_lower_bound = ace::getXssData<std::size_t>(xss_, SIG + Pos_XS_list.at(i) - 1); // IE
		std::size_t num_xs_points = ace::getXssData<std::size_t>(xss_, SIG + Pos_XS_list.at(i));  // NE
		assert(epoint_lower_bound + num_xs_points - 1 == epoints.size());

		// エネルギー分点の取得
		std::vector<double> epoints_mt(num_xs_points);
		for(std::size_t j = 0; j < num_xs_points; j++){
			epoints_mt.at(j) = epoints.at(j+epoint_lower_bound-1);
		}

		// 断面積値を取得
		std::vector<double> xs_vals = ace::getXssData<double>(xss_, num_xs_points, SIG + Pos_XS_list.at(i) + 1);

        CrossSection tmpxs(epoints_mt, xs_vals, MT_list.at(i),  N_release_list.at(i),
                           Qval_list.at(i), epoint_lower_bound, 0  );
		XSmap_.emplace(tmpxs.mt, tmpxs);
	}

	// ###################  LAND ブロック処理 ################################################
	// 角度分布があるか無いかは ACEのLANDブロックで決まる。
	//
	//  0：等方分布。 LABかCMどちらで等方かはTYRブロックで決められる
	// -1：ANDブロックにデータなし。散乱方向はDLWブロックで決められる。
	//  n：角度分布データの開始位置。ANDブロックの先頭を基準とする。
	//
	// LANDブロックの最初は特例的にelasticが入ってくる。
	// このブロックの配置順はMTテーブルの配置順であり、MT順ではない
	// したがってキーでソートされてしまうstd::mapにはテーブル配置順情報は失われているから注意

	// 最初に MT2 (elastic) が必ずくる。
	XSmap_.at(ace::Reaction::ELASTIC).angular_flag =  ace::getXssData<int>(xss_, LAND);
	//std::cout << "elastic angular flag=" << 	XSmap_.at(ace::Reaction::ELASTIC).angular_flag << std::endl;
	for(std::size_t i = 0; i < MT_list.size(); i++){
		for(MTmap_iterator it = XSmap_.begin(); it != XSmap_.end(); it++){
			// XSmapのうち MTlistに記載されており、かつ 中性子放出数が0でないもののみ角度分布フラッグ(と分布)が存在する。
			if((it->first == MT_list.at(i)) && (it->second.release_n != 0)){
				it->second.angular_flag = ace::getXssData<int>(xss_, LAND + i + 1);
				//std::cout << "MT" << ace::mt::toNumber(it->first) << " angular flag = " << it->second.angular_flag << std::endl;
			}
		}
	}

	// ###################### AND ブロック ######################################################################
	// 最初は必ず弾性散乱elasticの角度分布読み取り
	assert(XSmap_.at(ace::Reaction::ELASTIC).angular_flag == 1);
	XSmap_.at(ace::Reaction::ELASTIC).angular_dists = ReadAngularTable(xss_, AND, XSmap_.at(ace::Reaction::ELASTIC).angular_flag);

	// TODO_xs angular_flag=0の等方と<0のDLWブロック依存の場合のデータ読み取り
	// その他の反応
	for(std::size_t i = 0; i < MT_list.size(); i++){
		assert(MT_list.at(i) != ace::Reaction::ELASTIC);

        ace::Reaction currentMT = MT_list.at(i);
		//dDebug() << "reading mt=" << ace::mt::mt::string(currentMT) << "location=" << XSmap_[currentMT].angular_flag;
		// angular_flag = 0:角度分布なし、 -1：等方分布 なのでそれ以外の場合はtableを読みに行く
		if(XSmap_.at(currentMT).angular_flag != -1 && XSmap_.at(currentMT).angular_flag != 0) {
			XSmap_.at(currentMT).angular_dists = ReadAngularTable(xss_, AND, XSmap_.at(currentMT).angular_flag);
		}
	}


	// ##################### LDLW Block Block ########################################################################
	// このlocationはJXS(11)=DLWからの相対値
	std::vector<int> locationOfSecondaryNEnergies = ace::getXssData<int>(xss_, NR, LDLW);
	//dDebug() << "locationofSecondNE=\n" << locationOfSecondaryNEnergies;


	// ###################### DLW Block ################################################################################
	// ここから



}



void ace::NeutronTransportFile::DumpNXS(std::ostream &os)
{
	assert(nxs_.size() == 16);
// ややこしい話だがACEフォーマットの解説で使われている配列は1から始まるのでずれる。
	os << "NXS( 1):Length of Second block data = " << nxs_.at(0) << std::endl;
	os << "NXS( 2):ZAID = " << nxs_.at(1) << std::endl;
	os << "NXS( 3):NES:Number of Energies = " << nxs_.at(2) << std::endl;
	os << "NXS( 4):NTR:Number of Reactions excluding elastic = " << nxs_.at(3) << std::endl;
	os << "NXS( 5):NR:Number of reactions having 2nd neutrons excluding erastic = " << nxs_.at(4) << std::endl;
	os << "NXS( 6):NTRP:Number of photon production reactions = " << nxs_.at(5) << std::endl;
	os << "NXS( 7) is not defined = " << nxs_.at(6) << std::endl;
	os << "NXS( 8):NPCR: Number of delayed neutron precursor families = " << nxs_.at(7) << std::endl;
	if(nxs_.at(8)  != 0) os << "NXS( 9) is not defined but used = " << nxs_.at(8) << std::endl;
	if(nxs_.at(9)  != 0) os << "NXS(10) is not defined but used = " << nxs_.at(9) << std::endl;
	if(nxs_.at(10) != 0) os << "NXS(11) is not defined but used = " << nxs_.at(10) << std::endl;
	if(nxs_.at(11) != 0) os << "NXS(12) is not defined but used = " << nxs_.at(11) << std::endl;
	if(nxs_.at(12) != 0) os << "NXS(13) is not defined but used = " << nxs_.at(12) << std::endl;
	if(nxs_.at(13) != 0) os << "NXS(14) is not defined but used = " << nxs_.at(13) << std::endl;
	os << "NXS(15):NT: number of PIKMT reactions = " << nxs_.at(14) << std::endl;
	os << "NXS(16):0:normal photon producttion, -1: donot product photon = " << nxs_.at(15) << std::endl;
}

void ace::NeutronTransportFile::DumpJXS(std::ostream &os)
{
	//std::cout << "JXS_SIZE=" << JXS_SIZE << ", .size()=" << jxs_.size();
	assert(jxs_.size() == JXS_SIZE);

	os << "JSX( 1):ESZ   = " << jxs_.at( 0) << std::endl;
	os << "JSX( 2):NU    = " << jxs_.at( 1) << std::endl;
	os << "JSX( 3):MTR   = " << jxs_.at( 2) << std::endl;
	os << "JSX( 4):LQR   = " << jxs_.at( 3) << std::endl;
	os << "JSX( 5):TYR   = " << jxs_.at( 4) << std::endl;
	os << "JSX( 6):LSIG  = " << jxs_.at( 5) << std::endl;
	os << "JSX( 7):SIG   = " << jxs_.at( 6) << std::endl;
	os << "JSX( 8):LAND  = " << jxs_.at( 7) << std::endl;
	os << "JSX( 9):AND   = " << jxs_.at( 8) << std::endl;
	os << "JSX(10):LDLW  = " << jxs_.at( 9) << std::endl;
	os << "JSX(11):DLW   = " << jxs_.at(10) << std::endl;
	os << "JSX(12):GPD   = " << jxs_.at(11) << std::endl;
	os << "JSX(13):MTRP  = " << jxs_.at(12) << std::endl;
	os << "JSX(14):LSIGP = " << jxs_.at(13) << std::endl;
	os << "JSX(15):SIGP  = " << jxs_.at(14) << std::endl;
	os << "JSX(16):LANDP = " << jxs_.at(15) << std::endl;
	os << "JSX(17):ANDP  = " << jxs_.at(16) << std::endl;
	os << "JSX(18):LDLWP = " << jxs_.at(17) << std::endl;
	os << "JSX(19):DLWP  = " << jxs_.at(18) << std::endl;
	os << "JSX(20):YP    = " << jxs_.at(19) << std::endl;
	os << "JSX(21):FIS   = " << jxs_.at(20) << std::endl;
	os << "JSX(22):END   = " << jxs_.at(21) << std::endl;
	os << "JSX(23):LUNR  = " << jxs_.at(22) << std::endl;
	os << "JSX(24):DNU   = " << jxs_.at(23) << std::endl;
	os << "JSX(25):BDD   = " << jxs_.at(24) << std::endl;
	os << "JSX(26):DNEDL = " << jxs_.at(25) << std::endl;
	os << "JSX(27):DNED  = " << jxs_.at(26) << std::endl;
	os << "JSX(28):      = " << jxs_.at(27) << std::endl;
	os << "JSX(29):      = " << jxs_.at(28) << std::endl;
	os << "JSX(30):      = " << jxs_.at(29) << std::endl;
	os << "JSX(31):      = " << jxs_.at(30) << std::endl;
	os << "JSX(32):      = " << jxs_.at(31) << std::endl;
//	if(jxs_.at(27) != 0) os << "JSX(28): not defined but used = " << jxs_.at(27) << std::endl;
//	if(jxs_.at(28) != 0) os << "JSX(29): not defined but used = " << jxs_.at(28) << std::endl;
//	if(jxs_.at(29) != 0) os << "JSX(30): not defined but used = " << jxs_.at(29) << std::endl;
//	if(jxs_.at(30) != 0) os << "JSX(31): not defined but used = " << jxs_.at(30) << std::endl;
//	if(jxs_.at(31) != 0) os << "JSX(32): not defined but used = " << jxs_.at(31) << std::endl;
}


#define SETNXS(ARG) ARG = nxs_.at(static_cast<int>(NXSPOS::ARG))
void ace::NeutronTransportFile::initNXS() {
	SETNXS(LENGTH2ndBLOCK);
	SETNXS(ZA);
	SETNXS(NES);
	SETNXS(NTR);
	SETNXS(NR);
	SETNXS(NTRP);
	SETNXS(NPCR);
	SETNXS(NT);
	SETNXS(PHOTOPROD);
}
#undef SETNXS

#define SETJXS(ARG) ARG = jxs_.at(static_cast<int>(JXSPOS::ARG))
void ace::NeutronTransportFile::initJXS() {
	SETJXS(ESZ);
	SETJXS(NU);
	SETJXS(MTR);
	SETJXS(LQR);
	SETJXS(TYR);
	SETJXS(LSIG);
	SETJXS(SIG);
	SETJXS(LAND);
	SETJXS(AND);
	SETJXS(LDLW);
	SETJXS(DLW);
	SETJXS(GPD);
	SETJXS(MTRP);
	SETJXS(LSIGP);
	SETJXS(SIGP);
	SETJXS(LANDP);
	SETJXS(ANDP);
	SETJXS(LDLWP);
	SETJXS(DLWP);
	SETJXS(YP);
	SETJXS(FIS);
	SETJXS(END);
	SETJXS(LUNR);
	SETJXS(DNU);
	SETJXS(BDD);
	SETJXS(DNEDL);
	SETJXS(DNED);
}
#undef SETJXS

