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
#include "latticecreator.hpp"

#include <memory>

#include "core/geometry/surface/plane.hpp"
#include "core/geometry/surface/surfacemap.hpp"
#include "core/geometry/cell/boundingbox.hpp"
#include "core/geometry/cellcreator.hpp"
#include "core/io/input/cardcommon.hpp"
#include "core/math/nvector.hpp"
#include "core/utils/message.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/time_utils.hpp"
#include "core/utils/matrix_utils.hpp"


// DimensionDeclaratorに記載されているi, j, k方向のレンジから格子定義用surfaceを生成する。
// planesが平行かどうかはチェック済みとしてここではチェックしない。



geom::LatticeCreator::LatticeCreator(const CellCreator *creator, int latvalue, const inp::CellCard &latticeCard,
									 const std::unordered_map<std::string, inp::CellCard> *solvedCards)
	:latticeCard_(latticeCard), isSelfFilled_(false), latticeArg_(latvalue)
{
	// latticeEquationの展開はここより前で済ましておくこと。 要素定義面数チェックも。故にここではassertに留める
	assert(latticeCard_.equation.find_first_of("()") == std::string::npos);

	//std::string latticeSurfEquation = utils::dequote(std::make_pair('(', ')'), latticeCard_.equation);
	std::vector<std::string> latticeUnitSurfs = utils::splitString('\"', " ", latticeCard_.equation, true);
	for(auto &arg: latticeUnitSurfs) 	arg = utils::dequote('\"', arg);
	assert(latticeUnitSurfs.size()%2 == 0 || latticeUnitSurfs.size() < 8);
	assert(latticeCard_.parameters.find("fill") != latticeCard_.parameters.end());

	fillingData_ = inp::FillingData::fromString("fill=" + latticeCard_.parameters.at("fill"));
	fillingData_.init(creator, solvedCards, latticeCard_, latticeUnitSurfs, latticeArg_, &isSelfFilled_);
	//mDebug() << "\nFILLDATA=" << fillData.toInputString();
	//mDebug() << "BaseElement center===" << fillingData_.baseUnitElement()->center();
}

/*
 * 結局の所lat=1と2で面の構成方法は異なってしまう。具体的には
 * LAT=1の場合セルは格子状に位置するので、必要となるsurface数はそれぞれのインデックスの方向の数だけであるが、
 * LAT=2の場合、隣接するセルとしか面は共有できないので要素の数(÷2)だけ面が必要となる
 *              正六角形の場合のみ隣接しないセルと面を共有し得るが基本要素セルから遠い所では
 *              数値誤差により未定義領域が出来てしまう可能性があるので、正六角形だからといって
 *              特別な取扱はしない。
 */
