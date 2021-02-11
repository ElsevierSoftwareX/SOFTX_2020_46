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
#include "cellcreator.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#ifdef ENABLE_GUI
#include <QApplication>
#include <QProgressDialog>
#endif

#include "core/geometry/cell_utils.hpp"
#include "core/geometry/latticecreator.hpp"
#include "core/geometry/surfacecreator.hpp"
#include "core/geometry/tetracreator.hpp"
#include "core/formula/logical/lpolynomial.hpp"
#include "core/io/input/cardcommon.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/io/input/filldata.hpp"
#include "core/io/input/surfacecard.hpp"
#include "core/utils/utils.hpp"
#include "core/utils/system_utils.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/geometry/cell/cell.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surface/surface.hpp"
#include "core/geometry/surface/surfacemap.hpp"

/*
 * cell カードを解釈し、cellオブジェクトを生成する。
 * NAME MAT RHO  EQUATION
 */

namespace {
const char SELF_SUFFIX[] = "_self";

std::string getSelfUnivName(const std::string &univName, const std::string &cellName)
{
	//std::cout << "creating self universe name fron cellName={" << cellName << "}univName={" << univName << "}" << std::endl;
	if(univName.empty()) abort();
	return univName + "_" + cellName + SELF_SUFFIX;
}

}  // end anonymous namespace


std::shared_ptr<const geom::Cell> geom::CellCreator::createCell(inp::CellCard card) const
{
	// TRCLはこの関数の外で処理していてここでは処理済みとする。故にtransform matrixは引数には不要
	// と思っていたが、bb="xmin,xmax..." もここより前で変換しておく。


	//mDebug() << "Creationg cell from card===" << card.toInputString();
	//mDebug() << "Creating cell eq===" << card.equation;
	auto poly = lg::LogicalExpression<int>::fromString(card.equation, surfCreator_->map().nameIndexMap());


	std::shared_ptr<Cell> cell;
	if(card.materialName == "-1") {
		cell = std::make_shared<Cell>(card.name, surfCreator_->map(), poly, -1);  // 最外殻セル
	} else 	if(card.materialName == "0") {
		cell = std::make_shared<Cell>(card.name, surfCreator_->map(), poly, 1);
	} else {
//		mDebug() << "material name=" << card.materialName;
//		for(auto &mpair: materialMap_) {
//			mDebug() << "mat=" << mpair.first;
//		}
		if(materialMap_.find(card.materialName) == materialMap_.end()) {
			throw std::out_of_range(std::string("Material \"") + card.materialName + "\" not found");
		}
		cell = std::make_shared<Cell>(card.name, surfCreator_->map(), poly,
									  materialMap_.at(card.materialName),
									  card.density, 1.0);
	}
	// cardのパラメータにBBがあった場合はBBを作成する。
	// TODO セルカードパラメータもCellのコンストラクタへ渡してコンストラクト時に初期BBを計算できるようにする。
	auto it = card.parameters.find("bb");
	if(it != card.parameters.end()) {
		cell->setInitBB(BoundingBox::fromString(it->second));
	}
	return cell;
}

