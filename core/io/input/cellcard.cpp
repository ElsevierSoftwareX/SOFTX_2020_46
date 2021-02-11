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
#include "cellcard.hpp"

#include <atomic>
#include <stdexcept>
#include <mutex>
#include <regex>
#include <stdexcept>
#include <thread>
#include "cellparameter.hpp"
#include "filldata.hpp"
#include "ijmr.hpp"
#include "cardcommon.hpp"
#include "core/math/nmatrix.hpp"
#include "core/utils/utils.hpp"
#include "core/formula/fortran/fortnode.hpp"
#include "core/formula/logical/lpolynomial.hpp"
#include "core/formula/logical/logicalfunc.hpp"
#include "core/geometry/cellcreator.hpp"
#include "core/geometry/surfacecreator.hpp"

namespace {
// パラメータ名はアルファベットと:*が使われる。パラメータは空白なし文字列か()でくくった文字列
std::regex cellParamPattern(R"(([\w:*]+) *= *(\( *[-+*/%\w\d .{},()]+ *\)|[-+\w\d.]+))");
std::regex likebutPattern(R"(like +([\d\w]+) +but)");
//	std::regex surfacePattern(R"(([ :(]|^)\"*([-+]*)([\d\w._@]+)\"*([ :)]|$))");
// むしろ半角空白は内部表現としても不可に変更
//std::regex surfacePattern(R"(([ :(]|^)(\"*)([-+]*)([\d\w._@]+)(\"*)([ :)]|$))");
std::regex surfacePattern(R"(([ :(]|^)(\"*))" + inp::surfaceNameRegexStr() + R"((\"*)([ :)]|$))");

}

constexpr int inp::CellCard::NOT_USED_ORDER;


// cell名  matindex 密度 式 からcardを作成
// cellcardのセル名、材料、密度を文字列化して出力
std::string inp::CellCard::getHeaderString() const
{
	std::stringstream ss;
	// cell名と物質名、密度
	ss << name << " " <<  materialName << " ";
	if(materialName != "0" && materialName != "-1") {
		ss << density;
	}
	ss << " ";	
	return ss.str();
}

std::string inp::CellCard::getParamsString() const
{
	if(parameters.empty()) return "";

	std::stringstream ss;
	for(auto it = parameters.begin(); it != parameters.end(); ++it) {
		if(it != parameters.begin()) ss << " ";
		ss << it->first << "=" << it->second;
	}
	return ss.str();
}

std::string inp::CellCard::toString() const
{
	std::stringstream ss;
	ss << name << " " <<  materialName << " ";
	if(materialName != "0" && materialName != "-1") {
		ss << density;
	}
	ss << " eq=" << equation;
	if(hasTrcl()) ss << " trcl=" <<trcl;
	if(!parameters.empty()) {
		ss << " parameters =(";
		for(auto &paramPair: parameters) {
			ss << "{" << paramPair.first << ", " << paramPair.second << "} ";
		}
		ss << ")";
	}
	if(!depends.empty()) {
		ss << ", dending on cells =";
		for(auto &elem: depends) {
			ss<< " " << elem;
		}

	}
	return ss.str();
}

std::string inp::CellCard::toInputString() const
{
	std::string retstr = "\"" + name + "\" " + materialName + " " + std::to_string(density) + " " + equation;
	if(!likeCell.empty()) {
		retstr += " like " + likeCell + " but";
	}
	for(auto &param: parameters) {
		retstr += " " + param.first + "=" + param.second;
	}
	return retstr;
}


// fill=パラメータで使われているunivをkeyに、適用されているTRをvalueにしたmultimapを返す。
// fill=1(2) 1(3)のような入力の場合 map(pair("1", "2"), pair("1", "2"))が返る
std::unordered_multimap<std::string, std::string> inp::CellCard::fillingUniverses() const
{
	// カードにfillパラメータがなければ当然からのマップを返す。
	if(parameters.find("fill") == parameters.end()) return std::unordered_multimap<std::string, std::string>();

	std::unordered_map<char, char> qmarks {{'"', '"'}, {'(', ')'}, {'{', '}'}};
	auto fillParams = utils::splitString(qmarks, " ", parameters.at("fill"), true);
	std::unordered_multimap<std::string, std::string> univMap;
	std::smatch sm;
	for(auto &param: fillParams) {
		//mDebug() << "fillParam=" << param;
		if(param.find_first_of(":") == std::string::npos
		&& std::regex_search(param, sm, std::regex(R"((\w*) *($|\(.*\)))"))) {
			univMap.emplace(sm.str(1), sm.str(2));
		}
	}
	return univMap;
}




/*
 * セルにTRCLが設定されている場合、
 * ・論理多項式内のsurface名をTR後の名前に置き換え、
 * ・新たに生成すべきsurfaceの情報(TRターゲットsurface名、TRCLしているcell名、TRCL内容のタプル)を返す。
 */
