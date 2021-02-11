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
#include "tetracreator.hpp"

#include <algorithm>
#include <array>

#include <cmath>
#include <fstream>
#include <memory>
#include <unordered_set>

#include "tetrahedron.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/geometry/cell/bb_utils.hpp"
#include "core/geometry/surface/surfacemap.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/io/input/cellcard.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/system_utils.hpp"
#include "core/utils/matrix_utils.hpp"
#include "core/formula/logical/lpolynomial.hpp"

namespace {

const char COMMENT_CHAR = '#';
const char NODEFILE_REFSITE[] = "http://wias-berlin.de/software/tetgen/1.5/doc/manual/manual006.html#ff_node";
const char ELEMFILE_REFSITE[] = "http://wias-berlin.de/software/tetgen/1.5/doc/manual/manual006.html#ff_ele";
const char PHITS_REFDOC[] = "https://phits.jaea.go.jp/manual/manualE-phits.pdf";


std::string ValidTetraElementName(const std::string baseFileName,
                                  int numDigits,
                                  const std::string &elemID,
                                  const std::string & latticeCardName) {
    return baseFileName + "_"
        + utils::paddedString('0', numDigits, elemID) + "<" + latticeCardName;
}
// elementsを取り巻く周辺セルの命名は要素名と少しだけ違う。
std::string ValidTetraSurroundingName(const std::string baseFileName,
                                        int numDigits,
                                        const std::string & latticeCardName) {
    return baseFileName + "_-"
        + utils::paddedString('0', numDigits-1, std::string("0")) + "<" +latticeCardName;
}


}  // end anonymous namespace

/*
 * 改行コードについて
 * Windows式CRLF改行ファイルをLinuxのようなLF改行システムでgetlineすると
 * 読み取った文字列の末尾にCRが残る。
 * 故にstring末尾の\rを削除する必要がある。
 *
 * ややこしいのがファイルの最後に改行が無い場合、
 * 最後のgetlineではstringの末尾に\rが入らない。
 * （改行自体が存在しないのでCRの混入も無い）
 *
 * さてifstreamのeofとgetlineのtrue/falseの関係は、
 *
 * ・最後に改行のないテキストの最終行でgetlineした場合
 *		getline→true, eof()→true
 * ・最後に改行のあるテキストの最終行でgetlineした場合
 *		getline→true, eof()→false
 *
 * よってeof==trueなら最後に改行が無いと言えるのでこれを基準に
 * \rを削除するしないを判別できる。
 * だが多分string::back()のほうがifstream::eof()より低コストな気がするのでそちらで対処する。
 *
 *
 * ファイル読み取りの戦略は下記の通り
 *
 * 1．getline==falseになるまでファイルを読む
 * 2．while(getline){}を抜けた時点で必要数に達していない場合はUnexpected EOF
 * 3．while(getline){}を抜けた時点でifstream::eof()==trueなら改行がないことを警告
 * これでwhileループ内でeofチェックする必要がなくなる。
 */



// TODO nodeファイルとかもinputViewerで開けるようにしたい。