std::vector<inp::CellCard> geom::LatticeCreator::createElementCards(const std::string &selfUnivName,
																	const std::unordered_map<size_t, math::Matrix<4>> &trMap,
																	SurfaceMap *smap)
{

	/*
	 * 六角形格子の場合、隣り合う要素でしか面を共有できないので、要素ごとに個別に面を作成する必要がある。
	 * 但し正方形格子や正六角形格子でそれをやった場合、同一平面としてSurfaceCreatorあたりで集約されて無駄に計算時間がかかる。
	 */
	// 面をsmapに追加

	generatePlanes(fillingData_.irange(), fillingData_.jrange(), fillingData_.krange(), smap);


//	for(const auto& planePair:  fillingData_.baseUnitElement()->planePairs()) {
//		mDebug() << "単位+面 ===" << planePair.first.toInputString() << "単位-面 ===" << planePair.second.toInputString();
//	}


	/*
	 *  ここでの単位プラス/マイナス面の扱いと、elementの論理式との扱いで異なる点があるので注意。
	 *
	 * element論理式ではsindex方向に関しては -s1s2_@1 s1s2_@-1とするので
	 * s1s2_@1とs1s2_@-1の法線ベクトルは同じ方向を向いている必要がある。
	 * s1s2_@1とs1s2_@-1についても同様。
	 *
	 * しかしながらs1とs2の定義によってはこれが満たせていない。
	 * 具体的にはsindexがxのマイナス方向を向いている場合など。
	 * 例えばlatticeカードが
	 * L1 0 -s1 s2 lat=1 fill=0:0 0:0 0:0 1
	 * s1 PX 5
	 * s2 PX -5
	 * ならば単位プラス面は normal=(1, 0, 0), dist=5
	 * ならば単位マイナス面は normal=(1, 0, 0), dist=-5
	 * とならなければいけない。
	 *
	 * つまり生成面の法線ベクトルはインデックスの正方向を向くように変更する必要がある。
	 */


	// [0 0 0]のBoundingBoxを作成する
	BoundingBox unitBB =  fillingData_.baseUnitElement()->unitBB();


	// 明示的TRCLは既に適用済みとなっており、unitPlanesも既に明示TRCLは適用されている。
	// 故にセル構築時に一括TRCL適用するとBBには二回適用されてしまう
	// 気持ち悪いがここでは逆方向にtransformする。
	if(latticeCard_.hasTrcl()) {
		// latticeカード明示的TRCLの逆変換
		unitBB.transform(utils::generateTransformMatrix(latticeCard_.trcl).inverse());
	}
	// 外側セルのBBでカットしたい所だがこれはFILL時にしか確定しない。

		// ############# lattice要素セルの作成開始
	int numElement = 0;

	auto indexVectors = fillingData_.baseUnitElement()->indexVectors();
	auto irange = fillingData_.irange(), jrange = fillingData_.jrange(), krange = fillingData_.krange();
	std::vector<inp::CellCard> latticeElemCards;


// TODO  この3重ループはijkが大きいと遅い。  1．プログレス出す 2．並列化
// 64*64*63 ＝2.6E5 個要素で1.8秒くらいなので、正直並列化の必要を感じない速度。あとまわし。
// i,j,kどのレベルで並列化するか、というのも問題。大体はi,j,での要素数は多く、kでの要素数は少ない

	// 充填要素セルのセルカード(elementCard)の共通する部分の雛形を作成する。
	inp::CellCard elementCardTemplate = latticeCard_;
	// ※！latticeセルカードの明示的TRCLもlattice要素内部充填セルに適用されるのでこれは削除しない。
	// fill, lat,パラメータは適用済みなので充填要素セルカードからは削除
	elementCardTemplate.parameters.erase(elementCardTemplate.parameters.find("fill"));
	elementCardTemplate.parameters.erase(elementCardTemplate.parameters.find("lat"));

	mDebug() << "Start creating lattice-element cellcards";
	utils::SimpleTimer timer;
	timer.start();
	for(int k = krange.first; k <= krange.second; ++k) {
		for(int j = jrange.first; j <= jrange.second; ++j) {
			for(int i = irange.first; i <= irange.second; ++i) {
				//mDebug() << "creating element [i,j,k] =[" << i << j << k << "]";


				inp::CellCard elementCard = elementCardTemplate;
				// 要素インデックスからlattice要素セル名を作成
				elementCard.name = inp::indexToElementName(latticeCard_.name, i, j, k);
				elementCard.equation = 	utils::concat(this->getLatticeSurfaceNames(i, j, k), " ");
				//mDebug() << "elemcard.equation===" << elementCard.equation;


				// 要素のboundingBox
				auto elementBB = unitBB;

				std::string univStr = fillingData_.universes().at(numElement);
				// FILL時TR(univ番号にTR文字列がくっついている場合)は暗黙並進移動よりも先に適用される。
				std::smatch univMatch;
				static const std::regex fillTimeTrRegex(R"((\w+) *\((\**[-+ .0-9a-zA-Z]+)\))");
				if(std::regex_search(univStr, univMatch, fillTimeTrRegex)) {
					auto matrix = utils::generateTransformMatrix(trMap, univMatch.str(2));
					elementCard.addTrcl(utils::toTrclString(matrix), true);
					univStr = univMatch.str(1);

					// Lattice要素定義時TRCLは要素の位置ではなく、要素の中にのみ適用なのでbbには適用してはならない
					// が、セル生成時には区別できないので格子要素BBには逆行列を適用しておく。美しくないがとりあえず。
					elementBB.transform(matrix.inverse());  // 格子要素暗黙TRの逆変換
				}

				/*
				 * 要素内のuniverseに適用するTRCL文字列を作成する。
				 * 格子要素セル内のuniverseには格子位置中心への並進移動TRを適用する必要がある。
				 * このtransformは暗黙にlattice要素内部のunivに適用される。
				 */
				// 要素セルの内部ユニバースに適用するTRベクトル
				// 2次元格子の場合要素中心を決めるのに、3ペア目は使われないので最初の2個の並進ベクトルを使用すれば良い。
				math::Vector<3> implicitTrVec = i*indexVectors.at(0) + j*indexVectors.at(1);
				// 3次元の場合z方向のdisplacementがdispVecの3番めが4番目どちらに来るか判定が必要
				if(fillingData_.dimension() == 3) implicitTrVec += (fillingData_.numIndex() == 3) ? k*indexVectors.at(2) : k*indexVectors.at(3);
				std::string implicitTrclStr = std::to_string(implicitTrVec.x()) + " "
						+ std::to_string(implicitTrVec.y()) + " "
						+ std::to_string(implicitTrVec.z());
				elementCard.addTrcl(implicitTrclStr, true);


				// TRCLはFILL後に適用するのでここでは単位BBをセットする
				// でもそうするとFILL時に位置が確定しないので外側BBでのカットができなくなる。
				elementCard.parameters["bb"] = elementBB.toInputString();


				/*
				 * FILLについて暗黙のFILLと自己充填FILLの処理
				 *
				 * ●lattice定義時に自分のunivでfillすることが認められている。
				 * 例えば
				 *   2 2 -1.0  -11 12 -13 14 lat=1 u=1  fill=0:1 0:0 0:0  1 2
				 * これを今の展開スキームで展開すると
				 * lattice要素は
				 *		2[1 0 0] -11_1 11 -13 14 fill=2 trcl(20 0 0) u=1
				 *		2[0 0 0] -11   12 -13 14 fill=1 trcl( 0 0 0) u=1
				 * となり、2[0 0 0]のfill/univ解決時に循環参照となってしまう。
				 * よって自己参照のfill先は_selfを付けて区別するようにする。
				 *
				 * lattice要素は
				 *		2[0 0 0] -11   12 -13 14 fill=1_self trcl( 0 0 0) u=1
				 * と展開して、自己参照用のuniverse定義を追加する。
				 *		2_self 2 -1.0  -11 12 -13 14 u=1_self
				 *  問題はこのu=1の定義に他のセルカードも使われている場合があるか？
				 * → 無い。なぜならLATカードに入力できる面は角柱で4/6個で平行6面体を敷き詰める
				 * ようになっている。そのため他のセル定義を同じuniverse内に重複定義なし
				 * で定義することは不可能
				 *
				 * ●またfillパラメータを省略した場合自分自身で充填を行う。
				 *   fill省略時の対応はLAT=で分岐する前に処理してしまっている。
				 */
				// fillするuniv名とカードのuパラメータが一致する場合は自己充填
				if(elementCard.parameters.find("u") != elementCard.parameters.end() && univStr == elementCard.parameters.at("u")) {
					isSelfFilled_ = true;
					elementCard.parameters["fill"] = selfUnivName;
					//							mDebug() << "自己充填発見 universe名=" << univStr << "card.parameters.at(u)" << card.parameters.at("u");
					//							mDebug() << "fill引数のセルフuniv名=" << card.parameters.at("fill");
				} else {
					elementCard.parameters["fill"] = univStr;  // 自己参照していない場合はそのままにuniv番号を設定する
				}
				//mDebug() << "格子要素セル= " << elementCard.toString();
				++numElement;
				latticeElemCards.emplace_back(std::move(elementCard));
			}  // end i loop
		}  // end j loop
		// DEBUG
//		mDebug() << "EXIT for debug";
//		std::exit(EXIT_SUCCESS);

	}  // end k loop
	timer.stop();
	mDebug() << "End creating lattice-element cellcards in" << timer.msec() << "(ms)";
	//mDebug() << "格子要素セル作成終了";

	return latticeElemCards;
}