std::list<std::tuple<std::string, std::string, std::string>> inp::CellCard::solveTrcl()
{
//    mDebug() << "Enter solveTRCL, cardinput=" << toInputString();
	if(trcl.empty()) return std::list<std::tuple<std::string, std::string, std::string>>();
	// transform自体はsurfaceカードの方でやるのでここではTRCLからTRSFに渡すだけでよい。なので行列生成はしない。
	/*
	 * TRCLがあると、
	 * 1．セルを定義しているsurface名を新しいsurface名に置き換える
	 *    (当コードでは _cell名_surface名への置き換え)
	 *    ちなみにマクロボディの一部(1.1等)で定義されているcellはMCNPやTRCLできないがphitsではできる
	 *    phitsではsurface番号が1000以上でもTRCLできる。
	 *    また、コンプリメント部分はTRCLの影響を受けない。
	 * 2．TRCLされた新しいsurfaceを生成しなければならない。
	 *    コード間の依存を減らすためここではSurfaceCardの生成に留める。
	 */
	std::smatch surfaceMatch;
	/*
	 * smatch.str(1) 行頭、括弧あるいは演算子
	 * smatch.str(2) クオーテーションが掛かっている場合はここにマッチ
	 * smatch.str(3) 符号がある場合はここにマッチ
	 * smatch.str(4) surface名
	 * smatch.str(5) クオーテーションがある場合はここにマッチ、
	 * smatch.str(6) 行末、括弧あるいは演算子
	 *
	 */
	std::list<std::tuple<std::string, std::string, std::string>> retList;

	// まず論理多項式内のsurfaceをTR適用後の名前に変更する。
	auto searchBegin = equation.cbegin();
	while(std::regex_search(searchBegin, equation.cend(), surfaceMatch, surfacePattern)) {
		//mDebug() << "surface発見 名前=" << surfaceMatch.str(4);
		// surfaceMatch.str(3)が符号、surfaceMatch.str(4)がsurface名

		// spos-eposはクオーテーション"を含む
        std::string::size_type spos = static_cast<size_t>(std::distance(equation.cbegin(), surfaceMatch[2].first));
        std::string::size_type epos = static_cast<size_t>(std::distance(equation.cbegin(), surfaceMatch[5].second));
		std::string oldSignString = surfaceMatch.str(3);    // 符号部分の文字列 空,+,-のいずれか
		std::string oldSurfBaseName = surfaceMatch.str(4);  // 符号なしsurf名
		std::string newSurfBase = inp::getTransformedSurfaceName(name, oldSurfBaseName);
		std::string newSurfName = "\"" + oldSignString + newSurfBase + "\"";

//		mDebug() << "OLD surface名=" << oldSignString+oldSurfBaseName;
//		mDebug() << "new surfaceBase=" << newSurfBase;
//		mDebug() << "new surface名=" << newSurfName;

		//mDebug() << "Seaching string=" << std::string(searchBegin, equation.cend());
		//mDebug() << "oldcard equation=" << equation;
		// sposからepos-spos個の文字をnewSurfNameで置き換える。
		equation.replace(spos, epos-spos, newSurfName);
		//mDebug() << "newcard equation=" << equation;
		searchBegin = equation.begin();
		std::advance(searchBegin, spos + newSurfName.size());

		// TRCLでTR対象となったsurface名、TRCLしたセル名、TRSF内容のタプルを追加
		std::pair<char, char> quotes{'(', ')'};
		std::vector<std::string> trsfArgVec = utils::splitString(",", utils::dequote(quotes, trcl), true);
		std::string trsfStrs;
		for(auto &trStr: trsfArgVec) {
			trsfStrs = trsfStrs +  " trsf=(" + trStr + ")";
		}
		retList.emplace_back(oldSurfBaseName, name, trsfStrs);
	}
	return retList;
}

void inp::CellCard::addTrcl(const std::string &newTrStr, bool isBack)
{
	if(newTrStr.empty()) return;
	if(trcl.empty()) {
		trcl = newTrStr;
	} else {
		if(isBack) {
			trcl += "," + newTrStr;
		} else {
			trcl = newTrStr + "," + trcl;
		}
	}
	return;
}

/*
 * #  2 → 2を返す
 * (#2) → 2を返す。
 * # (2) →何も返さない。 surface complimentなのでcomplimentCell名ではないから
 */
// 論理多項式の文字列を受けとって依存先のセル名のベクトルを返す
std::vector<std::string> inp::GetComplimentCellNames(const std::string &str)
{
	//mDebug() << "Enter GetComplimentCellNames, str===" << str;
	std::vector<std::string> retVec;
	std::string::size_type sharpPos, prevSharpPos = 0, targetEndPos, targetStartPos;
	while(sharpPos = str.find_first_of("#", prevSharpPos), sharpPos != std::string::npos) {
		//numberEndPos = str.find_first_of(" :)(", sharpPos);
		targetStartPos = str.find_first_not_of(" ", sharpPos+1);
		if(targetStartPos == std::string::npos) throw std::invalid_argument("Equation ends with #. str=" + str);
		targetEndPos = str.find_first_of(" :)(", targetStartPos+1);

		std::string complimentTargetName = str.substr(targetStartPos, targetEndPos - targetStartPos);
		if(complimentTargetName.empty()) {
			throw std::invalid_argument("Empty depending cell is detected. (maybe program error), str=" + str);
		} else if(complimentTargetName.front() == '(') {
			//throw std::invalid_argument("Cell name starts with \"(\". (maybe surf compliment hasn't processed), str="+str);
		} else {
			retVec.emplace_back(complimentTargetName);
		}
		prevSharpPos = sharpPos + 1;
	}
	//mDebug() << "Eexit GetComplimentCellNames, result===" << retVec;
	return retVec;
}