// NOTE 第三引数のmmapはxsdirが未指定なら空になっているのでアクセス時には用チェック
geom::CellCreator::CellCreator(const std::list<inp::DataLine> &cellInputs, SurfaceCreator *sCreator,
                               const std::unordered_map<std::string, std::shared_ptr<const mat::Material>> &mmap,
                                bool warnPhitsCompat, int numThread, bool verbose)
	:surfCreator_(sCreator), materialMap_(mmap)
{
//	mDebug() << "material map size===" << mmap.size();
//	for(auto mp: mmap) {
//		mDebug() << "mp.first===" << mp.first;
//	}
	/*
	 * セルの他のセルへの依存性問題
	 * 補集合#, like-but, fill= が指定されているセルは他のセルに依存する。
	 * 補集合やlikeのターゲットとなっているセルの定義でさらに補集合/likeを使っている場合や、
	 * 多重universeの場合セル依存性は連鎖構造となる。
	 *
	 * fill-universeの場合、セルの形状自体には影響が出ず、独立したセル間の連結構造が変わるだけ、
	 * と考えられセル構築後にセルの内外関係を構築すれば、createCell時には考えなくても良いかもしれない。
	 * とりあえずの実装ではfillされたcellは中側にある(複数の)セルで置き換えることにする。
	 *
	 * セルカードの依存性解決は以下の順序で行う
	 * 1．補集合とlik-butを同時に解決
	 * 2．TRCLの処理
	 * 3．fill/universe
	 *
	 * TRCLをfill/universeの前に処理しておかないと、universeでTRCLが使われていた時、
	 * filledセルの外側境界にはTRCLは適用せず、中身にはTRCL適用する、というように
	 * セルの一部のsurfaceにだけTRCLを適用するようなケースが生じ、処理が大変になるから。
	 *
	 */

//    mDebug() << "セルクリエイターへ渡されたDatalineリスト";
//    for(auto &dl: cellInputs) {
//        mDebug() << "dl=====" << dl.data;
//    }

	// セル名をキーにしたセルカードのマップを作成。
	std::unordered_map<std::string, inp::CellCard> solvedCards;
	// ############ 補集合とlike-butの依存性を解決する###############
	solveCellDependency(warnPhitsCompat, cellInputs, &solvedCards);
	warnUnusedSurface(solvedCards);  // 使わないsurfaceがある場合はここで警告

//    mDebug() << "\n依存性解決後のセルカード一覧";
//    for(auto cardPair: solvedCards) {
//        mDebug() << cardPair.second.toString();
//    }


	// ############ 同階層の明示的TRCLを適用する##########################
	// 明示的、とはセルカード中でTRCL=で指定しているもの。fill, latに伴うものは含めない
	applyExplicitTRCL(&solvedCards);

//    mDebug() << "\n明示的TRCL解決後のセルカード一覧.";
//    for(auto cardPair: solvedCards) {
//        mDebug() << cardPair.second.toInputString();
//    }
//	mDebug() << "サーフェイス一覧";
//	mDebug() << surfCreator_->surfaceMap_.frontSurfaceNames();
////	for(auto &bs: surfCreator_->surfaceMap_.backSurfaces()) {
////		if(bs) mDebug() << bs->toInputString();
////	}








	// ############# lattice/universeの解決 ############################
	// lattice定義セルカードから各Latice要素のセルカードを作成する。(要素セル内部は未充填)
	appendLatticeElements(&solvedCards);






//	mDebug() << "Lattice解決後のサーフェイス一覧";
//	mDebug() << surfCreator_->surfaceMap_.frontSurfaceNames();
//	for(auto &sp: surfCreator_->surfaceMap_.frontSurfaces()) {
//		mDebug() << sp.second->toInputString();
//	}

//	mDebug() << "\nLattice解決終了後のセルカード";
//	for(auto &cardPair: solvedCards) {
//		mDebug() << "card==" << cardPair.second.toString();
//	}
//	mDebug();

    // FILLすると何故か数式に余計な空白や括弧が付いてくることがある？空白くらいなら良いが中身が空の括弧は気持ち悪い。


    // FILL時に外側セルのBBで内側セルのBBをカットするかどうかにてついては
    // それにまともに意味があるのはLAT=3の四面体周辺セルの場合のみなので
    // 大半のケースでは無駄になるため実施しない。

	// 次にfillされているセルカードを置き換える。その後orderをキーにしたマルチマップに格納する。
	// この時TRで必要となるsurfaceを生成しsurfaceMap_へ登録する。
	// orderをキーとしたマップ.オーダーはセル生成順序で負のオーダーセルは粒子追跡に不要なセル
	// 実際にはorderは優先順位ではなく、生成不要セル(univ定義セル等)の生成を抑制するフラグとして使われる。
	// なのでここではbooleanのメンバでも作ってしまえば不要
	// TODO vectorでmake_move_iterator使うのが多分速い
	//std::multimap<int, inp::CellCard> cellcards;
	std::vector<inp::CellCard> cellcards;
    fillUniverse(numThread, solvedCards, &cellcards, verbose);
	solvedCards.clear();

	/*
	 *  bb=でBBが手動定義されていてかつTRCLが存在する場合、bbの引数にもTRを適用する必要がある。
     * Lattice要素セルにはLatticeカード内の明示的TRCLは適用済みだが、
     * 外側セルのTRCLは要素生成時には適用できない（外側セル情報はそのときはまだない）
     * のでFILL時TRCLは最後に一括して適用する。
	 */

	//mDebug() << "\nunivrse解決後のセルカード一覧";
	// ############# セル生成 ############################################
	for(auto it = cellcards.begin(); it != cellcards.end(); ++it) {
		// ユニバース定義セルや、外側セルは粒子追跡に使わないのでインスタンスを作らない。
		if(it->parameters.find("u") == it->parameters.end()) {
			//mDebug()<< "実際に作成するcellcard=" << it->toInputString();

			try{
				// TODO ここで(Xsdir::awr質量テーブルか個別の核種ファイルの質量)適当な核種質量を使って原子数密度は全て重量密度に変えてしまおう！！
				// 原子数密度→質量密度へは組成の平均原子重量が必要。個別の核種の質量はaceファイルかawrテーブルに記載されている。
				// 故にここでは組成データが必要となる。
				inp::CellCard &card = *it;

				// BBとTRCLが両方ある場合はbbにもTRCL適用する
				auto bbIt = card.parameters.find("bb");
				if(bbIt != card.parameters.end()) {
					if(card.hasTrcl()) {
						auto matrix = utils::generateTransformMatrix(surfCreator_->transformationMatrixMap(), card.trcl);
						auto bb = geom::BoundingBox::fromString(bbIt->second);
						bb.transform(matrix);
						bbIt->second = bb.toInputString();
					}
					// TODO ここ文字列から一回BB作ってもう一度文字列に戻す(そして後でまたBB作成する)の非効率
				}

				std::string bbstr;
				if(card.parameters.find("bb") != card.parameters.end()) bbstr = card.parameters.at("bb");
				//mDebug() << "最終的にセル生成するカード===" << card.name << "eq===" << card.equation << "trcl=" << card.trcl << "bb=" << bbstr;
				cells_.emplace(card.name, createCell(card));
            } catch (std::out_of_range &oor) {
                std::stringstream sse;
                sse << it->pos() << " Parameter used in the cell card is invalid." << oor.what();
                throw std::invalid_argument(sse.str());
			} catch (std::exception &e) {
                std::stringstream sse;
                sse << it->pos() << " While creating cell =" << it->name << ", " << e.what();
                throw std::invalid_argument(sse.str());
			}
		}
	}

//	mDebug() << "\n最終的に残った粒子追跡に使うセル";
//	for(auto &elem: cells_) {
//		mDebug() << "cell===" << elem.second->toString();
//	}
//	mDebug() << "サーフェイス一覧";
//	//	mDebug() << surfCreator_->surfaceMap_.frontSurfaceNames();
//	for(auto &bs: surfCreator_->surfaceMap_.frontSurfaces()) {
//		if(bs) mDebug() << bs->toString();
//	}



//	mDebug() << "STOP for DEBUGGING.";
//	abort();

	// Cell-Surface連結構造をupdateする
	utils::updateCellSurfaceConnection(cells_);

//	mDebug() << "#### SurfaceMap ####";
//	surfCreator_->surfaceMap_.dumpFrontSufraceList(std::cout);
}