void appendSquareLatticeSurfaces(const inp::FillingData &fillingData,
								 const std::pair<int, int> &irange,
								 const std::pair<int, int> &jrange,
								 const std::pair<int, int> &krange,
								 geom::SurfaceMap *smap,
								 std::vector<std::map<int, std::string>> *latSurfNameMap)
{
	const math::Point &center = fillingData.baseUnitElement()->center();
	const std::vector<std::pair<geom::Plane, geom::Plane>> &unitPlanePairs = fillingData.baseUnitElement()->planePairs();
	const std::vector<math::Vector<3>> &indexVectors = fillingData.baseUnitElement()->indexVectors();
//	mDebug() << "Generating planes.";
//	for(const auto& planePair: unitPlanePairs) {
//		mDebug() << "unit plane(+-) ===" << planePair.first.toString() << ",  " << planePair.second.toString();
//	}
//	mDebug() << "center=============" << center;

		// i方向の平面生成
		int minRange = 0, maxRange = 0;
		//mDebug() << "positive surfs size========" << unitPositiveSurfaces.size();
		for(int dirIndex = 0; dirIndex < static_cast<int>(unitPlanePairs.size()); ++dirIndex) {
			const geom::Plane &plusPlane = unitPlanePairs.at(dirIndex).first;
			const geom::Plane &minusPlane = unitPlanePairs.at(dirIndex).second;
			const math::Vector<3> &displacement = indexVectors.at(dirIndex);

	//		math::Vector<3> displacement = plusPlane.normal()*plusPlane.distance() - minusPlane.normal()*minusPlane.distance();
	//		mDebug() << "dirindex=" << dirIndex << "単位Plane正側=" << plusPlane.toString();
	//		mDebug() << "dirindex=" << dirIndex << "単位Plane負側=" << minusPlane.toString();
	//		mDebug() << "dirindex=" << dirIndex << "Displacement=" << displacement;

	//		mDebug() << "dirIndex===========" << dirIndex << "latticearg" << latticeArg_;
			if(dirIndex == 0) {
				// i indexのレンジ
				minRange = 2*irange.first - 1;
				maxRange = 2*irange.second +1;
			} else if (dirIndex == 1) {
				//j indexのレンジ
				minRange = 2*jrange.first - 1;
				maxRange = 2*jrange.second + 1;
			} else {
				// k indexのレンジ
				minRange = 2*krange.first - 1;
				maxRange = 2*krange.second + 1;
			}

			std::map<int, std::string> tmpMap;
			for(int i = minRange; i <= maxRange; ++i) {
				if(i%2 ==0) continue;	// 矩形格子では奇数番目の面のみ作成する。
				auto planeName = plusPlane.name() + minusPlane.name() + "_@" + std::to_string(i);

				// 基準点は要素中心
				auto tmpPlane = std::make_shared<geom::Plane>(planeName, plusPlane.normal(), math::dotProd(plusPlane.normal(), center + 0.5*i*displacement));
				//mDebug() << "i=" << i << " Creating plane =" << tmpPlane->toString();
				std::shared_ptr<geom::Surface> tmpPlaneR = tmpPlane->createReverse();
				smap->registerSurface(tmpPlane->getID(), tmpPlane);
				smap->registerSurface(tmpPlaneR->getID(), tmpPlaneR);
				tmpMap[i] = planeName;
			}

			latSurfNameMap->emplace_back(std::move(tmpMap));
		}
}