inp::CellCard inp::CellCard::fromString(const std::string &str, bool checkValidUserInput)
{
//    mDebug() << "CellCard::fromString str===" << str;
	std::string cellStr = str;

	/*
	 * MCNPではセルパラメータの"="はオプショナルで、無くてもOK.
	 * すると、セル名、surface名に文字列を認めた場合、単純な正規表現マッチングでは
	 * 判別不能になりそうだが、セルパラメータはジオメトリ論理式の後ろに書くことと
	 * 決められているの(MCNP manual 1-5)で、これを使って判別する。
	 *
	 * 対処方針：
	 * まず入力順をみて、cell名、材料名、 密度を確定させる。
	 * あとはcellパラメータは予約語とのマッチングを取ってパラメータ部分を確定、unorder_mapへ分離する。
	 * 残った部分が消去法的にsurface名等の論理式となる。
	 *
	 * 留意点：
	 * ・univ名はijmr表現と重複してはならない。Fill=で与えられた時判別不能になるので
	 * ・セル名には#は用いてはならない。complimentと判別不能になるため。
	 * ・論理式以外のパラメータは U, TRCL, FILL, LAT DXC, EXT, FCL, IMP, NONU, PD, PWT, TMP, VOL, WWN  RHO  MAT    by manual A-16
	 * ・あとlike-butもある。
	 */

	/*
	 *  問題は 番号とparticle designatorがあるのとないのと。 NAME:n:m nとm
	 *  番号を持つものは番号を省略可能なので要注意
	 * ・両方ないもの  : u, trcl, fill, lat, nonu pwt vol rho mat
	 * ・両方あるもの  : dxc wwn
	 * ・designatorのみ: ext fcl imp
	 * ・番号のみ      : pd tmp
	 *
	 * パラメータ引数は
	 * u:  u= 1個 (u=1)
	 * trcl: = 可変長 (trcl=2  trcl=(cos(0) 2r 9j))
	 * fill: 可変長
	 * lat 1個(123のどれか)
	 * dxc: 1個 (dxc1:n=1) みたいな感じ。
	 * ext: 1個 (ext:n=1)
	 * fcl: 1個 (fcl:n=1)
	 * imp: 1個 (imp:n=1)
	 * nonu 0個
	 * pd:  1個 (pd1=1)
	 * pwt: 1個 (pwt=1)
	 * tmp: 1個 (tmp1=100)
	 * vol: 1個 (vol=10)
	 * wwn: 1個 (wwn1:N=0.5)
	 * rho: 1個 (row=0.5)
	 * mat: 1個 (mat=hydrogen)
	 *
	 * ※ trcl,fillは ijmr展開の対象となる。
	 *
	 *
	 */

	const std::string PSEP = " =";  // セパレータは空白とイコール
	std::string cellName = inp::cutFirstElement(PSEP, &cellStr);
	cellName = utils::canonicalName(cellName);
	// TODO checkValidCellName(cellName);
	std::string materialName =  inp::cutFirstElement(PSEP, &cellStr);
	// materialNameはあとでmatMapにアクセスする時に見つからなければ例外発生なのでここではチェック不要
	std::string densStr, likeCell;

	// 2番めの入力はlike, マテリアル名、0のどれか
	if(materialName.empty()) {
		throw std::invalid_argument("Cell input is too short. No material name found.");
	} else if(materialName == "like") {
		likeCell = inp::cutFirstElement(" ", &cellStr);  // "like=m"という表記はありえないのでセパレータは" "のみ。
		likeCell = utils::canonicalName(likeCell);
		std::string butStr = inp::cutFirstElement(" ", &cellStr);
		if(butStr.empty()) {
			throw std::invalid_argument("Cell input is too short. \"but\" not found after like-m");
		} else  if(butStr != "but") {
			throw std::invalid_argument("Parameter after like-m should be \"but\", actual=" + butStr);
		}
	} else if(materialName != "0" && materialName != "-1") {
		// 密度文字列が数値かどうかはFortran数式の可能性があるのであとで実施。
		densStr =  inp::cutFirstElement(" =", &cellStr);
		if(densStr.empty()) {
			throw std::invalid_argument("Too short cell input. No density input found.");
		}
	}
	// NOTE Fillingするときにはcell生成コストが実行時間にものすごく効くので、checkValid=falseならチェックしない
	// ここでmaterialNameがパラメータ名に該当していたら例外発生。
	if(checkValidUserInput) {
		if(isCellParam(materialName)) {
			throw std::invalid_argument("cell parameters shoud be placed at the end. str=" + str);
		}
	}


	std::vector<std::string> polynomialParams;
	std::unordered_map<std::string, std::vector<std::string>> paramvMap;
	std::string tmpStr;
	std::string trString;

	/*
	 * ！！！！重要！！！！
	 * 可変長パラメータの末尾と論理式のサーフェイス名は区別不可能(もともとのMCNP純正入力でも)
	 * なのでセルパラメータより後ろに論理式は含まれないものとする。これはmcnpマニュアルに明記してある。
	 *
	 * → すると…ijmr展開を無差別適用してもOK!!!
	 *
	 */

	// 先頭の多項式部分をpolynomialParamsに分離する。
	while(true) {
		if(cellStr.empty()) break;
		tmpStr = inp::cutFirstElement(PSEP, &cellStr);
		if(!isCellParam(tmpStr)) {
            // remove optional '+' char
            if(tmpStr.front()=='+') tmpStr = tmpStr.substr(1);
			polynomialParams.emplace_back(tmpStr);
		} else {
			cellStr = tmpStr + " " + cellStr;
			break;
		}
	}
	// 多項式のパラメータを分離したのでijmr展開する。
	// エラー処理はもう一階層上で実施するのでここではcatchしない。

	// TODO ここでphitsなら警告を出すべし。
	inp::ijmr::expandIjmrExpression(PSEP, &cellStr);
	utils::trim(&cellStr);

	// ここからはパラメータ部分の処理。
	while(utils::trim(&cellStr), !cellStr.empty()) {
		tmpStr = inp::cutFirstElement(PSEP, &cellStr);
		if(!isCellParam(tmpStr)) {
			// セルパラメータより後ろに多項式パラメータがあったらエラーにする。
			//std::cerr << "paramStr===\"" << tmpStr << "\", residual=\"" << cellStr << "\"." << std::endl;
			throw std::invalid_argument("Polynomial parameter exists after cell-parameter. param =" + tmpStr + ", data line = " + str);
		}
		// 固定長パラメータ(u,)
		auto result = isCellFixedParam(tmpStr);
		if(result.first) {
			// 固定長パラメータの格納。
			for(int i = 0; i < result.second; ++i) paramvMap[tmpStr].emplace_back(inp::cutFirstElement(PSEP, &cellStr));
			continue;
		}

		// #### ここからFILLパラメータの場合の特殊処理
		/*
		 * パラメータの正規表現は(R"(([\w:]+) *= *(\( *[-+\w\d .]+ *\)|[\w\d.]+))");
		 * という感じなので=のあとの「文字列1個」か「括弧でくくった文字列」としていた。
		 * だがしかし、FILLパラメータは特殊で括弧でくくらず可変長という面倒くさい仕様になっている。
		 * よってパラメータ名がfillだった時は汚い個別処理を導入する。
		 */
		if(tmpStr == "fill" || tmpStr == "*fill") {
			// fill=以降fillを含んだ全体を渡してfillingデータを読み取る。univ数がDimDeclより小さい場合はエラーにすべき。
			std::string laterThanFill = tmpStr + " " + cellStr;
//            mDebug() << "laterTHanFill===" << laterThanFill;
			inp::FillingData fillData = inp::FillingData::fromString(laterThanFill);
			if(fillData.isDegree()) tmpStr = tmpStr.substr(1); // degree指定時は*fill=なので*を外す
			//FillData::toInputString()はdegree指定をfillではなくunivの方に移すので、以後は必ず*fillではなくfill=となる。
			paramvMap[tmpStr].emplace_back(fillData.toInputString());
//            mDebug() << "filldatastring=" << fillData.toInputString();
//            mDebug() << "filldata.readcount=" << fillData.readCount();
//            mDebug() << "submatch result string!!";

			/*
			 * セルカードのパラメータ文字列を除去する。
			 * Fill=の場合は以降が可変長で不規則なので何文字あるかはFillingDataに
			 * 読み込み・計算を任せている。
			 *
			 * paramMatch.str(1)がパラメータ名称
			 * paramMatch.str(2)がバラメータ引数文字列
			 *
			 * FillingData::readCount()は
			 *
			 * paramMatch[2].firstはパラメータ値部分の先頭を指しているので、
			 * そこからFillingDataがfillパラメータだと認識した文字数(readCount())
			 * だけ進ませてfillパラメータの引数文字列後端へと移動させている。
			 */
//			auto fillParamEnd = paramMatch[2].first;
//			// fillData.readCountが実際より大きい場合、fillParamEndが文字列の外に出る。readCountが正しければ本来起こらない。
//			assert(cellStr.size() >= std::distance(cellStr.cbegin(), fillParamEnd) + fillData.readCount());
//			std::advance(fillParamEnd, fillData.readCount());
////            mDebug() << "cellStr=" << cellStr;
////            mDebug() << "先頭からparamMatch[0].firstまで" << std::string(cellStr.cbegin(), paramMatch[0].first);
////            mDebug() << "削除区間=" << std::string(paramMatch[0].first, fillParamEnd);
//			cellStr.erase(paramMatch[0].first, fillParamEnd);

			cellStr = cellStr.substr(fillData.readCount());

		}else if(tmpStr == "trcl" || tmpStr == "*trcl") {
			//mDebug() << "before tmpStr ===" << tmpStr << "currentTrString===" << trString;
			getCellTrStr(tmpStr.front() == '*', &cellStr, &trString);
			//mDebug() << "before tmpStr ===" << tmpStr << "currentTrString===" << trString;
		} else {
			// wwn, tmpは可変長で終端マークが無いため次のセルパラメータまでを入力として解釈する。
			std::string paramName = tmpStr;
			while(tmpStr = inp::cutFirstElement(" ", &cellStr), !isCellParam(tmpStr)){
				if(tmpStr.empty()) break;
				paramvMap[paramName].emplace_back(tmpStr);
			}
			// whileループ先頭でのcutFirstElementに合わせるようにセルパラメータを戻す。
			cellStr = tmpStr + " " + cellStr;
		}
	}



	// ここからカード生成
	// ####### ここまででcellInputsは{セル名、マテリアル名　密度　論理式…}となっている。
	CellCard card;
	if(!likeCell.empty()) {
	// like-butカード
		if(polynomialParams.size() > 1) {
			throw std::invalid_argument("Both like-but and logical polynomial are found. polynomial=" +  utils::concat(polynomialParams, " "));
		} else {
			// TODO セルカードのコンストラクタではパラメータマップはmap<string, string>なので変換する。いずれ統一する。
			std::unordered_map<std::string, std::string> paramMap;
			for(const auto &p: paramvMap) {
				paramMap.emplace(p.first, utils::concat(p.second, ","));
			}
			// like-batが使用されている場合はセル定義論理多項式は存在しないのでコンプリメントも存在しないとしてよい。
			card = CellCard(cellName, "", 0, "", likeCell, paramMap, std::vector<std::string>{likeCell}, trString);
		}
	} else {
	// それ以外のカード
		if(polynomialParams.empty()) {
			throw std::invalid_argument("Too short cell card. Logical polynomial is empty");
		}

		double density = 0;
		// 通常セル用の密度を計算。特殊セルのエラーチェックは冒頭で実施済み。
		if(materialName != "0" && materialName != "-1") density = fort::eq(densStr);

		/*
		 * polynomialParamsはセルの定義式を空白で分割してた内容が入っている。
		 *
		 */
		// #やカッコの間にスペースがある場合 "#  ( -42 43)"の場合はparamsは{#, (, -42, 43)}となっているので
		// スペースなしの場合と処理を共通化するため、次の要素に連結しておく。
		// 逆に")"だけの要素も前に連結する必要がある。
		// 空白要素はsplit時にignoreしているので考える必要ない。
		for(auto it = polynomialParams.begin(); it != polynomialParams.end();) {
			if(it->find_first_not_of("#(") == std::string::npos) {
				if(it+1 == polynomialParams.end()) {
					throw std::invalid_argument("polynomial parameters end with # or (.");
				}
				*(it+1) = *it + *(it+1);
				it = polynomialParams.erase(it);
			} else if(it->find_first_not_of(")") == std::string::npos) {
				if(it == polynomialParams.begin()) {
					throw std::invalid_argument("polynomial parameters start with ).");
				}
				*(it-1) = *(it-1) + *it;
				it = polynomialParams.erase(it);
				--it;
			} else {
				++it;
			}
		}

		// #(-42 43 -44)みたいな面コンプリメントはpolynomialParamsでは #(-42, 43, -44)のように区切られて格納されている。
		// このままだと右側括弧が見つからないで落ちるので、右側括弧が見つかるまで連結しておく

		for(auto it = polynomialParams.begin(); it != polynomialParams.end();) {
			// セルコンプリメントと混同しないように。
			if((it->size() > 1 && it->substr(0, 1) == "(")  ||  (it->size() > 2 && it->substr(0, 2) == "#(")) {
				int balance = utils::parenthesisBalance('(', ')', *it, false);
				//mDebug() << "要素*it===" << *it << "シャープ発見！！！ バランス===" << balance;
				if(balance == 0) {
					++it;
					continue;  // 括弧のバランスが取れている場合は連結の必要なしなのでiteratorを進めてcontinue
				}
				// polynomialへの入力に左括弧が見つかった場合
				auto headElementIt = it;  // #(で始まる先頭部分
				while(++it, (it != polynomialParams.end() && balance != 0)) {
					//mDebug() << "headElement===" << *headElementIt << "*it===" << *it;
					*headElementIt += " " + *it;
					balance += utils::parenthesisBalance('(', ')', *it, true);
					it = polynomialParams.erase(it);
					--it;
				}
				// 上のwhileを抜けた時はit==endかbalance==0のどちらか(あるいは両方)。
				// end到達なら対応括弧なしなので例外発生
				if(it == polynomialParams.end() && balance != 0) {
					throw std::invalid_argument("No right-side blacket found in surfce compliment. string=" + *headElementIt);
				}
			} else {
				++it;  // surfaceコンプリメントでない場合はiteratorを進めるだけ。
			}
		}
		// 連結作業終わり。
		//mDebug() << "面コンプリメント連結後のパラメータvector===" << polynomialParams;

		/*
		 * 面コンプリメントの展開。
		 *
		 * (入れ子)面コンプリメントはLogicalExpression::fromString方へ移したから、セルカード作成時には実施しなくても良くなったはず
		 */
//		for(size_t i = 0; i < polynomialParams.size(); ++i) {
//			//mDebug() << "logicParams i===" << i << "str===" << polynomialParams.at(i);
//			// ここで# と (の間は空白が入る場合があるので対応しているが、本来要素には空白は含まれないので無視しても良い。
//			//std::string::size_type pos = polynomialParams.at(i).find("#(");
//			std::string::size_type sharpPos = polynomialParams.at(i).find("#");
//			if(sharpPos == std::string::npos) continue;  // #がないなら何もしない。
//			std::string::size_type lParPos = polynomialParams.at(i).find_first_not_of(" ", sharpPos+1);  // 左括弧位置
//			if(lParPos == std::string::npos || polynomialParams.at(i).at(lParPos) != '(') {
//				continue;  // #が無いならなにもしない。#の後が(でない場合もsurfaceコンプリメントではいから何もしない
//			}

//			std::string::size_type targetEndpos = utils::findMatchedBracket(polynomialParams.at(i), lParPos, '(', ')');
//			if(targetEndpos == std::string::npos) {
//				mDebug() << "params===" << polynomialParams;
//				throw std::invalid_argument("No right-side blacket found in surfce compliment. str=" + polynomialParams.at(i));
//			}
//			std::string argStr = polynomialParams.at(i).substr(lParPos+1, targetEndpos-lParPos-1);
//			std::string replacingStr = lg::complimentedString(argStr);
//			//				auto poly = lg::Polynomial::fromString(argStr).compliment();
//			//				polynomialParams.at(i).replace(pos, endpos-pos+1, poly.toString());
//			polynomialParams.at(i).replace(sharpPos, targetEndpos-sharpPos+1, replacingStr);

//		}



		auto dependingCells = GetComplimentCellNames(utils::concat(polynomialParams, " "));  //依存先セル名を保存
		for(auto &dcellName: dependingCells) dcellName = utils::canonicalName(dcellName);
		//mDebug() << "CellName ===" << cellName <<"polys===" << polynomialParams << "dpendingCells===" << dependingCells;
		std::unordered_map<std::string, std::string> paramMap;
		for(const auto &p: paramvMap) {
			paramMap.emplace(p.first, utils::concat(p.second, ","));
		}
		card = CellCard(cellName, materialName, density, utils::concat(polynomialParams, " "), "", paramMap, dependingCells, trString);
	}

	// ここでやっとカード作成完了。あとはチェック
	// 不適格なカードはここで例外を発生させる。
	checkNameCharacters(card.name, checkValidUserInput);
	if(std::find(card.depends.begin(), card.depends.end(), card.name) != card.depends.end()) {
		throw std::invalid_argument("Self-reference-cell is invalid.");
	} else if (card.name.front() == '*') {
		throw std::invalid_argument(std::string("Cell name starring with * is forbidden, name = \"") + card.name + "\"");
	}
	std::regex invalidParamPattern(R"(^[-+0-9.]+)");
	std::smatch invParamMatch;
	for(auto it = card.parameters.begin(); it != card.parameters.end(); ++it) {
		if(std::regex_search(it->first, invParamMatch, invalidParamPattern)) {
			throw std::invalid_argument(std::string("Invalid card because \"")
										+ it->first + "\" is recognized as the parameter name");
		}
	}

	return card;
}