geom::CellCreator::CellCreator(const std::unordered_map<std::string, std::shared_ptr<const Cell>> &cellMap)
	:cells_(cellMap)
{
	// Cell-Surface構造をupdateする
	utils::updateCellSurfaceConnection(cells_);

}

void geom::CellCreator::warnUnusedSurface(const CardMap &solvedCards) const
{
	std::set<std::string> usedSurfaceNames;
	for(auto &cardPair: solvedCards) {
		auto tmpSet = inp::GetSurfaceNames(cardPair.second.equation);
		usedSurfaceNames.insert(tmpSet.begin(), tmpSet.end());
	}
//	for(auto &cardPair: dependingCards) {
//		auto tmpSet = GetSurfaceNames(cardPair.second.equation);
//		usedSurfaceNames.insert(tmpSet.begin(), tmpSet.end());
//	}
	auto frontSurfaces = surfCreator_->map().frontSurfaces();
	for(auto it = frontSurfaces.begin(); it != frontSurfaces.end(); ++it) {
		if(usedSurfaceNames.find((it->second)->name()) == usedSurfaceNames.end()) {
			// NOTE ここの未使用surface警告の動作がおかしい？
			// TRCLの元となるsurfaceは必要不可欠だが未使用なので警告がでてしまう？テストケースを作成すべし。
			//mWarning() << "Surface \"" << (*it)->name() << "\" is defined in surface card, but not used.";
		}
	}
}

