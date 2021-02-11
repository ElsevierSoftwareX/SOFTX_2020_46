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
#include "mcnpinput.hpp"

#include <deque>

#include "modecard.hpp"
#include "core/io/input/common/commoncards.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/ijmr.hpp"
#include "core/io/input/inputdata.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/physics/physconstants.hpp"
#include "core/utils/message.hpp"
#include "core/utils/system_utils.hpp"

namespace {
// regexにmatchしたdataカード全てをlistにして返す
std::list<inp::DataLine> getDataCardsByRegex(const std::regex &reg, const std::list<inp::DataLine> &dataLines)
{
	std::list<inp::DataLine> retList;
	for(auto &dl: dataLines) {
		if(std::regex_search(dl.data, reg)) {
			retList.push_back(dl);
		}
	}
	return retList;
}



}  // end anonymous namespace

void inp::mcnp::McnpInput::init(const std::string &inputFileName)
{
	// titleはセルカードを含むブロックの1行目なのでここでは取得しない
	inp::InputData::init(inputFileName);

	// SC, titleの他、セル名や面名にi j m rを使い得るようにするのでdataList一括のijmr展開はダメ
	// 必要箇所に来たところでinp::DataLine::expandIjmr(std::string*)みたいなのを使う

	/*
	 * Inputdata::initでinclude処理と行中コメント削除をしてしまうと
	 * ここで「継続と後置コメントのみ」の行はここでは削除されているので
	 * blank-line-delimiterとして扱われてしまう
	 *
	 * MCNPでは
	 * ・継続行パターンのみ＋後置コメント  → 前の行に連結したうえで後置コメント削除
	 * ・blank-line-delimiterかつ前置継続行（後置コメントなし)→ blank-line-delimiter扱い
	 * ゆえにここではまだ後置コメントは削除してはならない。(blank-line-delimiterと区別できなくなる)
	 *
	 */

	// まず空白行でカード集ごとに分割。
	std::list<inp::DataLine> tmpDataList;
	for(auto &dl: dataLines_) {
		//mDebug() << std::string("data===") + "\"" + dl.data + "\"";
		// blank-line-delimiter判定。 空白とタブ以外の文字は無い場合。
		if(dl.data.find_first_not_of(" \t") != std::string::npos) {
			// 非デリミタ行
			//mDebug() << "Not delimter !!!!";
			tmpDataList.emplace_back(dl);
		} else {
			//mDebug() << "Blank-line-delimtier";
			// dl.dataがデリミタ かつ tmpDataListがemptyということは連続blank-line-delimite
            if(tmpDataList.empty()) {
                if(!config_.quiet) mWarning(dl.pos(), "sequential multiple blank-line-delimter found. Input file reading ended.");
				//break;
			} else {
				cardsVec_.emplace_back(std::move(tmpDataList));
				tmpDataList.clear();
			}
		}
	}
	if(!tmpDataList.empty())cardsVec_.emplace_back(std::move(tmpDataList));




	// 末尾の空白は別に認めてもいいのでcardsVecを後ろから空白が続く時は削除する。
	while(cardsVec_.back().empty()) cardsVec_.pop_back();


    /*
     * The structure of mcnp input is
     * 1. messageblock(optional)
     * 2. title
     * 3. cell-block
     * 4. surf-block
     * later than 5. enything-else-is-acceptable
     *
     * Therefore
     * 1st Remove message block,
     * 2nd Remove title(even comment-outed line shoud be treated as title!)
     * 3rd Remove comments
     */

    /*
     * Message blockの解釈は以下の通り
     * message:以下,次のblank-line-delimiterまでがmessageblockになる。
     * ・コメントアウトは普通の入力業と同じ
     * ・(継続行パターンが無くても)複数行継続
     * ・内容はコマンドライン引数と同じ(inp以外)
     *
     */


    const std::list<inp::DataLine> &messageBlock = cardsVec_.front();
    inp::DataLine messageDataLine = messageBlock.front();
    std::smatch sm;
    if(std::regex_search(messageDataLine.data, sm, std::regex(R"(^ *message:(.*)$)", std::regex_constants::icase))) {
        // 1st block is a message-block
        message_ = sm.str(1);
        // concat message block
        for(auto it = messageBlock.cbegin(); it != messageBlock.cend(); ++it) {
            if(it != messageBlock.cbegin()) message_ += " " + it->data;
        }
        // remove message block
        cardsVec_.pop_front();
        //mDebug() << "message_===" << message_;
    }

	// ブロック数のチェックとタイトル取得  
    if(cardsVec_.size() < 3 || cardsVec_.size() >= 4) {
		std::stringstream errss;
        for(size_t i = 0; i < cardsVec_.size(); ++i) {
			// フルパスだとわけわからんからここでファイル名だけにする。
            auto fpair = utils::separatePath(cardsVec_.at(i).front().file);
            errss << fpair.second + ":" + std::to_string(cardsVec_.at(i).front().line);
            if(i != cardsVec_.size()-1) errss << ", ";
		}
		std::string errmess = errss.str();
        if(cardsVec_.size() < 3) {
            std::stringstream ss;
            ss << cardsVec_.front().front().pos()
               << " Number of card blocks should be more than 3, actual="
               << cardsVec_.size() << ". Blocks started at " << errmess;
            throw std::invalid_argument(ss.str());
        } else {
            if(!config_.quiet)mWarning(cardsVec_.front().front().pos(),
                "Number of card blocks should be more than 3, actual=",
                cardsVec_.size(), ". Blocks started at ", errmess);
        }
    }

    title_ = cardsVec_.front().front().data;
    cardsVec_.front().pop_front();

//    mDebug() << "title===" << title_;
//    mDebug() << "CardsVec size===" << cardsVec_.size();
//    for(size_t i = 0; i < cardsVec_.size(); ++i) {
//        mDebug() << "i ===" << i << "CardsSize====" << cardsVec_.at(i).size();
//        mDebug() << cardsVec_.at(i);
//    }




	// mcnpではタイトル行がコメント行の場合でもタイトル行として解釈するので、コメント処理はタイトル取得後
	for(size_t i = 0; i < cardsVec_.size(); ++i) {
		for(auto it = cardsVec_.at(i).begin(); it !=  cardsVec_.at(i).end(); ++it) {
			comm::removeMcnpPostComment(&(it->data));    // mcnp式後置コメントの除去
			comm::removePhitsPostCommentNotSharp(&(it->data));  // phits式後置コメントの除去
		}
		// 行頭コメントアウト行のカット
        InputData::removeComment(config_.mode, &cardsVec_.at(i));
		/*
		 *  継続行連結はコメント削除後にしないと、コメントを挟んだ場合、コメント行に連結されてしまう。
		 */
		inp::InputData::concatLine(&cardsVec_.at(i), true, false);
	}

//    mDebug() << "\n 継続処理、コメント除去後";
//    mDebug() << "title===" << title_;
//    mDebug() << "CardsVec size===" << cardsVec_.size();
//    for(size_t i = 0; i < cardsVec_.size(); ++i) {
//        mDebug() << "i ===" << i << "CardsSize====" << cardsVec_.at(i).size();
//        mDebug() << cardsVec_.at(i);
//    }



    // 最初に構築すべきはMaterialsなので、そのためのvector<ptype>をDatacardsから取得する
    assert(cardsVec_.size() >= 3);

//    for(size_t i = 0; i < cardsVec_.size(); ++i) {
//        mDebug() << "\ncard block number i === " << i;
//        for(auto &dl: cardsVec_.at(i)) {
//            mDebug() << dl;
//        }
//    }


	cellCards_ = cardsVec_.at(0);
	surfaceCards_ = cardsVec_.at(1);
	// Material名にstringを受け入れたい。かつModeとかぶらないように、カード名の次の入力は数値という条件を追加。



	/*
	 *
	 * materialCards_ = getDataCardsByRegex(std::regex(R"(^ {0,4}m(\"\w+\"|[0-9]+) +)", std::regex_constants::icase), cardsVec_.at(2));
	 * transformCards_ = getDataCardsByRegex(inp::comm::TrCard::regex(), cardsVec_.at(2));
	 * ↑だと↓のような6013の継続行をミスっているイリーガルな入力がエラーにならない
	 * M1 6000 1.0
	 * M2 6012 0.989
	 *   6013 0.011
	 * M3 6012 1.0
	 *
	 * 一行レベルでの正規表現マッチングでmatcard抜き出ししているから。
	 */

	// 真面目なデータカードからのmat,trカード抽出
	const std::regex matCardRegex(R"(^ {0,4}m(\"\w+\"|[0-9]+) +)", std::regex_constants::icase);
	const std::regex trCardRegex = inp::comm::TrCard::regex();
	const std::regex errorRegex(R"(^ {0,4}[0-9]+)");  // 数字から始まるカードは明らかにエラー
	for(const auto &dl: cardsVec_.at(2)) {
		if(std::regex_search(dl.data, matCardRegex)) {
			materialCards_.emplace_back(dl);
		} else if(std::regex_search(dl.data, trCardRegex)) {
			transformCards_.emplace_back(dl);
		} else if(std::regex_search(dl.data, errorRegex))
			throw std::runtime_error(dl.pos() + std::string(" \"") + dl.data +"\" is an invalid data card.");
	}


	// mcnp/phits共通カードはここで小文字化する。 TODO セルカードはtfile=があるのでできればここではまだ小文字化したくない
	InputData::toLowerDataLines(&cellCards_);
	InputData::toLowerDataLines(&surfaceCards_);
	InputData::toLowerDataLines(&materialCards_);
	InputData::toLowerDataLines(&transformCards_);

	/*
	 * ここでijmr展開したいが、 セルカードでのijmrは
	 * ・fill=でuniv番号指定でRを使用する。
	 * ・TRCL, FILL時TRCLで ijmr 全て使用する。
	 * わけであり、セル名に文字列を認めるのなら、引数、セル名を区別する必要があり、
	 * またi,j,m,rは予約語としてセル名、サーフェイス名として認めない措置が必要。
	 *
	 * → 結局CellCard::fromStringにijmr展開を移したのでここでは扱わない。
	 *
	 * TODO surfaceCardsは？
	 * material, transformはここで展開する。
	 */
	for(auto &dl: transformCards_) {
		inp::ijmr::expandIjmrExpression(" ", &dl.data);
	}


	/*
	 *  mcnpのxsdir設定は
	 * 1．実行時にxsdir=で指定する。 → コンストラクタでxsdirFileName_にセット済み
	 * 2. メッセージブロックでの指定
	 * 3．入力ファイルのメッセージブロックでのDATAPATH=datapath の下のxsdir
	 * 4．カレントディレクトリ
	 * 5．環境変数
	 */
    if(config_.xsdir.empty()) {
		if(!xsdirArg().empty()) {
            config_.xsdir = xsdirArg();
        } else if(!datapath().empty()) {
            config_.xsdir  =  datapath() + PATH_SEP + "xsdir";
		} else {
            config_.xsdir  = InputData::getAlternativeXsdirFileName(config_.quiet);
		}
	}
    if(!config_.xsdir.empty()) {
        if(utils::isRelativePath(config_.xsdir)
            && !path_.empty()) config_.xsdir  = path_ + PATH_SEP + config_.xsdir;

	}
	// TODO データカードは個別カードに振り分けたほうが良いかも。全カードのregex適用するのは無駄だから。

	// DEBUG
    if(config_.verbose)mDebug() << cellCards_;
    if(config_.verbose)mDebug() << surfaceCards_;
}