inp::CellCard inp::CellCard::fromString(const std::string &file, size_t line, const std::string &str, bool checkValidUserInput)
{
	auto card = fromString(str, checkValidUserInput);
	card.file = file;
	card.line = line;
	return card;
}

const std::regex &inp::CellCard::getDeclPattern()
{
	static std::regex declaratorPattern(R"( *([-+]*\w+): *([-+]*\w+) +([-+]*\w+) *: *([-+]*\w+) +([-+]*\w+) *: *([-+]*\w+))");
	return declaratorPattern;
}


// dcardセルカードの依存しているセルカードをsolvedCardsから検索して、dcardの依存を解決する。解決できたらtrue
// 第二引数は論理多項式のコンプリメントを取得する時に必要。TODO index値は不要なので本来削除できるはずなので削除したい。
bool inp::CellCard::solveCellCardDependency(const std::unordered_map<std::string, int> &nameIndexMap,
											 const std::unordered_map<std::string, inp::CellCard> &solvedCards,
											 inp::CellCard *dcard)
{
	// LogicalExpression::fromStringは不正な式の場合例外を投げるので適切な位置でキャッチする必要がある。
    // 入力ファイルの内のエラー発生場所ががわかるところ、かつルーチンの一番深いところでcatchする
	for(auto depIt = dcard->depends.begin(); depIt != dcard->depends.end();) {
		try{
			std::string dependingCellName = *depIt;
//			mDebug() << "セルの依存解決開始。解決対象===" << dcard->toString();
//			mDebug() << "依存先のセル名===" << dependingCellName;
			// 依存先セルがsolvedCardsに見つかったら依存性を解決する。
			if(solvedCards.find(dependingCellName) != solvedCards.end()) {
				// cardの生成が可能なタイミングになっているのでコンプリメントは式展開する。
				//mDebug() << "セル===" << dependingCellName << "は解決可能！";
				std::smatch sm;
				std::string regexStr = std::string("# *(") + dependingCellName + ")([ :)(]|$)";

				while(std::regex_search(dcard->equation, sm, std::regex(regexStr))) {
					//					while(std::regex_search(dcard.equation, sm, std::regex(R"(#([\d\w\.]+)([ :)]|$))"))) {
					const CellCard dependedCard = solvedCards.at(sm.str(1));
                    auto sharpPos = static_cast<std::string::size_type>(std::distance(dcard->equation.cbegin(), sm[0].first));
                    auto numEnd = static_cast<std::string::size_type>(std::distance(dcard->equation.cbegin(), sm[1].second));
					// 依存しているセルのa多項式
					// polyonmial::toString()は面倒だがマップが必要。(NOTE mapへのポインタをpolynomialに持たせる？)
					std::string replacing;
					if(dependedCard.hasTrcl()) {
						// 依存先がTRCLしている場合はsurface名はTRCL後の物にする必要があるのでconvMapを作成する。
						auto depPoly = lg::LogicalExpression<int>::fromString(dependedCard.equation, nameIndexMap);
						std::unordered_map<std::string, int> transformedMap;
						// NOTE 本来ここは使用する範囲だけで良い。
						for(auto & niPair: nameIndexMap) {
							transformedMap[inp::getTransformedSurfaceName(dependedCard.name, niPair.first)] = niPair.second;
						}
						replacing = depPoly.complimented().toString(transformedMap);
					} else {
                        replacing = lg::complimentedString<std::string>(dependedCard.equation);
					}

					//mDebug() << "compliment=" << sm.str(0) << ", replacing=" << replacing;
					//mDebug() << "replacing===" << replacing;
					dcard->equation.replace(sharpPos, numEnd - sharpPos, replacing);
					dcard->order = std::max(dependedCard.order + 1, dcard->order);
				}  // end while loop



				// likebutセルは MAT RHO処理をする。
				if(!dcard->likeCell.empty() && dcard->likeCell == dependingCellName) {
					CellCard likedCard = solvedCards.at(dcard->likeCell);
					dcard->equation = likedCard.equation;
					// likebutの時のlike先パラメータはTRCL以外を引きつぐ
					for(auto paramPair: likedCard.parameters){
						dcard->parameters.insert(paramPair);
					}
					if(dcard->parameters.find("mat") != dcard->parameters.end()) {
						dcard->materialName = dcard->parameters.at("mat");
					} else {
						dcard->materialName = likedCard.materialName;
					}
					if(dcard->parameters.find("rho") != dcard->parameters.end()) {
						dcard->density = utils::stringTo<double>(dcard->parameters.at("rho"));
					} else {
						dcard->density = likedCard.density;
					}
					dcard->order = std::max(likedCard.order + 1, dcard->order);
				}

				// これでdependingCellNameの依存関係は解決されたのでdcard.dependsから除く
				depIt = dcard->depends.erase(depIt);
			} else {
				++depIt;
			}

		} catch (std::exception &e) {
			throw std::runtime_error(dcard->pos() + " While solving cell dependency, "  + e.what());
		}
	}  // dependセル1個処理ループ終わり


	return dcard->depends.empty();
}


