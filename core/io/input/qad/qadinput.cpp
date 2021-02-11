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
#include "qadinput.hpp"

#include <iomanip>
#include <regex>
#include <unordered_map>

#include "core/utils/message.hpp"
#include "core/utils/stream_utils.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/system_utils.hpp"
#include "core/io/input/common/commoncards.hpp"

namespace {


/*
 * 自由形式入力で行跨ぎがあるとlineNumberがずれる。
 * これを防ぐには
 * 1．次の空白(asciiコード32)あるいはCR(同13)以外の位置までssを進める
 * 2. 次の文字(ss.peak())が改行(LF, 同10)なら++lineNumberする
 * 3. ss >> で入力を読み取る
 * の繰り返し。
 */
// T型の変数をフリーフォーマットでssから読み取る。 途中改行があれば lineNumberに加算する。
template <class T>
T readFreeFormatValue(std::stringstream &ss, int* lineNumber)
{
	// ここでCR(ascii=13)も飛ばしているからCR対策は出来ている。
	while(ss.peek() == ' ' || ss.peek() == '\r') {ss.get();}
	if(ss.peek() == '\n') {
		if(lineNumber != nullptr) ++(*lineNumber);
		//mDebug() << "Linefeed detected!!!!!!!!! during freeformat reading current===" << *lineNumber;
	}
	std::string buff;
	ss >> buff;
	return utils::stringTo<T>(buff);
}

// cardが終わったらgetlineして余剰データチェックをする。
// この時カード行末の改行文字をssから削除してしまうのでlineNumberは+1する。
// 余剰データがあったら例外発生。
void checkCardEnd(std::stringstream &ss, int* lineNumber)
{
	std::string buff;
	getline(ss, buff);
	utils::sanitizeCR(&buff);
	if(buff.find_first_not_of(" ") != std::string::npos) {
		throw std::runtime_error("extra data = \"" + utils::trimmed(buff) + "\"");
	}
	if(lineNumber != nullptr) ++(*lineNumber);
}
}  // end anonymous namespace