// 基本要素定義面名plname1, plname2, DimDecl i, jの作成する面の名前
std::string hexPlaneName(const geom::Plane &pl1, const geom::Plane &pl2, int i, int j)
{
	return pl1.name() + pl2.name() + "_@" + std::to_string(i) + "_@" + std::to_string(j);
}
std::string squarePlaneName(const geom::Plane &pl1, const geom::Plane &pl2, int i)
{
	return pl1.name() + pl2.name() + "_@" + std::to_string(i);
}

void appendHexLatticeSurfaces(	 const inp::FillingData &fillingData,
								 const std::pair<int, int> &irange,
								 const std::pair<int, int> &jrange,
								 const std::pair<int, int> &krange,
								 geom::SurfaceMap *smap,
								 std::vector<std::map<int, std::map<int, std::map<int, std::string>>>> *hexSurfNameMap)
{
	//mDebug() << "Generating planes.";
	//for(const auto& planePair: unitPlanePairs) {
	//	mDebug() << "unit plane(+-) ===" << planePair.first.toString() << ",  " << planePair.second.toString();
	//}
	//mDebug() << "center=============" << center;
	const std::vector<std::pair<geom::Plane, geom::Plane>> &planePairs = fillingData.baseUnitElement()->planePairs();
	const std::vector<math::Vector<3>> &indexVectors = fillingData.baseUnitElement()->indexVectors();


	/*
	 *  irange, jrange, krangeはDimension Declaratorの値で、これらはそれぞれの次元の要素数に対応する。
	 * 六角形格子ではxy平面上に三種類の面(s, t, u面)を作る必要がある。
	 */
	std::vector<math::Vector<3>> unitNormals; // s, t, uインデックスに対応する、-側面から+側面へのベクトル
	assert(planePairs.size() >= 3); // 六角格子なので基本要素を構成する面は3ペア以上
	for(const auto& pPair: planePairs) {
		math::Point pt = pPair.second.normal()*pPair.second.distance();// 負側の面上の点
		double dist = std::abs(math::dotProd(pt, pPair.first.normal()) - pPair.first.distance()); // 面間距離
		unitNormals.emplace_back(dist*pPair.second.normal());
	}

	mDebug() << "lattice element normals ===" << unitNormals;

	for(size_t index = 0; index < 3; ++index) {
		const geom::Plane &unitPlusPlane = planePairs.at(index).first;
		const geom::Plane &unitMinusPlane = planePairs.at(index).second;
		// 六角形体系では面はに種類のインデックス番号で管理されなければならない。
		std::map<int, std::map<int, std::map<int, std::string>>> tmpMap;
		// tmpMap[i][j][0] が格子要素[i,j]のマイナス側面、
		// tmpMap[i][j][1] が同プラス側面

		std::string planeName;
		// k方向はi,j方向と直行し、六角格子でも面を共有可能なのでここ(要素定義面作成)ではkループは回さない
		for(int i = irange.first; i <= irange.second; ++i) {
			for(int j = jrange.first; j <= jrange.second; ++j) {
				// [i,j]要素を構成する面(s,t,u面)を作成する。
				planeName = hexPlaneName(unitPlusPlane, unitMinusPlane, i, j);

				// 要素[i,j]の中心
				const math::Point center = fillingData.baseUnitElement()->center() + i*indexVectors.at(0) + j*indexVectors.at(1);
				math::Point refPoint = unitMinusPlane.normal()*unitMinusPlane.distance() + center;  // minusPlane上の点
				auto tmpPlane = std::make_shared<geom::Plane>(planeName, unitMinusPlane.normal(), refPoint);
				std::shared_ptr<geom::Surface> tmpPlaneR = tmpPlane->createReverse();
				smap->registerSurface(tmpPlane->getID(), tmpPlane);
				smap->registerSurface(tmpPlaneR->getID(), tmpPlaneR);
				//int mapIndex = (index == 0) ? i : j;
				tmpMap[i][j][0] = planeName;
//				mDebug() << "Creating plane. index ===" << index << "i, j===" << i << j << " minus plane===" << tmpPlane->toInputString();

				if(index == 0) {
					tmpMap[i-1][j][1] = planeName;
//					mDebug() << "Creating plane. index ===" << index << "i, j===" << i-1 << j << " plus plane===" << tmpPlane->toInputString();

				} else if(index == 1) {
					tmpMap[i][j-1][1] = planeName;
//					mDebug() << "Creating plane. index ===" << index << "i, j===" << i << j-1 << " plus plane===" << tmpPlane->toInputString();

				} else if(index == 2) {
					tmpMap[i+1][j-1][1] = planeName;
//					mDebug() << "Creating plane. index ===" << index << "i, j===" << i+1 << j-1 << " plus plane===" << tmpPlane->toInputString();

				}


				/*
				 * [imax,j]要素では +s面       を作成する必要がある。
				 * [i,jmax]要素では +t面、+u面 を作成する必要がある
				 * [i,jmin]要素では       +u面 を作成する必要がある
				 *
				 */

				if(i == irange.second && index == 0) {
					// +s面作成
					planeName = hexPlaneName(unitPlusPlane, unitMinusPlane, i+1, j);
					refPoint = unitPlusPlane.normal()*unitPlusPlane.distance() + center;  // plusPlane上の点
					tmpPlane = std::make_shared<geom::Plane>(planeName, unitMinusPlane.normal(), refPoint); // 法線は+-面どちらも同じ。
					tmpPlaneR = tmpPlane->createReverse();
					smap->registerSurface(tmpPlane->getID(), tmpPlane);
					smap->registerSurface(tmpPlaneR->getID(), tmpPlaneR);
					tmpMap[i][j][1] = planeName;
//					mDebug() << "Creating plane. index ===" << index << "i, j===" << i << j << "Splus plane===" << tmpPlane->toInputString();
				} else if(i == irange.first && index == 2) {
					// iが下限値の場合、+u面も作成する必要がある。
					planeName = hexPlaneName(unitPlusPlane, unitMinusPlane, i-1, j+1);
					refPoint = unitPlusPlane.normal()*unitPlusPlane.distance() + center;  // plusPlane上の点
					tmpPlane = std::make_shared<geom::Plane>(planeName, unitMinusPlane.normal(), refPoint); // 法線は+-面どちらも同じ。
					tmpPlaneR = tmpPlane->createReverse();
					smap->registerSurface(tmpPlane->getID(), tmpPlane);
					smap->registerSurface(tmpPlaneR->getID(), tmpPlaneR);
					tmpMap[i][j][1] = planeName;
//					mDebug() << "Creating plane. index ===" << index << "i, j===" << i << j << "Splus plane===" << tmpPlane->toInputString();
				}


				if(j == jrange.second && index == 1) {
					// +t面
					planeName = hexPlaneName(unitPlusPlane, unitMinusPlane, i, j+1);
					refPoint = unitPlusPlane.normal()*unitPlusPlane.distance() + center;  // plusPlane上の点
					tmpPlane = std::make_shared<geom::Plane>(planeName, unitMinusPlane.normal(), refPoint); // 法線は+-面どちらも同じ。
					tmpPlaneR = tmpPlane->createReverse();
					smap->registerSurface(tmpPlane->getID(), tmpPlane);
					smap->registerSurface(tmpPlaneR->getID(), tmpPlaneR);
					tmpMap[i][j][1] = planeName;
//					mDebug() << "Creating plane. index ===" << index << "i, j===" << i << j << "Tplus plane===" << tmpPlane->toInputString();
				} else if((j == jrange.second || j == jrange.first) && index == 2) {
					// +u面
					planeName = hexPlaneName(unitPlusPlane, unitMinusPlane, i-1, j+1);
					refPoint = unitPlusPlane.normal()*unitPlusPlane.distance() + center;  // plusPlane上の点
					tmpPlane = std::make_shared<geom::Plane>(planeName, unitMinusPlane.normal(), refPoint); // 法線は+-面どちらも同じ。
					tmpPlaneR = tmpPlane->createReverse();
					smap->registerSurface(tmpPlane->getID(), tmpPlane);
					smap->registerSurface(tmpPlaneR->getID(), tmpPlaneR);
					tmpMap[i][j][1] = planeName;
//					mDebug() << "Creating plane. index ===" << index << "i, j===" << i << j << "Uplus plane===" << tmpPlane->toInputString();
				}
			}
		}
		// hexSurfNameMapは hexSurfNameMap[s][i][j]で[i,j]要素のs方向のマイナス側面
		// latSurfNameMap[stu方向番号0=s方向、1=方向][その方向のインデックス]
		hexSurfNameMap->emplace_back(std::move(tmpMap));
	}
	// ここからはstuに直交する方向。3次元なら面がある。
	if(fillingData.dimension()== 3) {
		const geom::Plane &unitPlusPlane = planePairs.at(3).first;
		const geom::Plane &unitMinusPlane = planePairs.at(3).second;
//		mDebug() << "z方向の単位要素の+面===" << unitPlusPlane.toInputString();
//		mDebug() << "z方向の単位要素の-面===" << unitMinusPlane.toInputString();
		std::map<int, std::map<int, std::map<int, std::string>>> tmpMap;
		for(int k = krange.first; k <= krange.second; ++k) {
			std::string planeName = squarePlaneName(unitPlusPlane, unitMinusPlane, k);
			// 要素[i,j]の中心
			const math::Point center = fillingData.baseUnitElement()->center() + k*indexVectors.at(3);
			math::Point refPoint = unitMinusPlane.normal()*unitMinusPlane.distance() + center;  // minusPlane上の点
			auto tmpPlane = std::make_shared<geom::Plane>(planeName, unitMinusPlane.normal(), refPoint);
			std::shared_ptr<geom::Surface> tmpPlaneR = tmpPlane->createReverse();
			smap->registerSurface(tmpPlane->getID(), tmpPlane);
			smap->registerSurface(tmpPlaneR->getID(), tmpPlaneR);
			tmpMap[0][k][0] = planeName;
			tmpMap[0][k-1][1] = planeName;

			// 端っこ
			if(k == krange.second) {
				planeName = squarePlaneName(unitPlusPlane, unitMinusPlane, k+1);
				// 要素[i,j]の中心
				math::Point refPoint = unitPlusPlane.normal()*unitPlusPlane.distance() + center;
				auto tmpPlane = std::make_shared<geom::Plane>(planeName, unitMinusPlane.normal(), refPoint);
				std::shared_ptr<geom::Surface> tmpPlaneR = tmpPlane->createReverse();
				smap->registerSurface(tmpPlane->getID(), tmpPlane);
				smap->registerSurface(tmpPlaneR->getID(), tmpPlaneR);
				tmpMap[0][k][1] = planeName;
			}
		}
		hexSurfNameMap->emplace_back(std::move(tmpMap));
	}
}