void geom::CellCreator::solveCellDependency(bool warnPhitsCompat, const std::list<inp::DataLine> &cellInputs, CardMap *solvedCards) const
{
	// ################################ 補集合、like-butの解決 ######################
    //mDebug() << "comlpiment, like-but展開開始";
	CardMap dependingCards;

	for(auto &element: cellInputs) {
		try{
            //mDebug() << "\n\n入力からセルカード作成, str=" << element.data;
			inp::CellCard card = inp::CellCard::fromString(element.file, element.line, element.data);
            //mDebug() << "card===" << card.toString();
			//assert(!"Debug abort");
			auto depCardIt = dependingCards.find(card.name);
			auto solCardIt = solvedCards->find(card.name);
			if(depCardIt != dependingCards.end()){
				throw std::invalid_argument(std::string("Multiple cell definition found. cell name =\"") + card.name + "\" at " + depCardIt->second.pos());
			} else if (solCardIt != solvedCards->end()) {
				throw std::invalid_argument(std::string("Multiple cell definition found. cell name =\"") + card.name + "\" at " + solCardIt->second.pos());
			} else if (warnPhitsCompat && !utils::isArithmetic(card.name)) {
				mWarning(element.pos()) << "Non-integer cell name \"" << card.name << "\" is not phits-compatible.";
			}
			// コンプリメント、likebutが使われているセルはdependingCardsに、それ以外はsolvedCardsに追加。
			if(!card.depends.empty() || !card.likeCell.empty()) {  // surfaceコンプリメントは1（セルカードのコンストラクタで）処理済みのはずなのでdepends.emptyをチェック
				dependingCards.emplace(card.name, card);
			} else {
				solvedCards->emplace(card.name, card);
			}
		} catch(std::exception &e) {
            throw std::invalid_argument(element.pos() + " While reading cell card,,, " + e.what());
		}
	}

//    mDebug() << "依存性解決前の独立セルカード";
//    for(auto card: *solvedCards) {
//        mDebug() << card.second.toString();
//    }
//    mDebug() << "従属セルカード";
//    for(auto card: dependingCards) {
//        mDebug() << card.second.toString();
//    }

	// 次に依存関係の解決。この時依存先がTRCLしているなら依存先の論理多項式はTRCL後の名前にする。
	while(!dependingCards.empty()) {
		auto dpendingCardCount = dependingCards.size();
		for(auto it = dependingCards.begin(); it != dependingCards.end();) {
			inp::CellCard *dcard = &(it->second);

			// 依存先のセル名も正準化しておかないと依存先の置換が失敗してしまう。
			std::smatch dcMatch;
			auto searchStartIt = dcard->equation.cbegin();
			while(std::regex_search(searchStartIt, dcard->equation.cend(), dcMatch, std::regex(R"(# *([0-9a-zA-Z_]*))"))) {
				std::string dcellName = utils::canonicalName(dcMatch.str(1));
				//mDebug() << "DendingcellName org, canonical ===" <<dcMatch.str(1) << dcellName;
				auto sharpPos = std::distance(dcard->equation.cbegin(), dcMatch[0].first);
				auto replacedEndPos = sharpPos + dcellName.size() +1;
				dcard->equation.replace(dcMatch[0].first, dcMatch[0].second, "#" + dcellName);
				searchStartIt = dcard->equation.cbegin(); // これは有効なiteratorか？
				std::advance(searchStartIt, replacedEndPos);
			}

			// 依存性が解決できたらsolvedCardsに追加し、dependingCardsから削除する。
			//mDebug() << "解決試行：解決対象従属セルカード===" << dcard->toInputString();
			if(inp::CellCard::solveCellCardDependency(surfCreator_->map().nameIndexMap(), *solvedCards, dcard)) {
				//mDebug() << "依存性解決成功結果===" << dcard->toInputString();
				solvedCards->emplace(dcard->name, *dcard);
				it = dependingCards.erase(it);
			} else {
				//mDebug() << "解決不能でした===" << dcard->toInputString();
				++it;
			}
		}
		// whileループを1回通過して解決されたカードが1個もない場合は循環参照等で解決不能になっている。のでfatal
		if(dpendingCardCount == dependingCards.size() && !dependingCards.empty()) {
			// エラー表示には行番号で並んでいた方が都合が良いので行番号をキーにしたmapに詰め替える。
			std::map<int, inp::CellCard> depCards;
			for(auto it = dependingCards.cbegin(); it != dependingCards.cend(); ++it) {
				depCards.emplace(it->second.line, it->second);
			}
			std::string message;
			for(auto it = depCards.cbegin(); it != depCards.cend(); ++it) {
				// 最初のカードだけは絶対パスにしてあとは相対パスで。
				// 見やすいand InputViewerで位置がわかるように。
				if(it == depCards.cbegin()) {
					message += it->second.pos() + " ";
				} else {
					message += utils::separatePath(it->second.file).second + ":" + std::to_string(it->second.line) + " ";
				}
			}
			message += "Cell dependncy cannot be resolved, ";
			message += "depending on non-existing cell(s) or circular dependency.\n";
			for(const auto &cellPair: dependingCards) {
				std::stringstream cellss;
				cellss << "Cell " << cellPair.first
					   << " depends on cell(s) " << PRINTABLE<std::vector<std::string>>(cellPair.second.depends) << std::endl;
				message += cellss.str();
			}
            throw std::runtime_error(message);
		}
	}
//    mDebug() << "依存性解決 後 の独立セルカード";
//    for(auto card: *solvedCards) {
//        mDebug() << card.second.toString();
//    }

}