// 今は入力読み込みごとにカレントディレクトリを変更しているのでtfile の指定は絶対パスでも相対パスでもよい。
// ↑と考えていたが、コマンドラインから別フォルダのニュ力ファイルを読んだ場合、カレントディレクトリと
// ニュ力ファイルのディレクトリが一致しないので tfileを読み込む起点となる入力ファイルのパスを与える必要がある。
// コンストラクタではとりあえずファイルのデータを読み込むに留める
geom::TetraCreator::TetraCreator(const inp::CellCard &latticeCard)
	: tsfac_(1.0), latticeCellCard_(latticeCard), maxElemId_(0), checkOuterCell_(true)
{
	// 1. パラメータ(tsfac, tfile, universe)の取得
	if(latticeCellCard_.parameters.find("tsfac") != latticeCellCard_.parameters.end()){
		tsfac_ = utils::stringTo<double>(latticeCellCard_.parameters.at("tsfac"));
	}
	if(latticeCellCard_.parameters.find("tfile") != latticeCellCard_.parameters.end()) {
		baseName_ = latticeCellCard_.parameters.at("tfile");
	} else {
        throw std::invalid_argument(latticeCellCard_.pos() + " tfile parameter is missing.");
	}
	if(latticeCellCard_.parameters.find("u") != latticeCellCard_.parameters.end()) {
		auto univStr = latticeCellCard_.parameters.at("u");
		if(univStr.front() == '-') {
			mWarning(latticeCellCard_.pos()) << "Negative universe disable outer cell checking.";
			universe_ = univStr.substr(1);
		} else {
			universe_ = univStr;
		}
	} else {
        throw std::invalid_argument(latticeCellCard_.pos() + " universe is missing in lattice cell card.");
	}

	/*
	 * latticeCard.fileから絶対パスあるいはカレントディレクトリの相対パスが得られるので
	 * tfileのパス指定にはそれを使えう。その結果は実際にlatticeカードを記述しているファイル
	 * からの相対パス指定となる。
	 */
	// accessiblePathは絶対かカレントディレクトリからの相対パス。
	// どちらかはわからないが入力ファイルのパスであることは間違いない。
	std::string nodeFile = baseName_ + ".node";
	std::string elementFile = baseName_ + ".ele";

	if(utils::isRelativePath(baseName_)) {
		nodeFile = utils::separatePath(latticeCard.file).first + nodeFile;
		elementFile = utils::separatePath(latticeCard.file).first + elementFile;
	}

	std::ifstream nodeIfs(utils::utf8ToSystemEncoding(nodeFile).c_str());
	if(nodeIfs.fail()) {
        std::stringstream sse;
        sse << latticeCellCard_.pos() << " No such a node file = " << nodeFile;
        throw std::runtime_error(sse.str());
	}

	std::ifstream elementIfs(utils::utf8ToSystemEncoding(elementFile).c_str());
	if(elementIfs.fail()) {
        std::stringstream sse;
        sse << latticeCellCard_.pos() << " No such a element file = " << elementFile;
        throw std::runtime_error(sse.str());
	}


	// 2．ノードファイル読み込み
	std::string buff;
	size_t line = 0;


	while(getline(nodeIfs, buff)) {
		++line;
		utils::sanitizeCR(&buff);
		utils::trim(&buff);
		utils::removeInlineComment(COMMENT_CHAR, &buff);
		if(!buff.empty()) break;
	}
	if(buff.empty() || nodeIfs.eof()) {
        throw std::invalid_argument(latticeCellCard_.pos()
                                    + " Unexpected EOF while reading header of the node file ="
                                    + nodeFile);
	}
	const std::vector<std::string> nodeHeaderParams = utils::splitString(" ", buff, true);
	const std::string nodeHeaderPos = nodeFile + ":" + std::to_string(line);
	// ヘッダチェック
	if(nodeHeaderParams.size() < 3) {
        throw std::invalid_argument(nodeHeaderPos + " Invalid node file header found. Too few parameters(should be >3)," +
               "header =" + buff + "included from" + latticeCellCard_.pos() + " See " + NODEFILE_REFSITE);
	} else if(utils::stringTo<int>(nodeHeaderParams.at(1)) != 3) {
        throw std::invalid_argument(nodeHeaderPos + " Invalid node file header found. Only 3D is supported, actual="
               + nodeHeaderParams.at(1) + "included from" + latticeCellCard_.pos() + "See " + NODEFILE_REFSITE);
	}
	const size_t numNodes = utils::stringTo<size_t>(nodeHeaderParams.at(0));
	while(getline(nodeIfs, buff)) {
		++line;
		utils::sanitizeCR(&buff);
		utils::trim(&buff);
		utils::removeInlineComment(COMMENT_CHAR, &buff);
		if(buff.empty()) continue;  // 空行/コメント行はcontinue

		auto params = utils::splitString(" ", buff, true);
		if(params.size() != 4) {
			std::string errorPos = nodeFile + ":" + std::to_string(line);
            throw std::invalid_argument(errorPos + " Number of data should be 4, actual =" + utils::toString(params.size())
                    + "data =" + buff + "included from" + latticeCellCard_.pos());
		}

		// ここまで来てやっと節点データ
		math::Point pt{tsfac_*utils::stringTo<double>(params.at(1)),
					   tsfac_*utils::stringTo<double>(params.at(2)),
					   tsfac_*utils::stringTo<double>(params.at(3))};
		//if(matrix) math::affineTransform(&pt, *(matrix.get()));

		nodes_.emplace(std::make_pair(utils::stringTo<size_t>(params.at(0)), std::move(pt)));
	}
	// CRなしでEOFになったらnewlineなしだが検知する意味はあまりない。

	if(nodes_.size() < numNodes) {
        throw std::invalid_argument(latticeCellCard_.pos() + " Unexpected EOF while reading the node file =" + nodeFile);
	} else 	if(nodes_.size() > numNodes) {
		mWarning(nodeHeaderPos) << "Number of nodes described in header(" << numNodes << ") and"
							<< " existed in file(" << nodes_.size() << ") are different.";
	}



	// 3．四面体要素ファイル(elementファイル)読み込み
	line = 0;
	while(getline(elementIfs, buff)) {
		++line;
		utils::sanitizeCR(&buff);
		utils::trim(&buff);
		utils::removeInlineComment(COMMENT_CHAR, &buff);
	if(!buff.empty()) break;
	}
	if(elementIfs.eof()) {
        throw std::invalid_argument(latticeCellCard_.pos()
                                    + " Unexpected EOF while reading header of the element file =" + elementFile);
	}

	auto elementHeaderParams = utils::splitString(" ", buff, true);
	std::string elemHeaderErrorPos = elementFile + ":" + std::to_string(line);
	if(elementHeaderParams.size() != 3) {
        throw std::invalid_argument(elemHeaderErrorPos+ " Number of input in element file header should be 3, actual ="
               + std::to_string(elementHeaderParams.size()) + "See " + ELEMFILE_REFSITE);
	}

	const int numVertexForAElement = utils::stringTo<int>(elementHeaderParams.at(1));
	const int numAdditionalInfo = utils::stringTo<int>(elementHeaderParams.at(2));
	if(numVertexForAElement != 4) {
        throw std::invalid_argument(elemHeaderErrorPos + " Only 1st order of tetrahedra is supported."
               + "See " + PHITS_REFDOC + " and " + ELEMFILE_REFSITE);
	} else if(numAdditionalInfo != 1) {
		mWarning(elemHeaderErrorPos) << "Phits requires only 1 addiotnal parameter"
									 << " in the element file, actual =" << numAdditionalInfo;
	}

	// ヘッダチェックが終わったので要素を読みに行く
	size_t numElements = utils::stringTo<size_t>(elementHeaderParams.at(0));
	while(getline(elementIfs, buff)) {
		++line;
		utils::sanitizeCR(&buff);
		utils::trim(&buff);
		utils::removeInlineComment(COMMENT_CHAR, &buff);
		if(buff.empty()) continue;

		// チェック
		auto elementParams = utils::splitString(" ", buff, true);
		std::string elemErrorPos = elementFile + ":" + std::to_string(line);
		size_t numParams = elementParams.size();
		if(numParams < 6) {
            throw std::invalid_argument(elemErrorPos
                                        + "Number of parameter for a element should be greater equal than 6 , actual ="
                                        + std::to_string(elementParams.size()));
		} else if(numParams > 6) {
			mWarning() << "Phits requires only 1 addional parameter."
                        << numParams - 1 - static_cast<size_t>(numVertexForAElement) << "are added.";
		}

		//size_t id = utils::stringTo<size_t>(elementParams.at(0));
		size_t p1 = utils::stringTo<size_t>(elementParams.at(1));
		size_t p2 = utils::stringTo<size_t>(elementParams.at(2));
		size_t p3 = utils::stringTo<size_t>(elementParams.at(3));
		size_t p4 = utils::stringTo<size_t>(elementParams.at(4));
		// ここからやっとelement生成

		size_t idNum = std::stoul(elementParams.at(0));
		if(maxElemId_ < idNum) maxElemId_ = idNum;  // 後で桁数を確定させるために最も大きい要素IDを記録しておく。

		elements_.emplace(std::make_pair(elementParams.at(0),
										 std::make_tuple(line, std::array<size_t, 4>{p1, p2, p3, p4},	elementParams.at(5))));
		// element数過剰は意図せざる体系になり得るので必要数getしたらbreakする
		if(elements_.size() == numElements) break;
	}




	if(numElements > elements_.size()) {
        throw std::invalid_argument(elementFile + ":" + std::to_string(line) + " Unexpected EOF while reading elements."
               + " Number of elements, read =" + std::to_string(elements_.size()) + "required =" + std::to_string(numElements));
	} else  {
		while(getline(elementIfs, buff)) {
			++line;
			utils::sanitizeCR(&buff);
			utils::trim(&buff);
			utils::removeInlineComment(COMMENT_CHAR, &buff);
			if(buff.empty()) continue;
			mWarning(elementFile + ":" + std::to_string(line)) << "Unnecessary extra data exists.";
		}

	}
}