// Laticeの要素を定義するための面を生成する。
void geom::LatticeCreator::generatePlanes(const std::pair<int, int> &irange,
									 const std::pair<int, int> &jrange,
									 const std::pair<int, int> &krange,
									 SurfaceMap *smap)
{


//	const std::vector<std::pair<Plane, Plane>> &unitPlanePairs = fillingData_.baseUnitElement()->planePairs();

    if(fillingData_.baseUnitElement()->planePairs().empty()) {
        throw std::invalid_argument("In creating lattice element, unit surface is empty.");
	}
	assert(fillingData_.dimension() <= 3 && fillingData_.dimension() > 0);
	assert(latticeArg_ == 1 || latticeArg_ == 2);
	if(latticeArg_ == 1) {
		appendSquareLatticeSurfaces(fillingData_, irange, jrange, krange,
									smap,
									&latticeSurfaceNameMap_);
	} else if (latticeArg_ == 2) {
		appendHexLatticeSurfaces(fillingData_,
								 irange, jrange, krange,
								 smap,
								 &hexSurfaceNameMap_);
	}
}


// [i,j,k]要素を構成する面の名前を返す。
std::vector<std::string> geom::LatticeCreator::getLatticeSurfaceNames(int i, int j, int k) const
{
	// indexと構築時のサイズの整合性チェック
	if(fillingData_.dimension() == 2 && k != 0) {
		throw std::invalid_argument("k-index is not zero while lattice is 2D. k=" + std::to_string(k));
	}
	assert(latticeArg_ == 1 || latticeArg_ == 2);

	std::vector<std::string> retVec;
	if(latticeArg_ == 1) {
		// 要素番号のiからi方向の内部インデックスを計算する。
		//iPlusIndex
		auto iIndexPair = iPlaneIndexPair(i, j, k);
		auto jIndexPair = jPlaneIndexPair(i, j, k);
		auto kIndexPair = kPlaneIndexPair(i, j, k);
		// s, tインデックス方向に関しては矩形/六角形格子, 2次元/3次元格子共通
		retVec.emplace_back("-" + latticeSurfaceNameMap_.at(0).at(iIndexPair.second));
		retVec.emplace_back(latticeSurfaceNameMap_.at(0).at(iIndexPair.first));
		retVec.emplace_back("-" + latticeSurfaceNameMap_.at(1).at(jIndexPair.second));
		retVec.emplace_back(latticeSurfaceNameMap_.at(1).at(jIndexPair.first));
		if(fillingData_.dimension() == 3) {
			retVec.emplace_back("-" + latticeSurfaceNameMap_.at(2).at(kIndexPair.second));
			retVec.emplace_back(latticeSurfaceNameMap_.at(2).at(kIndexPair.first));
		}
	} else if(latticeArg_ == 2) {
		// s方向
		retVec.emplace_back("-" + hexSurfaceNameMap_.at(0).at(i).at(j).at(1));
		retVec.emplace_back(      hexSurfaceNameMap_.at(0).at(i).at(j).at(0));
		// t方向
		retVec.emplace_back("-" + hexSurfaceNameMap_.at(1).at(i).at(j).at(1));
		retVec.emplace_back(      hexSurfaceNameMap_.at(1).at(i).at(j).at(0));
		// u方向
		retVec.emplace_back("-" + hexSurfaceNameMap_.at(2).at(i).at(j).at(1));
		retVec.emplace_back(      hexSurfaceNameMap_.at(2).at(i).at(j).at(0));
		if(fillingData_.dimension() == 3) {
			auto umap = hexSurfaceNameMap_.at(3).at(0).at(k);
			retVec.emplace_back("-" + hexSurfaceNameMap_.at(3).at(0).at(k).at(1));
			retVec.emplace_back(      hexSurfaceNameMap_.at(3).at(0).at(k).at(0));
		}
	}
	return retVec;
}