// CardMap はセル名をキーとして、セルカードを値としたunordered_map
void geom::CellCreator::applyExplicitTRCL(geom::CellCreator::CardMap *solvedCards)
{
	// ############################## （同階層の）TRCLの解決 ################################
	/*
	 * TRCLを処理してsurfaceMap_に書き込む
	 * 1. TR行列を作成
	 * 2．セルカードの多項式内のsurface名をTR後のものに置換
	 * 3．TRしたsurfaceを新たに作成(※他のセルからはTRしていないsurfaceを参照しているのでTR前surfaceは削除してはならない)
	 * 4．作成したsurfaceをsurfaceマップに登録
	*/
	std::list<std::tuple<std::string, std::string, std::string>>  trCreatingSurfaceList;
	for(auto &cardPair: *solvedCards) {
//        mDebug() << "cellName===" << cardPair.first << "card ===" << cardPair.second.toString();
        /*
         * solveTrclは
         *  TRCLによってセルカードの多項式中に含まれるsurface名をTR後のものに変更し、
         *  "TR対象surface名, TRCLしたセル名、TRSF文字列"のタプルリストを返す。
         */

        auto tmpList = cardPair.second.solveTrcl();
//        mDebug() << "After card ===" << cardPair.second.toString();
//        for(const auto &tup: tmpList) {
//            mDebug() << "surf===" << std::get<0>(tup) << ", cell===" << std::get<1>(tup) << ", trsf===" << std::get<2>(tup);
//        }


        trCreatingSurfaceList.insert(trCreatingSurfaceList.end(), tmpList.begin(), tmpList.end());
	}
	// TRCLで発生した新しいSurfaceの作成と登録
	for(auto &surfTuple: trCreatingSurfaceList) {
//        mDebug() << "TRで新たに作成するsurfaceタプル(TR対象surface, 発生cell, trsf)="
//                 << std::get<0>(surfTuple) << ", " << std::get<1>(surfTuple) << ", " << std::get<2>(surfTuple);
		surfCreator_->surfaceMap_.registerTrSurface(surfCreator_->transformationMatrixMap(), surfTuple);
	}

}