void inp::qad::QadInput::init(const std::string &inputFileName)
{
	/*
	 * この関数では基底の4リストlist<DataLine> cellCards_, surfaceCards_, materialCards_, transformCards_;
	 * を初期化する。
	 * QADではtransformは無いのでcell, surface, materialを作成する。
	 */

	/*
	 * QADのメタカードはmcnp/phitsと割と異なるのでInputData::initは使えない。 自力で処理すべし。
	 * といっても継続行があるのは一部のカードだけなので、結局処理すべきは
	 * 勝手に拡張したコメントの削除くらい。
	 */
	inputFile_ = inputFileName;
	path_ = utils::separatePath(inputFile_).first;
	// こここでインクルード処理されてしまうが別にいいか。CRの除去とかもしてくれるし。
	dataLines_ = InputData::readFile("", inputFile_);
	// ここで正味データ行番号と実ファイル行番号の対応表を作成する。
	int netLn=0, realLn=0;  // 正味行番号と実業番号
	for(const auto& dl: dataLines_) {
		++realLn;
		if(!std::regex_search(dl.data, comm::getPreCommentPattern())) {
			++netLn;
			assert(realLn >= netLn);
			lineTable_.emplace(netLn, realLn);
		}
	}
    InputData::removeComment(config_.mode, &dataLines_);  // 行頭コメントアウトはphits/mcnpで同じなのでここ実施
	// 後置コメントの削除は基底クラスで実施しなくなった(MCNPで対応できなくなる)ためここで。
	for(auto it = dataLines_.begin(); it != dataLines_.end(); ++it) {
		comm::removeMcnpPostComment(&(it->data));    // mcnp式後置コメントの除去
		comm::removePhitsPostCommentNotSharp(&(it->data));  // phits式後置コメントの除去
	}

	//mDebug() << "input===\n" << dataLines_;

	/*
	 * 自由形式と固定形式の混在はどのように読み取るべきか？
	 * ・splitStringでvector化 → 行跨ぎしていた場合にに対応不可
	 * ・it->dataをstringstreamに食わせる → やっぱり行またぎしていた場合に対応不可
	 *
	 * よってdataLines_全体をstringstreamに食わせて
	 * 入力ファイル形式を頭から逐次処理するしか無い
	 */
	int lineNumber = 1;
	std::stringstream ss;
	for(const auto& dl: dataLines_) {
		ss << dl.data << std::endl;
	}
	// 最初のカードはタイトル
	getline(ss, title_);
	utils::sanitizeCR(&title_);
	++lineNumber;



	// 次のカードはコントロールカード (FreeFormat)
	try {
		lso_ = readFreeFormatValue<decltype(lso_)>(ss, &lineNumber);
		mso_ = readFreeFormatValue<decltype(mso_)>(ss, &lineNumber);
		nso_= readFreeFormatValue<decltype(nso_)>(ss, &lineNumber);
		mat_ = readFreeFormatValue<decltype(mat_)>(ss, &lineNumber);
		ncomp_ = readFreeFormatValue<decltype(ncomp_)>(ss, &lineNumber);
		nreg_ = readFreeFormatValue<decltype(nreg_)>(ss, &lineNumber);
		nrgy_ = readFreeFormatValue<decltype(nrgy_)>(ss, &lineNumber);
		nbound_ = readFreeFormatValue<decltype(nbound_)>(ss, &lineNumber);
		nsopt_ = readFreeFormatValue<decltype(nsopt_)>(ss, &lineNumber);
		nzso_ = readFreeFormatValue<decltype(nzso_)>(ss, &lineNumber);
		isrc_ = readFreeFormatValue<decltype(isrc_)>(ss, &lineNumber);
		ineut_ = readFreeFormatValue<decltype(ineut_)>(ss, &lineNumber);
		ngpf_ = readFreeFormatValue<decltype(ngpf_)>(ss, &lineNumber);
		ngpl_ = readFreeFormatValue<decltype(ngpl_)>(ss, &lineNumber);
		ngpi_ = readFreeFormatValue<decltype(ngpi_)>(ss, &lineNumber);
		ngint_ = readFreeFormatValue<decltype(ngint_)>(ss, &lineNumber);
		checkCardEnd(ss, &lineNumber);
	} catch (std::exception &e) {
        throw std::invalid_argument(inputFile_ + ":" + std::to_string(toRealLineNumber(lineNumber))
                                    + " Invalid control card. reason =" + e.what());
	}
	// fatalしたらruntime_error 投げるので以下はは通らない。

	int srcCardNum = 1;  // これから読み取るソースカードの番号

	try{
		// 線源情報カード1 (FreeFormat)
		aso_ = readFreeFormatValue<decltype(aso_)>(ss, &lineNumber);
		xiso_.at(0).at(0) = readFreeFormatValue<double>(ss, &lineNumber);
		xiso_.at(1).at(0) = readFreeFormatValue<double>(ss, &lineNumber);
		xiso_.at(0).at(1) = readFreeFormatValue<double>(ss, &lineNumber);
		xiso_.at(1).at(1) = readFreeFormatValue<double>(ss, &lineNumber);
		xiso_.at(0).at(2) = readFreeFormatValue<double>(ss, &lineNumber);
		xiso_.at(1).at(2) = readFreeFormatValue<double>(ss, &lineNumber);
		checkCardEnd(ss, &lineNumber);
		++srcCardNum;

		// 線源情報カード2 (FreeFormat)
		for(int i = 0; i < lso_+1; ++i) {
			rso_.emplace_back(readFreeFormatValue<decltype(rso_)::value_type>(ss, &lineNumber));
		}
		checkCardEnd(ss, &lineNumber);
		++srcCardNum;

		// 線源情報カード3 (FreeFormat)
		for(int i = 0; i < mso_+1; ++i) {
			zso_.emplace_back(readFreeFormatValue<decltype(zso_)::value_type>(ss, &lineNumber));
		}
		checkCardEnd(ss, &lineNumber);
		++srcCardNum;

		// 線源情報カード4 (FreeFormat)
		for(int i = 0; i < nso_+1; ++i) {
			phiso_.emplace_back(readFreeFormatValue<decltype(phiso_)::value_type>(ss, &lineNumber));
		}
		checkCardEnd(ss, &lineNumber);
		++srcCardNum;

		if(isrc_ == 2) {
			// 線源情報カード 5, 6, 7  (FreeFormat)
			for(size_t i = 0; i < rso_.size(); ++i) {
				fl_.emplace_back(readFreeFormatValue<decltype(fl_)::value_type>(ss, &lineNumber));
			}
			checkCardEnd(ss, &lineNumber);
			++srcCardNum;

			for(size_t i = 0; i < zso_.size(); ++i) {
				fm_.emplace_back(readFreeFormatValue<decltype(fm_)::value_type>(ss, &lineNumber));
			}
			checkCardEnd(ss, &lineNumber);
			++srcCardNum;

			for(size_t i = 0; i < phiso_.size(); ++i) {
				fn_.emplace_back(readFreeFormatValue<decltype(fn_)::value_type>(ss, &lineNumber));
			}
			checkCardEnd(ss, &lineNumber);
			++srcCardNum;
		}
	} catch (std::exception &e) {
        throw std::invalid_argument(inputFile_ + ":" + std::to_string(toRealLineNumber(lineNumber))
                                    + "Invalid data found in source information card"
                                    + std::to_string(srcCardNum) + ". reason =" + e.what());
	}



	// ジオメトリカード1 CGA card
	std::string buff;
	getline(ss, buff);
	utils::sanitizeCR(&buff);
	++lineNumber;
	if(buff.size() >= 10) {
		// FORTRANでは空白文字列から整数を読み込むと0になる。
		// stringToで空白文字列を変換しようとすると例外になるのでこれをcatchして対処
		try {
			ivopt_ = utils::stringTo<int>(buff.substr(0, 5));
		} catch (...) {
			ivopt_ = 0;
		}
		buff = buff.substr(5);
		try {
			idbg_ = utils::stringTo<int>(buff.substr(0, 5));
		} catch (...) {
			idbg_ = 0;
		}
		buff = buff.substr(5);
	}
	// formatは2I5,10x, 15A4なので10x部分を(あれば)カット
	if(buff.size() > 10) buff = buff.substr(10);
	jty_ = buff;

	/*
	 * ジオメトリカード2 CGB (2X, A3, 1X, I4, 6E10.3) + 継続行
	 * このカード群は可変長でENDが出るまで続く
	 * また、パラメータが6個以上必要なCGbodyもあるので継続行の場合もある。
	 * IALPはブランクの場合がある。この時は番号自動割当なので、適当に番号を割り振る。
	 */
	try {
		cgbs_ = CGBody::getCgBodiesFromQadFixed(ss, lineNumber);
	} catch(std::exception &e) {
		std::stringstream sse;
		sse << "Exception, what = " << e.what();
        throw std::invalid_argument(inputFile_ + ":" + std::to_string(toRealLineNumber(lineNumber)) + " " + sse.str());
	}

	/*
	 * ジオメトリカード3 (2X,A3,I5,9(A2,I5)) ゾーン定義。 要はセル定義
	 * QADでは"with input zone numbers being assigned sequentially"というように
	 * ゾーン番号には通し番号が割り振られる。
	 */
	try {
		cgcs_ = CGZone::getCgZonesFromQadFixed(ss, lineNumber);
	} catch (std::exception &e) {
		std::stringstream sse;
		sse << "Exception, what = " << e.what();
        throw std::invalid_argument(inputFile_ + ":" +  std::to_string(toRealLineNumber(lineNumber)) + " " + sse.str());
	}



	std::string currentLine;
	// ジオメトリカード4 （今は使っていないので飛ばす）
	// 書式は14I5だが、zone数が14以上の場合二行にわたる。
	size_t numCGDLines = cgcs_.size()/14 + 1;
	for(size_t i = 0; i < numCGDLines; ++i) {
		getline(ss, currentLine);
		//mDebug() << "CGD===" << currentLine;
		utils::sanitizeCR(&currentLine);
		++lineNumber;
	}

	// ジオメトリカード5 (14I5)
	mmiz_.reserve(cgcs_.size());
	getline(ss, currentLine);
	utils::sanitizeCR(&currentLine);
	++lineNumber;
	//mDebug() << "CGE dataline ===" << currentLine;
	while(!currentLine.empty()) {
		size_t matLen = (std::min)(size_t(5), currentLine.size());
		std::string matStr = currentLine.substr(0, matLen);
		//mDebug() << "matStr ===" << matStr;
		try {
			mmiz_.emplace_back(utils::stringTo<int>(matStr));
		} catch (std::exception &e) {
            throw std::invalid_argument(inputFile_ + ":" + std::to_string(toRealLineNumber(lineNumber))
                                        + "CGE error." + e.what());
		}
		currentLine = currentLine.substr(matLen);

		// zone数とmmizの要素数が一致したらそこで読み取りは打ち切る。
		// 非空白の余剰データがあれば警告する。
		if(cgcs_.size() == mmiz_.size()) {
            if(currentLine.find_first_not_of(" ") != std::string::npos) {
                if(!config_.quiet) mWarning(inputFile_ + ":" + std::to_string(toRealLineNumber(lineNumber)))
						<< "Extra data found in CGE card, extra data ="
						<< "\"" + currentLine + "\"";
			}
			break;
		}
	}

	int matCardNum = 1;
	// 材料カード1 (FreeFormat)
	try {
		buff = readFreeFormatValue<std::string>(ss, &lineNumber);
		nbld_ = utils::stringTo<int>(buff);
		for(int i = 0; i < mat_; ++i) {
			matz_.emplace_back(utils::stringTo<int>(readFreeFormatValue<std::string>(ss, &lineNumber)));
		}
		checkCardEnd(ss, &lineNumber);

		// 材料カード2 (nbld_ > 10)の場合のみ。 FreeFormat
		++matCardNum;
		if(nbld_ > 0) {
			matgp_ = readFreeFormatValue<decltype(matgp_)>(ss, &lineNumber);
			icgp_ = readFreeFormatValue<decltype(icgp_)>(ss, &lineNumber);
			try {
				congy_ = utils::stringTo<double>(readFreeFormatValue<std::string>(ss, &lineNumber));
			} catch (std::exception &e) {
                (void) e;
				throw std::runtime_error("CONGY should be a number. data = " + buff);
			}
			matdos_ = readFreeFormatValue<decltype(matdos_)>(ss, &lineNumber);
			checkCardEnd(ss, &lineNumber);
		}

		// 材料カード3 FreeFormat
		++matCardNum;
		for(int i = 0; i < ncomp_; ++i) {
			std::vector<std::string> compStrVec;
			for(size_t j = 0; j < matz_.size(); ++j) {
				compStrVec.emplace_back(readFreeFormatValue<std::string>(ss, &lineNumber));
			}
			checkCardEnd(ss, &lineNumber);
			comps_.emplace_back(utils::stringVectorTo<double>(compStrVec));
		}
	} catch(std::exception &e) {
        throw std::invalid_argument(inputFile_ + ":" + std::to_string(toRealLineNumber(lineNumber))
                                    + "Invalid material card" + std::to_string(matCardNum) +" reason =" + e.what());
	}


	dumpQadVars(std::cout);





	/*
	 * この関数では基底の4リストlist<DataLine> cellCards_, surfaceCards_, materialCards_, transformCards_;
	 * を初期化する。
	 * QADではtransformは無いのでcell, surface, materialを作成する。
	 */

	/*
	 * MMIZ: ゾーンの材料番号を格納。ゾーン番号は1から連続で与えられるのでvector格納している。MMIZの要素は材料番号なので連続しない
	 * m = MMIZ[i]とすると、
	 * COMPS[m-1]が材料番号mの組成に対応し、COMPS[m-1][j]が材料mのj番目核種密度を表す。
	 * MATZ：全材料共通の原子番号カタログ MATZ[j]がj番目の原子番号となる
	 *
	 * ゆえに
	 * ・ matz_.size()とcomps[].size()は等しくなければならない。
	 * ・comps_.size() >= m-1  (mが0，1000の場合を除く)
	 *
	 */
	for(size_t i = 0; i < comps_.size(); ++i) {
		assert(matz_.size() == comps_.at(i).size());
	}
	for(size_t i = 0; i < mmiz_.size(); ++i) {
		auto matNum = mmiz_.at(i);
		if(matNum != 0 && matNum != 1000) {
			assert(static_cast<int>(comps_.size()) > matNum -1);
		}
	}

	// 材料番号をキーに、密度を値にとるマップ
	std::unordered_map<size_t, double> totDensMap;
	// #### Materialカード作成
	for(size_t i = 0; i < mmiz_.size(); ++i) {
		assert(mmiz_.at(i) >= 0);
		size_t matNum = static_cast<size_t>(mmiz_.at(i));

		if(matNum == 0 || matNum == 1000) continue;
		if(matNum-1 >= comps_.size()) {
            throw std::invalid_argument(inputFile_ + ":" + std::to_string(toRealLineNumber(lineNumber))
                                        + "No entry for material " + std::to_string(matNum) + " found in CardN(COMP)."
                                        + "is NCOMP in cotrol card wrong?");
		}
		assert(comps_.at(matNum-1).size() == matz_.size());

		double totalDensity = 0;
		std::string matEntry = "M" + std::to_string(matNum) +  " ";
		const std::vector<double> &compositions = comps_.at(matNum-1);
		for(size_t j = 0; j < compositions.size(); ++j) {
			matEntry += " " + std::to_string(matz_.at(j)*1000)
					  + " " + std::to_string(-1*compositions.at(j));
			totalDensity += compositions.at(j);
		}
		materialCards_.emplace_back(DataLine(inputFile_, 0, matEntry));
		totDensMap.emplace(matNum, totalDensity);
	}


	// #### Cellカードはちょっとむずかしい。
	for(size_t i = 0; i < cgcs_.size(); ++i) {
		std::string cellCardStr = std::to_string(i+1) + " ";
		size_t matNum =  (mmiz_.at(i) == 1000) ? 0 : mmiz_.at(i);

		cellCardStr += std::to_string(matNum) + " ";
		cellCardStr += (matNum == 0) ? "" : std::to_string(-1*totDensMap.at(matNum)) + " ";
        cellCardStr += cgcs_.at(i).equation();
        if(config_.verbose) mDebug() << "cellCard ===" << cellCardStr;
		cellCards_.emplace_back(DataLine(inputFile_, 0, cellCardStr));
	}

	// #### surfaceカード作成
	for(size_t i = 0; i < cgbs_.size(); ++i) {
		std::string surfInp = cgbs_.at(i).toInputString();
		utils::tolower(&surfInp);
		surfaceCards_.emplace_back(DataLine(inputFile_, 0, surfInp));
	}








	mDebug() << "\n######## MCNP compat cards #####";
	mDebug() << cellCards_;
	mDebug();
	mDebug() << surfaceCards_;
	mDebug();
	mDebug() << materialCards_;




	//・assert(!"Debug end");
}