/*
 * Fillされているカードのuniverseを展開し、内部の個別のセルに分解するファンクタ
 *
 * この時並列化を念頭に置き、universeが複数のセルから構成されている場合
 * universe構成セルのベクトルの [start_index, end_index)までの範囲を
 * 処理する。この処理は要するにuniverse内部セルに外側セルの型をはめる
 * だけなので、univ内部セル同士の相互関係は無く、並列に実行できる。
 *
 * 入力
 * outerCard: fill=univでfillされているセル
 * fillingCards: univを構成するセルカードを集めたベクトル
 *
 * 出力
 * resultMap: outerCardを充填して作成されたセルカードのmap。キーはorder(優先度)
 *            作成されるセル数はuniv内部のセル数に等しい
 */
// セルのfilling処理の実装部分。
void fillingCell(geom::CellCreator *creator, int maxThread, size_t start_index, size_t end_index,
				 const std::unordered_map<size_t, math::Matrix<4>> &trMap,
//				 const std::unordered_multimap<std::string, inp::CellCard> &univMap,
				 const std::unordered_map<std::string, std::vector<inp::CellCard>> &univMap,
				 const inp::CellCard &outerCard,
				 const std::vector<inp::CellCard> &fillingCards,
				 const std::string &trString, // fill=u(x y z...)で指定されているTR文字列x y z...
				 const int depth, // 再帰呼び出し深さ
				 geom::SurfaceMap* surfMap,
				 std::vector<inp::CellCard> *resultVec,
				 std::atomic_bool *cancelFlag,
				 std::atomic_int *counter)
{
	if(cancelFlag->load()) return;
	//		//  outerCellのBBが求まるなら切り取れるように求めておく
	//		auto outerCell = creator->createCell(outerCard);
	//		geom::BoundingBox outerBB = outerCell->createBoundingBox();
	//		// mDebug() <<"filling outer cellcard=" << outerCard.name << "outerBB====" << outerBB.toInputString();

	// FIXME 並列化するならここのループを並列化するべきである。
	//mDebug() << "tID===" << std::this_thread::get_id() << "start,end===" << start_index << end_index << "num===" << end_index - start_index;
	for(size_t i = start_index; i < end_index; ++i) {
		if(cancelFlag->load()) return;
		const inp::CellCard fillingCard = fillingCards.at(i);
		// 充填セルのパラメータはu, trclを除き内側セルの物が引き継がれる
		// TRCLが、同階層TRCLは適用済みとして引き継がない点は重要。← こうなっっていなかった
		auto newParameters = fillingCard.parameters;
		newParameters.erase("u");
		newParameters.erase("trcl");
		// BoundingBoxオプションは内側にあれば内側の、なければ外側のものが初期値に使われる
		if(newParameters.find("bb") == newParameters.end()){
			auto bbIt = outerCard.parameters.find("bb");
			if(bbIt != outerCard.parameters.end()) newParameters.emplace(bbIt->first, bbIt->second);
		}

		// TRがある場合は1.fillで指定されているTR、2.外側セルのTRの順に適用する。
        inp::CellCard newCard(fillingCard.name + "<" + outerCard.name,
							  fillingCard.materialName,
							  fillingCard.density,
							  fillingCard.equation,
							  std::string(),  // likebutは処理済みなので考えなくて良い
							  newParameters,
							  std::vector<std::string>(),
							  trString);
		newCard.addTrcl(outerCard.trcl, true);
		newCard.order = std::max(fillingCard.order, outerCard.order);  // newCardのorderは大きい方。
		newCard.file = fillingCard.file;
		newCard.line = fillingCard.line;



		//			mDebug() << "Filling card =" << fillingCard.name;
		//			mDebug() << "New card=" << newCard.name;
		//mDebug() << "Fillで作成されたセルのカード=" << newCard.toString();
		// filledで生成されたカードにTRCLが含まれる場合あらたなsurfaceを登録処理する。
		if(newCard.hasTrcl()) {
			auto trCreatingSurfaceList = newCard.solveTrcl();
			// タプルは(TRターゲットsurface名、TRCLしているcell名、TRCL内容のタプル)
			for(auto trSurfTuple: trCreatingSurfaceList) {
				// ここはsurf = Surface::createTredSurface; surfMap->registerSurface()としたいが、
				// 新しいsurfを作成する時にsurfMapに登録されている情報が必要となるので
				// surfMap側のメソッドとする。
				try{
					/*
					 * FIXME TRされたセルを並列fillingしようとするとここでTR前のsufraceが
					 * surfacemapに見つからずにエラーになる。registerTrSurface実行時に
					 * ガードするか、registerTrSurfaceをスレッドセーフにするかの措置が必要。
					 * registerSurfaceはスレッドセーフにしたつもりだったがうまく言っていない。
					 */
					surfMap->registerTrSurface(trMap, trSurfTuple, true);
				} catch (std::out_of_range &e) {
					mDebug() << "Registering trSurfTuple, TRed surf===" << std::get<0>(trSurfTuple)
							 << "TRCLing cell===" << std::get<1>(trSurfTuple);
					throw std::runtime_error(
						   newCard.pos() + " " + e.what()
						   + ", this surface is not defined (or TRCL syntax is wrong?)");
				}
			}
		}
		newCard.equation = " (" + outerCard.equation + ") (" + newCard.equation + ")";
		//mDebug() << "fillで作成された新たなカードの式=" << newCard.equation;

		// innerCellのTRCLをparameterに戻す。
		// より下層のTRから先に適用されるのでinnerTrclを左側に置く
		newCard.addTrcl(fillingCard.trcl, false);

		// さらに深い階層がある場合は再帰的に解決する。
		if(newCard.parameters.find("fill") != newCard.parameters.end()) {
			auto filledCards = inp::CellCard::getFilledCards(creator, univMap, newCard, maxThread, depth, cancelFlag, counter);
			if(!filledCards.empty()) {
				resultVec->insert(resultVec->end(),
								  std::make_move_iterator(filledCards.begin()),
								  std::make_move_iterator(filledCards.end()));
			}
		} else {
			resultVec->emplace_back(std::move(newCard));
			// depth=1での充填セル数をprogressの基準としているので、
			// deopth==1でカードを生成したらカウンターを進める。
			if(depth == 1) {
				//mDebug() << "element card created ===" << resultVec->back().name;
				++(*counter);
			}
		}
	}
}

