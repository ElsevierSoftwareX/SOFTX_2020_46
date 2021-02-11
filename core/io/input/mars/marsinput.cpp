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
#include "marsinput.hpp"

#include <set>
#include <sstream>
#include <string>
#include "core/utils/message.hpp"
#include "core/utils/system_utils.hpp"
#include "core/io/input/common/commoncards.hpp"
#include "core/io/input/cg/cgbody.hpp"
#include "core/io/input/cg/cgzone.hpp"

void inp::mars::MarsInput::init(const std::string &inputFileName)
{
	/*
	 * この関数では基底クラスの4リストlist<DataLine> cellCards_, surfaceCards_, materialCards_, transformCards_;
	 * を初期化する。
	 * MARSではtransformは無いのでのととりあえずmaterialはよくわからないのでcell, surfaceを作成する。
	 */

	/*
     * MARSのメタカードはmcnp/phitsと割と異なるのでInputData::initは使えない。 自力で処理すべし。
	 * といっても継続行があるのは一部のカードだけなので、結局処理すべきは
	 * 勝手に拡張したコメントの削除くらい。
	 */

	/*
	 * トーラスは角度指定があるので、1サーフェイスカードでは表現できない。
	 * 適当にRPPを自動生成して複数カードで表現する。
	 * とくにセルカードの方で面倒なことになるが仕方ない。
	 */

	inputFile_ = inputFileName;
	path_ = utils::separatePath(inputFile_).first;
	// こここでインクルード処理されてしまうが別によい。CRの除去とかもしてくれるし。
	dataLines_ = InputData::readFile("", inputFile_);

	// 独自拡張として、MCNP/PHITS風の行頭・行末コメントアウトを有効にしたいので、ここでコメント除去
	// しかし、CGのニモニックでC（無限円筒)というものがあるため、コメントと区別できない。
	// 従ってMARSの場合文頭コメントアウトは採用不可
	//InputData::removeComment(mode_, &dataLines_);

	for(auto it = dataLines_.begin(); it != dataLines_.end(); ++it) {
		comm::removeMcnpPostComment(&(it->data));    // mcnp式後置コメントの除去
		comm::removePhitsPostCommentNotSharp(&(it->data));  // phits式後置コメントの除去
	}

	// 自由形式入力の読み出しがやりやすいように、最初にファイルのデータは全てstringstreamに読み込んでおく。
	std::stringstream ss;
	for(const auto& dl: dataLines_) ss << dl.data << std::endl;

	int lineNumber = 1;
	std::string buff;
	// とりあえず最初3行はヘッダとする。
	getline(ss, buff);
	getline(ss, buff);
	getline(ss, buff);
	lineNumber = 3;

	std::vector<CGBody> cgbodies;
	try {
		cgbodies = CGBody::getCgBodiesFromQadFixed(ss, lineNumber);
	} catch(std::exception &e) {
		std::stringstream sse;
		sse << "Exception, what = " << e.what();
        throw std::invalid_argument(inputFile_ + ":" + std::to_string(lineNumber) + " " + sse.str());
	}

	std::vector<CGZone> cgzones;
	try {
		cgzones = CGZone::getCgZonesFromMarsFixed(ss, lineNumber);
	} catch(std::exception &e) {
		std::stringstream sse;
		sse << "Exception, what = " << e.what();
        throw std::invalid_argument(inputFile_ + ":" + std::to_string(lineNumber) + " " + sse.str());
	}


    int line = 0;
    if(config_.verbose) mDebug() << "Cell card from MARS-CG input;";
    // #### Cellカードはちょっとむずかしい。
	for(size_t i = 0; i < cgzones.size(); ++i) {
		std::string cellCardStr = std::to_string(i+1) + " ";
		std::string matNumber = utils::trimmed(cgzones.at(i).ialp());
		if(matNumber.front() == '-') {
			matNumber = "0";
		} else {
			matNumber = matNumber + " -1.0 ";
		}
		cellCardStr += matNumber + " "; // FIXME 暫定。密度データの設定がわからないのでとりあえず-1。
		cellCardStr += cgzones.at(i).equation();
        if(config_.verbose)mDebug() << cellCardStr;
		cellCards_.emplace_back(DataLine(inputFile_, ++line, cellCardStr));
	}

    if(config_.verbose)mDebug() << "Surface card from MARS-CG input;";
	// #### surfaceカード作成
	for(size_t i = 0; i < cgbodies.size(); ++i) {
		std::string surfInp = cgbodies.at(i).toInputString();
		utils::tolower(&surfInp);
        if(config_.verbose) mDebug() << surfInp;
		surfaceCards_.emplace_back(DataLine(inputFile_, ++line, surfInp));
	}
	// ### Materialカード作成。FIXME 現段階ではただのダミー生成
	std::set<std::string> matList;
	for(const auto& zone: cgzones) matList.emplace(zone.ialp());
	for(const auto& matName: matList) {
		std::string trimmedMatName = utils::trimmed(matName);
		if(trimmedMatName.empty() || trimmedMatName.front() == '-') continue;
		std::string matEntry = "m" + trimmedMatName + " 1000 1";
        if(config_.verbose)mDebug() << "matcard===" << matEntry;
		materialCards_.emplace_back(DataLine(inputFile_, ++line, matEntry));
    }
    if(!config_.quiet) mWarning() << "Current material data for MARS-CG is dummy.";
}

std::vector<phys::ParticleType> inp::mars::MarsInput::particleTypes() const
{
    if(!config_.quiet) mWarning() << "Current particle data for MARS-CG is dummy.";
	return std::vector<phys::ParticleType>{phys::ParticleType::PHOTON};
}