void geom::CellCreator::appendLatticeElements(geom::CellCreator::CardMap *solvedCards)
{
	// ################# LatticeセルのFILLは独特なのでuniverseセルの前に処理する。
	std::vector<inp::CellCard> latticeElemCards;
	for(auto it = solvedCards->begin(); it != solvedCards->end();) {
		inp::CellCard latticeCard = it->second;
		assert(latticeCard.depends.empty()); // ここまでにカード間の依存性(universe除く)は解決されているはず
		// latが無いカードの場合はcontinue
		if(latticeCard.parameters.find("lat") == latticeCard.parameters.end()) {
			++it;
			continue;
		}

		//mDebug() << "latticecard===" << latticeCard.toString();

		// 以下LAT処理開始
		const int latvalue = utils::stringTo<int>(latticeCard.parameters.at("lat"));
		bool isSelfFilled = false;  // fill=で自己参照しているか、fill=が省略されている場合はtrue
		// cosine指定は既に適用済みのはずなのでここで発見したらプログラムエラー
		assert(latticeCard.parameters.find("*fill") == latticeCard.parameters.end());
		// LAT=3の場合fillは不要なので警告不要
		if(latticeCard.parameters.find("fill") == latticeCard.parameters.end() && latvalue != 3) {
			// fillパラメータなしの場合自分自身ユニバース、つまりu=selfで無限全要素埋め
			latticeCard.parameters["fill"] = getSelfUnivName(latticeCard.parameters.at("u"), latticeCard.name);
			isSelfFilled = true;
			mWarning(latticeCard.pos()) << "No  \"fill\" parameter was found, SELF is used.";
		}



		if(latticeCard.equation.find_first_of(":") != std::string::npos) {
			mDebug() << "equation=======" << latticeCard.equation;
            throw std::invalid_argument(latticeCard.pos() + " Invalid lattice element description."
				   "(wrong number of surfaces, absence of fill or something)");
		}

		// Lattice要素定義式は必ずしも "1 -2 3 -4"のような単純な列記になっているとは限られず、
		// 1 -2 (3 -4) というように部分的にくくられてたり、面コンプリメントされていたりするので、
		// 一旦LogicalExpressionを経由してから再度文字列化する。
		//latticeCard.equation = utils::dequote(std::make_pair('(', ')'), latticeCard.equation);
		latticeCard.equation = utils::dequote(std::make_pair('(', ')'),
											  lg::LogicalExpression<std::string>::fromString(latticeCard.equation).toString());

		size_t numSurfs = utils::splitString('\"', " ", latticeCard.equation, true).size();
		if(numSurfs%2 != 0 && (numSurfs > 8 || numSurfs < 2)) {
            throw std::invalid_argument(latticeCard.pos() + " Number of surfaces in LAT card should be 2, 4, 6, or 8"
									   "(otherwise, dimension declarator or number of input univers is wrong)."
                                       "\nSurfaces equation = " + latticeCard.equation);
		}

		/*
		 * ここからLattice要素展開。
		 * latticeCard: LAT=N を含み、格子を定義しているカード
		 */
		auto selfUnivBaseName = (latticeCard.parameters.find("u") != latticeCard.parameters.end()) ?
								latticeCard.parameters.at("u") : "";
		std::string selfUnivName = getSelfUnivName(selfUnivBaseName, latticeCard.name);



		try{
			if(latvalue == 3) {
				mWarning(latticeCard.pos()) << "LAT = 3 support is experimental.";
				TetraCreator tetCreator(latticeCard);
				//connect(&tetCreator, &TetraCreator::fileOpenSuceeded, this, &CellCreator::fileOpenSucceeded);
				auto cardVec = tetCreator.createTetraCards(selfUnivName,
														   surfCreator_->transformationMatrixMap(),
														   surfCreator_->mapPointer());
				latticeElemCards.insert(latticeElemCards.end(), std::make_move_iterator(cardVec.begin()), std::make_move_iterator(cardVec.end()));
				isSelfFilled = true;  // 連続四面体の非四面体部分は必ず自己充填

			} else if(latvalue == 1 || latvalue == 2) {


				LatticeCreator latCreator(this, latvalue, latticeCard, solvedCards);


				auto cardVec = latCreator.createElementCards(selfUnivName,
															 surfCreator_->transformationMatrixMap(),
															 surfCreator_->mapPointer());



//				mDebug() << "latCreatorで生成されたカード";
//				for(const auto&c :cardVec) {
//					mDebug() << c.toInputString();
//				}

				latticeElemCards.insert(latticeElemCards.end(), std::make_move_iterator(cardVec.begin()), std::make_move_iterator(cardVec.end()));
				// TODO このへんのisSelfFilledフラグはきれいな形に整理したい。Lattice/teteraCreatorにまとめるとか
				isSelfFilled = isSelfFilled || latCreator.isSelfFilled();
				// end LAT=1,2
			} else {
                std::stringstream ss;
                ss <<latticeCard.pos() + " LAT parameter should be 1, 2, or 3. actual = " << latvalue;
                throw std::invalid_argument(ss.str());
			}
        } catch (std::exception &e) {
            throw std::invalid_argument(latticeCard.pos() + " While creating lattice element, "+ e.what());
		}




		// 自己参照あるいはfill省略している場合、 自己参照用univを定義するセルカードを追加する
		if(isSelfFilled) {
			inp::CellCard selfcard = latticeCard;
			auto selfUnivBaseName = (selfcard.parameters.find("u") != selfcard.parameters.end()) ? selfcard.parameters.at("u") :"";
			selfcard.parameters["u"] = getSelfUnivName(selfUnivBaseName, selfcard.name);
			//mDebug() << "生成するセルフuniv名=" << selfcard.parameters.at("u");

			selfcard.name += SELF_SUFFIX;  // univ定義セルカードはインスタンスが作られないので名前は一意なら適当でいい
			// fill, lat,パラメータはここでは解決済みなので削除
			if(selfcard.parameters.find("fill") != selfcard.parameters.end()) selfcard.parameters.erase(selfcard.parameters.find("fill"));
			if(selfcard.parameters.find("lat") != selfcard.parameters.end()) selfcard.parameters.erase(selfcard.parameters.find("lat"));
			// selfユニバースの場合論理多項式はlatticeセルカードのeqと同じなので重複しないように空にする
			selfcard.equation.clear();
			latticeElemCards.emplace_back(selfcard);
		}
		// lattice展開後はのセルカードは不要なので、展開済みLAT定義セルカードは削除する。
		// 要素充填時にはlatは展開済みという前提のもと処理を行うので必ず削除しなければならない。
		it = solvedCards->erase(it);

	}  // end solvedCard loop


	for(auto &elem: latticeElemCards) {
		//mDebug() << "new elemname=" << elem.toString();
		solvedCards->emplace(elem.name, elem);
	}
}