std::string inp::mcnp::McnpInput::datapath() const
{
	auto dpath = datapathArg();
	if(dpath.empty()) {
		return dpath;
	} else if(utils::isRelativePath(dpath)) {
		return path_ + PATH_SEP + dpath;
	} else {
		return dpath;
	}
}

const std::string inp::mcnp::McnpInput::xsdirArg() const
{
	std::smatch sm;
	std::string xsdirArg;
	if(std::regex_search(message_, sm, std::regex(R"(xsdir[= ] *(\S*))", std::regex_constants::icase))) {
		xsdirArg = sm.str(1);
		utils::trim(&xsdirArg);
	}
	return xsdirArg;
}

const std::string inp::mcnp::McnpInput::datapathArg() const
{
	std::smatch sm;
	std::string datapath;
	if(std::regex_search(message_, sm, std::regex(R"(datapath[= ] *(\S*))"))) {
		datapath = sm.str(1);
		utils::trim(&datapath);
	}
	return datapath;
}

// カード名を引数にとり、返り値はカードのDataLineとカードの(カード名を除いた)引数のペア
// 対象はデータカード。第二引数はカードが重複して出た時に警告するかどうか。
// 重複カードは最後に出たものが有効になる。
std::pair<inp::DataLine, std::string> inp::mcnp::McnpInput::getDataCardAndArg(const std::string &cardName, bool warnDuplicated) const
{
	std::string regStr = "^ {0,4}" + cardName + "[ =](.*)$";
	std::regex cardReg(regStr);
	std::smatch sm;
	std::string arg;
	std::string prevPos;
	DataLine dataLine;
	for(auto &dl: cardsVec_.at(2)) {
		if(std::regex_search(dl.data, sm, cardReg)) {
			if(warnDuplicated && !arg.empty()) {
				mWarning(dl.pos()) << "Multiple card, name =" << cardName << ", previously input at " << prevPos;
			}
			prevPos = dl.pos();
			arg = sm.str(1);
			dataLine = dl;
		}
	}
	//if(arg.empty()) throw std::invalid_argument("No card found (card name = " + cardName + ")");
	return std::make_pair(dataLine, arg);
}