//class FillingWorker{
//public:
//	FillingWorker(std::exception_ptr ep): ex_ptr(ep) {;}
//	std::string exceptionString() const
//	{
//		// 要は通常の例外へ返還する
//		try{
//			if(ex_ptr){
//				std::rethrow_exception(ex_ptr);
//			} else {
//				return "";
//			}
//		} catch (std::exception &e) {
//			return e.what();
//		}
//	}
//	void operator() (geom::CellCreator *creator, int maxThread, size_t start_index, size_t end_index,
//					 const std::unordered_map<size_t, math::Matrix<4>> &trMap,
//	//				 const std::unordered_multimap<std::string, inp::CellCard> &univMap,
//					 const std::unordered_map<std::string, std::vector<inp::CellCard>> &univMap,
//					 const inp::CellCard &outerCard,
//					 const std::vector<inp::CellCard> &fillingCards,
//					 const std::string &trString, // fill=u(x y z...)で指定されているTR文字列x y z...
//					 geom::SurfaceMap* surfMap,
//					 std::vector<inp::CellCard> *resultVec)
//	{
//		try {
//			fillingCell(creator, maxThread, start_index, end_index, trMap, univMap, outerCard, fillingCards, trString, surfMap, resultVec);
//        } catch (...) {
//			ex_ptr = std::current_exception();
//		}
//	}