// cellcardsにfillした後のセルカードを格納する。
void geom::CellCreator::fillUniverse(int numThread, const CardMap &solvedCards, std::vector<inp::CellCard> *cellcards, bool verbose)
{
	// ########### Universe
	//mDebug() << "\nUniverse解決開始";


	std::unordered_map<std::string, std::set<inp::CellCard>> tmpUnivMap;// cardの重複を防ぐため一旦setで受けてからvector化する。
	//ここでuniverse-filled構造を作成する
	//まずuniv名をキーに,そのunivに所属するセルカード(複数)を値にしたユニバースマップを作成する
	// セルカードのu=の引数をキーに、カード自体をvalueにする。
	for(auto &cardPair: solvedCards) {
		inp::CellCard card = cardPair.second;
		if(card.parameters.find("u") != card.parameters.end()) {
			//mDebug() << "u=" << card.parameters.at("u") << ", cardname=" << card.name;
			//univMap.emplace(card.parameters.at("u"), card);
			// universe	がまだ未登録なら新規emplace
			tmpUnivMap[card.parameters.at("u")].insert(card);
		}
	}
	std::unordered_map<std::string, std::vector<inp::CellCard>> univMap;
	for(auto it = tmpUnivMap.cbegin(); it != tmpUnivMap.cend(); ++it) {
		univMap.emplace(it->first, std::vector<inp::CellCard>(it->second.begin(), it->second.end()));
	}
	//mDebug() << "universeマップ作成終了";


	/*
	 * fillしている先はgetFilledCards再帰的に解決されるので、
	 * ここでは一番外側のfillされているセルのみをgetFilledCardsにあたえること。
	 * 即ち
	 * ・u=が設定されているカードは外側セルではないのでここでは処理しない(他のu/fillチェーン内で再帰的に処理される)
	 */

//	mDebug() << "サーフェイス一覧";
////	mDebug() << surfCreator_->surfaceMap_.frontSurfaceNames();
//	for(auto &bs: surfCreator_->surfaceMap_.backSurfaces()) {
//		if(bs) mDebug() << bs->toString();
//	}


	// ここからFilling処理開始
	utils::SimpleTimer timer;
	timer.start();

	// ここのループをマルチスレッド化しても意味がない。
	// だいたい時間の掛かるfilling処理が発生するのはDicom2Phits出力みたいな一階層超多数セル
	// なのでトップレベルのfill処理単位で並列化しても無駄ほぼ無駄。
	//mDebug() << "Start cell filling,  num thread =" << numThread;
	for(auto it = solvedCards.begin(); it != solvedCards.end();++it) {
		inp::CellCard outerCellCard = it->second;
		// u=があるか、fill=,*fillのどちらもない無いカードはここでfill処理しない
		if(outerCellCard.parameters.find("u") != outerCellCard.parameters.end()) {
			cellcards->emplace_back(std::move(outerCellCard));
			continue;
		}
		auto fillIt = outerCellCard.parameters.find("fill");
		if (fillIt  == outerCellCard.parameters.end()) {
			fillIt = outerCellCard.parameters.find("*fill");
			if(fillIt == outerCellCard.parameters.end()) {
				cellcards->emplace_back(std::move(outerCellCard));
				continue;
			}
		}

		// outerCellCardは fill=univでフィルされているカード
		// filledCardsはfill処理が終わり、要素ごとに個別セル化されたカード

        if(verbose) mDebug() << "Cell =" << outerCellCard.toInputString() << "の充填セル作成開始";

		// 今の所バグがあるのでマルチスレッドfillingは無効化しているが、そのうちなんとかしたい。
		// → でもSurfaceMapへのregisterが原因なら並列化しても無駄。
		// 並列化は何か別の方式を考える必要がある。SurfaceMapはスレッドごとに作って
		// 最後にマージなど。

		// 生成されるカード数はunivMap.at(fill=の引数).size()で取得できる。
		// ここではfill時TRの処理はせず、単に要素数を知りたいだけなので
		// TR部分を除いた正味univ名を取得する。
		std::string fillingUnivName = fillIt->second;
		std::smatch sm;
        if(std::regex_search(fillingUnivName, sm, std::regex(R"((\w+) *\()"))) {
			fillingUnivName = sm.str(1);
		}
        if(univMap.find(fillingUnivName) == univMap.end()) {
            std::stringstream ss;
            ss << it->second.pos();
            ss << " Filled universe (" << fillingUnivName << ") not found in the univ map ={";
            for(const auto &p: univMap) ss << p.first << " ";
            ss <<"}";
            throw std::runtime_error(ss.str());
		}
		int numElems = static_cast<int>(univMap.at(fillingUnivName).size());
		// このouterCardはfill=univ番号で充填される。
		// このuniv番号を構成するセルカードのvectorがunivMap.at(univ番号)
		// 従ってfill充填を行うとouterCard1枚あたりunivMap.at(univ番号).size()個のセルカードが発生する。
		//mDebug()  << "numElems ===" << numElems;
		std::atomic_int counter(0);

		/*
		 * 並列化は色々むずかしい。
		 * 1．Surfaceの並列生成：今はuniqueなIDを与えているのでID発行をスレッドセーフにする必要がある。
		 * 2. SurfaceMapへの登録は当然スレッドセーフにする必要があるが、ここでmutexすると多分
		 *    そこがボトルネックになって並列化効率が上がらない。
		 */
        if(numThread > 1 && verbose) mWarning() << "Multi threading in cell filling is currently disabled for bugs.";
		std::vector<inp::CellCard> filledCards;
		std::exception_ptr ep;
		std::atomic_bool cancelFlag(false);
		std::thread th( [&filledCards, &ep, numElems, this](const std::unordered_map<std::string, std::vector<inp::CellCard>> &univmap,
											 const inp::CellCard &outerCard, std::atomic_bool *cancel, std::atomic_int *ctr)
		{
			try {

				// TODO Latticeが入れ子Latticeの場合、ctrはnumElems異常に加算されてしまう。
				//       弊害はスレッドのjoinで期待したより長く待たされることだけなので、あとで修正。
                // 再帰呼び出し深さ情報を与えて対処する。
                //mDebug() << "outerCard===" << outerCard.toString();
				filledCards = inp::CellCard::getFilledCards(this, univmap, outerCard, 1, 0, cancel, ctr);
			} catch (...) {
				ep = std::current_exception();
				ctr->store(numElems);
			}
		}, univMap, outerCellCard, &cancelFlag, &counter);

#ifdef ENABLE_GUI
		// progress_utils関数を使いたいが、計算開始時に計算対象の総数がわかっていないケースには適用できない。
		static const std::array<QString, 5> dots{".... ", " ....", ". ...", ".. ..", ".... "};
        QString message = QString("Cell:") + QString::fromStdString(outerCellCard.name + " ") + QObject::tr("Filling with universe cells...");
        QProgressDialog progress(message + dots.at(0), QObject::tr("Cancel"), 0, numElems, nullptr);
		progress.setMinimumDuration(1000);

		size_t loopCounter = 0;
		while(counter < numElems) {
			progress.setValue(counter);
			progress.setLabelText(message + dots.at(++loopCounter%dots.size()) + " "
								  + QString::fromStdString(std::to_string(counter) + "/" + std::to_string(numElems)));
			QApplication::processEvents();
			if(progress.wasCanceled()) {
				cancelFlag.store(true);
				break;
			}
			// バックグラウンドのレスポンスを生かすためにsleepは短めかで。
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
		progress.close();
		th.join();
		if(cancelFlag.load()) {
			std::stringstream ss;
			ss << "Filling operation was canceled. cell = " << outerCellCard.name;
			throw std::runtime_error(ss.str());
		}
		if(ep) std::rethrow_exception(ep);
#else
		th.join();
#endif

//		mDebug() << "セルカード=" << outerCellCard.name << "をfillした結果";
//		for(auto cd: filledCards) mDebug() << "card===" << cd.toInputString();

		cellcards->insert(cellcards->begin(),
						  std::make_move_iterator(filledCards.begin()),
						  std::make_move_iterator(filledCards.end()));

	}
	timer.stop();
    if(verbose) mDebug() << "Filling done in " << timer.msec() << "(ms)";
//	mDebug() << "STOP for DEBUGGING";
//	std::exit(EXIT_SUCCESS);
}



void geom::CellCreator::initUndefinedCell(const geom::Surface::map_type &surfMap)
{
	Cell::initUndefinedCell(surfMap);
}


std::ostream &geom::operator <<(std::ostream &os, const geom::CellCreator &cellObjs)
{
	// elemはpair<string, shared_ptr<Cell>>
	os << "CellMap:";
	for(auto& elem: cellObjs.cells()) {
		os << std::endl;
		os << elem.second->toString();
	}
	return os;
}