std::vector<phys::ParticleType> inp::mcnp::McnpInput::particleTypes() const
{
	std::list<inp::DataLine> modeLines = getDataCardsByRegex(ModeCard::regex(), cardsVec_.at(2));

	inp::DataLine modeLine;
	if(modeLines.size() == 0) {
		mWarning() << "MODE card is missing, MODE N was applied.";
		modeLine.data = "MODE N"; // Nがデフォルト値
	} else if(modeLines.size() != 1) {
		std::string err = "Multiple mode cards, location =";
		for(auto &dl: modeLines) err += " " + dl.pos();
		mWarning(modeLines.front().pos()) << err;
		modeLine = modeLines.front();
	} else {
		modeLine = modeLines.front();
	}

	auto modecard = mcnp::ModeCard(modeLine.data);

	std::string arg = modecard.argument();
	utils::tolower(&arg);
	auto modeArgVec = utils::splitString(" ", arg, true);
	std::vector<phys::ParticleType> ptypes;
	for(auto &modeStr: modeArgVec) {
		ptypes.emplace_back(phys::strToParticleType(modeStr));
	}

    if(config_.verbose) mDebug() << "particle types:" << ptypes;
//	for(size_t i = 0; i < ptypes.size(); ++i) {
//		mDebug() << "   i===" << i << phys::particleTypeTostr(ptypes.at(i));
//	}
	return ptypes;
}