std::string geom::LatticeCreator::getLatticeBBString(int i, int j, int k, const geom::BoundingBox &unitBB) const
{
	// lattice要素のBBは言うほど簡単ではない。なぜならi方向がxを向いているとは限らないため。
	// 真面目に計算するのは基本単位要素[0,0,0]だけにしてあとはそれの平行移動で作るべし
	auto disp = fillingData_.baseUnitElement()->indexVectors();

	/*
	 * displacementsは latticeセルの定義でP1P2P3P4P5P6P7P8としている時
	 * disp(0) P2P1
	 * disp(1) P4P3
	 * disp(2) P6P5
	 * disp(3) P7P8
	 * から算出しているどの成分がどの軸に対応しているかは
	 * ここで対応付けることになる。
	 *
	 * このクラスではk方向はi,j平面と直交する方向であることを仮定していることに要注意。
	 */

	math::Vector<3> transVec{0, 0, 0};
	transVec += i*disp.at(0);
	transVec += j*disp.at(1);  // ここまでで水平移動完了
	if(fillingData_.dimension() == 3) {
		 transVec += (latticeArg_ == 2) ? k*disp.at(3) : k*disp.at(2);
	}
	auto tmpBB = unitBB;
	tmpBB.translation(transVec);
//	mDebug() << "element i,j,k=" << i << j << k << ", transVec=" << transVec << "bb=" << tmpBB.toInputString();
	return tmpBB.toInputString();
}

// minus側インデックスがfirstになる。
std::pair<int, int> geom::LatticeCreator::iPlaneIndexPair(int i, int j, int k) const
{
	(void) k;
	int ipindex = (latticeArg_ == 1) ? 2*i + 1 : 2*i + j + 1;
	return std::make_pair(ipindex - 2, ipindex);
}

std::pair<int, int> geom::LatticeCreator::jPlaneIndexPair(int i, int j, int k) const
{
	(void) k;
	int jpindex = (latticeArg_ == 1) ? 2*j + 1 : i + 2*j + 1;
	return std::make_pair(jpindex - 2, jpindex);
}

std::pair<int, int> geom::LatticeCreator::kPlaneIndexPair(int i, int j, int k) const
{
	(void) i;
	(void) j;
	return std::make_pair(2*k - 1, 2*k + 1);
}