// 線源で指定されている粒子種類を返す。
std::vector<phys::ParticleType> inp::qad::QadInput::particleTypes() const
{
	// QADなのでとりあえずγ線を返す
	return std::vector<phys::ParticleType>{phys::ParticleType::PHOTON};
}

int inp::qad::QadInput::toRealLineNumber(int lineNumber) const
{
	return lineTable_.at(lineNumber);
}

#define OS(ARG) os << std::setw((ARG))
void inp::qad::QadInput::dumpQadVars(std::ostream &os) const
{
	os << std::left;
	os << "TITLE = " << title_ << std::endl;
	os << "#### Control data" << std::endl;
	OS(10) << "LSO =" << lso_ << std::endl;
	OS(10) << "MSO =" << mso_ << std::endl;
	OS(10) << "NSO =" << nso_ << std::endl;
	OS(10) << "MAT =" << mat_ << std::endl;
	OS(10) << "NCOMP =" << ncomp_ << std::endl;
	OS(10) << "NREG =" << nreg_ << std::endl;
	OS(10) << "NRGY =" << nrgy_ << std::endl;
	OS(10) << "NBOUND =" << nbound_ << std::endl;
	OS(10) << "NSOPT =" << nsopt_ << std::endl;
	OS(10) << "NZSO =" << nzso_ << std::endl;
	OS(10) << "ISRC =" << isrc_ << std::endl;
	OS(10) << "INEUT =" << ineut_ << std::endl;
	OS(10) << "NGPF =" << ngpf_ << std::endl;
	OS(10) << "NGPL =" << ngpl_ << std::endl;
	OS(10) << "NGPI =" << ngpi_ << std::endl;
	OS(10) << "NGINT =" << ngint_ << std::endl;

	os << "#### Source information " << std::endl;
	OS(10) << "ASO =" << aso_ << std::endl;
	os << "xiso(1,1), xiso(2,1) = " << xiso_.at(0).at(0) <<", " << xiso_.at(1).at(0) << std::endl;
	os << "xiso(1,2), xiso(2,2) = " << xiso_.at(0).at(1) <<", " << xiso_.at(1).at(1) << std::endl;
	os << "xiso(1,3), xiso(2,3) = " << xiso_.at(0).at(2) <<", " << xiso_.at(1).at(2) << std::endl;
	OS(10) << "RSO =" << utils::concat(rso_, ", ") << std::endl;
	OS(10) << "ZSO =" << utils::concat(zso_, ", ") << std::endl;
	OS(10) << "PHISO =" << utils::concat(phiso_, ", ")  << std::endl;
	if(isrc_ == 2) {
		OS(10) << "FL =" << utils::concat(fl_, ", ")  << std::endl;
		OS(10) << "FN =" << utils::concat(fm_, ", ")  << std::endl;
		OS(10) << "FN =" << utils::concat(fn_, ", ") << std::endl;
	}

	os << "#### CG information " << std::endl;
	OS(10) << "IVOPT =" << ivopt_ << std::endl;
	OS(10) << "IDBG =" << idbg_ << std::endl;
	OS(10) << "JTY =" << jty_ << std::endl;
	for(size_t i = 0; i < cgbs_.size(); ++i) {
		OS(10) << "CGBody" + std::to_string(i)<< " " << cgbs_.at(i).toInputString() << std::endl;
	}
	for(size_t i = 0; i < cgcs_.size(); ++i) {
		OS(10) << "CGZone" + std::to_string(i) << " " << cgcs_.at(i).toString() << std::endl;
	}
	OS(10) << "NMIZ =" << utils::concat(mmiz_, ", ") << std::endl;

	os << "#### Material information " << std::endl;
	OS(10) << "NBLD =" << nbld_ << std::endl;
	OS(10) << "MATZ =" << utils::concat(matz_, ", ") << std::endl;
	if(nbld_ > 10) {
		OS(10) << "MATGP =" << matgp_ << std::endl;
		OS(10) << "ICGP =" << icgp_ << std::endl;
		OS(10) << "CONGY =" << congy_ << std::endl;
		OS(10) << "MATDOS =" << matdos_ << std::endl;
	}
	for(size_t i = 0; i < comps_.size(); ++i) {
		OS(10) << "COMP"+std::to_string(i) << " =" << utils::concat(comps_.at(i), ", ") << std::endl;
	}
}
#undef OS