//private:
//	std::exception_ptr ex_ptr;
//};




// outerCellCardのFILL先を手繰って複数の個別セルカードを生成し、セルカードのorderをキーにしたmultimapを返す。
// この段階でlike-but, コンプリメント, 同階層のtrclは処理済みであること。
// univMapはuniverseをキーにして所属するセルのカードを保持する。
std::vector<inp::CellCard>
inp::CellCard::getFilledCards(geom::CellCreator *creator,
							   const std::unordered_map<std::string, std::vector<CellCard>> &univMap,
							   const inp::CellCard &outerCellCard,
							   const int maxThreadCount,
							  const int depth,  // この関数は再帰呼び出しされるので、現在の深さを引数に与える。
							  std::atomic_bool *cancelFlag,
							  std::atomic_int *counter  // counterはカード生成したらその時増やす。
							  )
{
	if(cancelFlag->load()) return std::vector<inp::CellCard>();
	const std::unordered_map<size_t, math::Matrix<4>> &trMap =creator->surfaceCreator()->transformationMatrixMap();
	geom::SurfaceMap* surfMap = creator->surfaceCreator()->mapPointer();
	const CellCard &outerCard = outerCellCard;  	// outerCardは FILL=univでfilledされるセルカード
	std::vector<CellCard> retCardVec;
	// outerCellCardが被fillセルでなければそのままの結果を返す。(fill展開しないので1要素ベクトル)
	if(outerCard.parameters.find("fill") == outerCard.parameters.end()
	&& outerCard.parameters.find("*fill") == outerCard.parameters.end()) {
		retCardVec.emplace_back(std::move(outerCard));
		// progressの進捗は再帰深さ1の階層での生成カード数で計っているため、depth == 1以外ではcounterは増やさない。
		mDebug() << "depth========" << depth << "ここには来ないはず？";
		if(depth == 1) ++(*counter);
		return retCardVec;
	}

	// outerCellCardが被fillセルの場合
	// ※ここでは既にLatticeセルは個別の要素セルに展開済みなのでfillパラメータの引数はunivers名となっている
	//mDebug() << "outercard name=" << outerCard.name << ",  params=" << outerCard.parameters;
	std::string universeName = outerCard.parameters.at("fill");
	assert(!universeName.empty());
	std::string trString;
	std::smatch withTrMatch;
	// TRありFILLかどうかを判定
	if(std::regex_search(universeName, withTrMatch, std::regex(R"((\w+) *\(([-+*/%{}()0-9a-zA-Z. ]+)\))"))) {
		trString = withTrMatch.str(2);
		// withTrMatchはuniverseNameへのiteratorを保持しているのでuniverseNameへの変更以後は不正になるので注意
		universeName = withTrMatch.str(1);
	}
	// univMapにfill先のunivが見つからない場合はエラー終了
	// LAT=が指定された居ないカードでFILL=にDimDeclを用いた場合ここのuniverseNameにDimDeclが入ってきてしまうので
	// エラーを返す。
	if(univMap.find(universeName) == univMap.end()) {
        throw std::invalid_argument(outerCard.pos() + " Universe" + universeName + "is used in FILL but not defined.");
	}



	// ここからoutercellをfillで指定されているunivで埋めていく
	// univ数=std::distance(range.first, range.second)は
	// latticeの場合100を超えるのでここは並列化する。
	// univMapの要素ペアのfirstはuniverse名, secondがそのuniverseを構成しているセルのカード



	const std::vector<CellCard> &fillingCellCards = univMap.at(universeName);


	// maxThreadCountが1以下あるいはfillするカードの数がmaxThread数未満なら問答無用でシーケンシャル実行
//	if(maxThreadCount <= 1 || fillingCellCards.size() < static_cast<size_t>(maxThreadCount)) {
		// fillingCell関数は例外を発生させる可能性があるが、getFilledCard関数は再帰的に実行されるため、
		// 何度もcatchされるのを避けるため、ここではcatchしない。
		//mDebug() << "depth===" << depth << "fillingCardsSize===" << fillingCellCards.size();
		fillingCell(creator, maxThreadCount, 0, fillingCellCards.size(), trMap, univMap, outerCard,
					fillingCellCards, trString, depth+1, surfMap, &retCardVec, cancelFlag, counter);
		/*
		 * depth=0は最初のcallでこれが終わった時は充填完了なので、depth=0の時に
		 * カウンターを進めると、正しい値にはなるがprogressとしては使えない。
		 * depth=1での充填セル数をprogressの進捗基準としているので、
		 * depth=1での処理が終わったらカウンタを進める。
		 */
		if(depth == 1) 	{
			//mDebug() << "outer card ===" << outerCard.name << " filled.";
			++(*counter);
		}
		return retCardVec;
//	}

	//assert(!"Debug end.");

	// 但しこの関数をmaxスレッド数で並列化すると、再帰実行時にスレッド数がどんどん増えて
	// 非効率になる可能性があるので、スレッド数は実行時判断する必要がある。
	// → 手動でworker数を管理するしかない。
	// → 単純にworker数をcpu数にすると上の方の層でスレッド数を上限まで使い切って、
	//    一部が終わってもあとはそれをjoin待ちするからスレッドを増やせなくなる。
	//   →なのでcurrentThreadCountは各スレッドが終了次第スレッド数カウント減少させる。
//	static std::mutex mtx;
//	static std::atomic_int currentThreadCount(0);
//	auto range = univMap.equal_range(universeName);


//	int numThread = 1;
//	{
//		std::lock_guard<std::mutex> lk(mtx);
//		// 現在スレッド数がmaxスレッド数の差が1以上の場合並列実行
//		if(maxThreadCount >= currentThreadCount + 1) {
//			numThread = maxThreadCount - currentThreadCount;
//			currentThreadCount += numThread;
//		}
//	}
//	// thread数1ならやはり逐次実行
//	if(numThread <= 1) {
//		try{
//			fillingCell(creator, maxThreadCount, 0, fillingCellCards.size(), trMap, univMap, outerCard,
//						fillingCellCards, trString, surfMap, &retCardVec);
//			return retCardVec;
//		} catch (std::exception &e) {
//			mFatal("While filling universea, exception occured.", e.what());
//		}
//	}

//	// ここから並列処理(並列数numThread)
//	mDebug() << "Filling threads ===" << numThread;
//	std::vector<std::thread> threads;
//	size_t numUniv = fillingCellCards.size();
//	std::vector<std::vector<inp::CellCard>> resultVecs(numThread);
//	assert(numUniv != 0 && numThread > 0);

//	std::vector<std::exception_ptr> ex_ptrs(numThread);
//	std::vector<FillingWorker> workers;
//	for(size_t i = 0; i < static_cast<size_t>(numThread); ++i) {
//		workers.emplace_back(ex_ptrs.at(i));
//	}

//	// filling時にsurfaceを新たに生成するので、排他処理するか、スレッドごとにコピーして最後にマージするか。
//	for(size_t n = 0; n < static_cast<size_t>(numThread); ++n) {
//		size_t sindex = numUniv/numThread*n     + std::min(numUniv%numThread, n);
//		size_t eindex = numUniv/numThread*(n+1) + std::min(numUniv%numThread, n+1);
//		threads.emplace_back(workers.at(n), creator, maxThreadCount, sindex, eindex, trMap, univMap, outerCard,
//							 fillingCellCards, trString, surfMap, &resultVecs.at(n));
//	}
//	for(auto &thread: threads) thread.join();
//	for(auto &worker: workers) {
//		if(!worker.exceptionString().empty()) {
//			mFatal("While filling universesbbbb, exception occured.", worker.exceptionString());
//		}
//	}
//	for(auto &res: resultVecs)	{
//		retCardVec.insert(retCardVec.end(), std::make_move_iterator(res.begin()), std::make_move_iterator(res.end()));
//	}

//	return retCardVec;
}



bool inp::operator <(const inp::CellCard &c1, const inp::CellCard &c2) {
	return c1.name < c2.name;
}

// 論理多項式に含まれているsurface名のセットを返す
std::set<std::string> inp::GetSurfaceNames(std::string str)
{
	// surface名は 「前方に "行頭(:+-空白"」 「後方に "行末):空白"」のある文字列
	std::regex surfBaseNamePattern(R"(([-+ :(]|^)([\d\w]+)([ :)]|$))");

	std::smatch sm;
	std::set<std::string> surfNameSet;
	auto frontIt = str.cbegin();
	while(std::regex_search(frontIt, str.cend(), sm, surfBaseNamePattern)) {
//		for(size_t i = 0; i < sm.size(); ++i) {
//			mDebug() <<"i=" << i << ", str=" << sm.str(i);
//		}
		frontIt = sm[2].second;
		surfNameSet.emplace(sm.str(2));
	}
	return surfNameSet;
}