/*
 * 連続四面体Latticeカードの明示TRCLはどのように処理すべきか？
 * ・そもそもLatticeカードのセル定義部分(論理多項式部分)は4面体は利用せず、4面体周辺領域のみが利用する。
 * → 4面体周辺領域へはLattice要素生成前に、既存のTRCLと同様に適用済みになっている。
 * なので四面体要素に適用するだけでOK。接点読み込み時に適用してしまうべし。
 * 但し、四面体内部充填Universeには適用する必要があるのでTRCL文字列自体は必要となる。
 *
 *
 */



std::vector<inp::CellCard> geom::TetraCreator::createTetraCards(const std::string &selfUnivName,
																const std::unordered_map<size_t, math::Matrix<4>> &trMap,
																geom::SurfaceMap *smap)
{

	std::vector<inp::CellCard>  elemCards;
	/*
	 * 面を3点から生成する。名前の一意性を確保するため、
	 * 3点p1p2p3のうち一番array中のindexが小さいものを一番左にする。つまり
	 * (p1, p2, p3), (p2, p3, p1), (p3, p1, p2)で生成する面は全て等価であるが、名前は"p1p2p3"とする
	 * また、"-p1p2p3"面(p1p2p3の裏面)と "p1p3p2"面も等価となるので重複生成しないように気をつける
	 *
	 * surface名を一意にするため名前は
     * (baseName_のファイル名のみ抜き出したもの)_Tp1p2p3とする。
	 */

    int digits = static_cast<int>(std::log10(maxElemId_+1) + 1);

	std::string surroundingEquation; // フィルする周囲の式、四面体要素の式、四面体を囲むbox内部の式
	const std::string outerEquation = checkOuterCell_ ? latticeCellCard_.equation : "";

	//bool doReduceSurroundingSurface = true;  // 四面体を凸クラスタに分けて周辺セルの定義式をシンプルにする。フラグ


	/*
	 * FILL時にTRが適用されるからここでは節点はTRしなくて良いと思ったが、
	 * 節点自体にはTRは適用されないのでここでaffine変換しておく必要がある。
	 *
	 */
	std::unique_ptr<math::Matrix<4>> matrix, invMatrix;
	if(!latticeCellCard_.trcl.empty()) {
		matrix.reset(new math::Matrix<4>(utils::generateTransformMatrix(trMap, latticeCellCard_.trcl)));
		mDebug() << "TR matrix=" << *matrix.get();
		invMatrix.reset(new math::Matrix<4>(matrix->inverse()));
	}

	// ここでnodes_には一括してTransformを適用する。
	if(matrix) {
		for(auto &p: nodes_) {
			math::affineTransform(&p.second, *matrix.get());
		}
	}
    const std::string baseName = utils::getFileName(baseName_);
	std::unordered_set<std::string> createdSet;
	TetrahedronFactory tetrafactory;
	std::vector<math::Point> bbPoints(4);
	for(auto &elem: elements_) {
        // elem はfirstがelementID, secondがelementファイル行番号、位相情報array, 要素充填univ名のタプル
		//std::string elementName = baseName_ + "_" + utils::paddedString('0', digits, elem.first) + "<" + outerCellCard_.name;
		const std::string &univ = std::get<2>(elem.second);
		const std::array<size_t, 4> &ph = std::get<1>(elem.second);  // 位相情報＝4点のインデックス




        auto tetra = std::make_shared<Tetrahedron>(baseName, ph, nodes_, &createdSet, smap);

        tetra->setName(ValidTetraElementName(baseName, digits, elem.first, latticeCellCard_.name));
//        tetra->setName(baseName_ + "_" + utils::paddedString('0', digits, elem.first) + "<" + latticeCellCard_.name);
		tetra->setParameter(std::make_pair("fill", univ));
		tetra->setParameter(std::make_pair("u", universe_));

		// BBのTRCLはセル構築直前に一括適用なのでここでは適用しない。
		// 逆にnodes_に掛かっているTRの逆変換を掛けてTRをキャンセルする
		for(size_t i = 0; i < ph.size(); ++i) {
			bbPoints[i] = nodes_.at(ph[i]);
			if(invMatrix) math::affineTransform(&bbPoints[i], *invMatrix.get());
		}
		std::string bbStr = geom::BoundingBox::fromPoints(bbPoints).toInputString();
		tetra->setParameter(std::make_pair("bb", bbStr));






//		if(doReduceSurroundingSurface) {
			tetrafactory.append(tetra);
//		} else {
//			inp::CellCard elementCard();
//			// 周辺セルの面削減をしない場合はここで四面体要素を追加すれば良い。
//			elemCards.emplace_back(baseName_ + ".ele", 0, // 生成元ファイルと行番号
//								   tetra->name(), "0", 0.0,  // セル名、Material名と密度
//								   outerEquation + " " + tetra->equation(),  // 論理多項式
//								   "", tetra->parameters(),     // パラメータ
//								   std::vector<std::string>(), latticeCellCard_.trcl); // complimentで依存しているセルとtrcl
//			// trcl文字列は各四面体共通になるので個別にもたせたりしない。四面体充填univにはtrclを適用する必要があるためelemCardには追加している。
//			surroundingEquation += tetra->surroundingEquation() + " ";
//		}

	}  // 要素ループ終わり



	// 周辺セルの面削減をする場合はここ
	/*
	 * 周辺セルの定義を短くしたい。
	 * 1．連続四面体は全てが1つの塊ではないのでまずは隣接している塊ごとに分ける
	 * 2．塊ごとが凸多面体になるように再分割する
	 * 3．凸多面体の外側面のみをORでつなぎ、多面体間はANDでつなぐ
	 *
	 * 但し、3D表示についてはポリゴン構築時間が支配的なので多少陰関数の速度が上がってもあまり差はない。
	 */
//	if(doReduceSurroundingSurface) {
		TetrahedronFactory::setUniqueID(*smap, std::numeric_limits<int>::max(), tetrafactory.tetraMap());
		tetrafactory.generateConvexTetrahedra();
		for(size_t i = 0; i < tetrafactory.tetrahedras().size(); ++i) {
			for(auto &tetra: tetrafactory.tetrahedras().at(i).elements()) {
				assert(tetra);
                tetra->setParameter(std::make_pair("u", universe_));
                elemCards.emplace_back(baseName + ".ele", 0, // 生成元ファイルと行番号
									   tetra->name(), "0", 0.0,  // セル名、Material名と密度
									   outerEquation + " " + tetra->equation(),  // 論理多項式
									   "", tetra->parameters(),     // like-but依存先とパラメータ
									   std::vector<std::string>(), latticeCellCard_.trcl); // complimentで依存しているセルとTRCL
			}
		}

		surroundingEquation = tetrafactory.surroundingEquation();
//	} else {
//			surroundingEquation.pop_back();
//	}

    // ここから要素を取り巻く周辺セルの追加



    inp::CellCard::map_type paramMap{{"fill", selfUnivName}, {"u", universe_}};
	auto poly = lg::LogicalExpression<int>::fromString(latticeCellCard_.equation, smap->nameIndexMap());
    auto boundingPlaneVectors = geom::bb::boundingSurfaces(poly, nullptr, *smap);
	auto tmpBB = BoundingBox::fromPlanes(nullptr, boundingPlaneVectors);

	if(!tmpBB.empty()) {
		//mDebug() << "周辺セルのtmpBBは=" << tmpBB.toInputString();
		paramMap.emplace("bb", tmpBB.toInputString());
	}
    const std::string cellName = ValidTetraSurroundingName(baseName, digits, latticeCellCard_.name);
	elemCards.emplace_back(cellName,
						   latticeCellCard_.materialName, latticeCellCard_.density, // Material名と密度
						   latticeCellCard_.equation + " (" + surroundingEquation + ")",  // 論理多項式
						   "",         // like-butで依存している先のセル名
						   paramMap,     // パラメータ
						   std::vector<std::string>(), // complimentで依存しているセル
						   latticeCellCard_.trcl);

    //mDebug() << "latticeCellCard eq ===" << latticeCellCard_.equation;
    //mDebug() << "周辺セルの式=" << elemCards.back().equation;
	return elemCards;
}
