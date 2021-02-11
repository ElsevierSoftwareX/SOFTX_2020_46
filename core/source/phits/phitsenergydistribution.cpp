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
#include "phitsenergydistribution.hpp"

#include <functional>

#include "phitsdistribution.hpp"


#include "core/math/constants.hpp"
#include "core/physics/physconstants.hpp"
#include "core/utils/utils.hpp"
#include "core/io/input/dataline.hpp"
#include "core/io/input/phits/phits_metacards.hpp"
#include "core/source/abstractdistribution.hpp"
#include "core/source/discretedistribution.hpp"
#include "core/source/multigroupdistribution.hpp"
#include "core/source/proportionaldistribution.hpp"





namespace {
const int DEFAULT_TYPE2_NM = 200;
const int DEFAULT_TYPE3_NM = -200;
const int DEFAULT_TYPE5_NM = -200;


// distribution作成関数のタイプ 返り値がDistributionへのスマポで、
// 引数が文字列、DataLineリスト、 DataLineリストのiterator
typedef  std::function<std::unique_ptr<src::Distribution>(const std::list<inp::DataLine> &,
														  std::list<inp::DataLine>::const_iterator&)>
		  creation_function_type;

creation_function_type getEnergyDistributionCreator(const std::string &typeStr)
{
	static std::unordered_map<std::string, creation_function_type> creatorMap {
		{"1", src::phits::EnergyMultiGroupCreator("1")},
		{"4", src::phits::EnergyMultiGroupCreator("4")},
		{"21", src::phits::EnergyMultiGroupCreator("21")},
		{"24",src::phits::EnergyMultiGroupCreator("24")},
		{"11", src::phits::EnergyMultiGroupCreator("1", true)},
		{"14", src::phits::EnergyMultiGroupCreator("4", true)},
		{"31", src::phits::EnergyMultiGroupCreator("21", true)},
		{"34",src::phits::EnergyMultiGroupCreator("24", true)},

		{"8", src::phits::EnergyDiscreteCreator("8")},
		{"9", src::phits::EnergyDiscreteCreator("9")},
		{"18", src::phits::EnergyDiscreteCreator("8", true)},
		{"19", src::phits::EnergyDiscreteCreator("9", true)},

		{"5", src::phits::EnergyUserDefinedCreator("5")},
		{"6", src::phits::EnergyUserDefinedCreator("6")},
		{"15", src::phits::EnergyUserDefinedCreator("5", true)},
		{"16", src::phits::EnergyUserDefinedCreator("6", true)},

		{"3", src::phits::EnergyMaxwellCreator("3")},
		{"7", src::phits::EnergyMaxwellCreator("7")},
		{"13", src::phits::EnergyMaxwellCreator("3", true)},
		{"17", src::phits::EnergyMaxwellCreator("7", true)},

		{"2", src::phits::EnergyGaussianCreator("2")},
		{"12", src::phits::EnergyGaussianCreator("2", true)}
	};
	return creatorMap.at(typeStr);
}

}  // end anonymous namespace


// e-type=... が読み取られたら、e_typeと次の行からのsourceInputを渡されて、エネルギー分布を作成する。
std::unique_ptr<src::Distribution>
	src::phits::createPhitsEnergyDistribution(const std::string &e_type,
										  const std::list<inp::DataLine> &sourceInput,
										  std::list<inp::DataLine>::const_iterator &it)
{
	/*
	 * エネルギー分布の定義文は割と書式が色々あるので面倒。
	 * ・e-typeの次にneが来るとは限らない
	 *		・e-type=2:ガウス分布なのでE群なし
	 *		・e-type=3or7(Maxwell),  5or6(任意関数)はnmで群数指定
	 * ・しかし群数が決まらないとE分布入力データの終わりが確定しない。
	 *
	 * 方針：
	 * ・とりあえずデータの終わりを確定させる。
	 * ・そのためには(データ終わり確定のための)type別処理を導入する。
	 *   …ならそもそもそのtype別処理の中でデータも処理してしまえばいいのでは？
	 * ・となるのでもう頭からtype別処理を導入して逐次処理してデータ終わりに到達したらreturnする。
	 *   type間で共通する処理は意外と少ないので言うほど非効率でもない。というか他に効率的な方法がない。
	 *
	 * 醜いが仕方ない
	 */
	std::string etype = e_type;
	utils::trim(&etype);
	// etype 1,4 は多群入力。入力数は2*ne+1
	return getEnergyDistributionCreator(etype)(sourceInput, it);
}


std::unique_ptr<src::Distribution>
src::phits::EnergyMultiGroupCreator::operator()(const std::list<inp::DataLine> &sourceInput,
												std::list<inp::DataLine>::const_iterator &it)
{
	std::vector<double> evec, pvec;
	//  1, 4, 21, 24 の場合、次の行に必ずn*が来るので、まず群数を読み込む
	int numPoints = inp::phits::GetParameterValue<int>("ne", sourceInput, it).second;
	// 群数を読み込んだので1行進める
	++it;

	src::Distribution::ReadMultiGroupData(numPoints, sourceInput, it, &evec, &pvec);
	if(isWaveLength_) for(auto &ene: evec) ene = phys::nAngToMeV(ene);


	// 内部表現はpdfなので郡幅で規格化する。type 21, 24は元々pdf入力なので規格化しない
	if(etype_ == "1" || etype_ == "4") {
		for(size_t i = 0; i < pvec.size(); ++i) {
			pvec[i] = pvec.at(i)/(evec.at(i+1) - evec.at(i));
		}
	}
	return std::unique_ptr<src::MultigroupDistribution>(new src::MultigroupDistribution(evec, pvec));
}

std::unique_ptr<src::Distribution>
src::phits::EnergyDiscreteCreator::operator()(const std::list<inp::DataLine> &sourceInput,
											  std::list<inp::DataLine>::const_iterator &it)
{
	std::vector<double> evec, pvec;
	int numPoints = inp::phits::GetParameterValue<int>("ne", sourceInput, it).second;
	++it;

	bool hasReadEnergyData = false;
	while(!hasReadEnergyData) {
		auto dataVec = utils::splitString(" ", it->data, true);
		while(!dataVec.empty()) {
			try {
				double etemp = utils::stringTo<double>(dataVec.front());
				if(isWaveLength_) etemp = phys::nAngToMeV(etemp);
				dataVec.erase(dataVec.begin());
				double ptemp = utils::stringTo<double>(dataVec.front());
				dataVec.erase(dataVec.begin());
				if(etemp < 0 || ptemp < 0) {
					throw std::invalid_argument("Energy and probability should be positive.");
				}
				evec.emplace_back(etemp);
				pvec.emplace_back(ptemp);
				// 所定の数読み込んだらbreakする。
				if(static_cast<int>(evec.size()) == numPoints && static_cast<int>(pvec.size()) == numPoints) {
					if(!dataVec.empty()) {  // E群最上限まで読んでデータが残ってたらおかしいという話
						mWarning(it->pos()) << "Too much input data in energy mesh type 8/9. Residual=" << dataVec;
					}
					hasReadEnergyData = true;
					break;
				}
			} catch (std::exception & e) {
                throw std::invalid_argument(it->pos() + " " + e.what());
			}
		}
		++it;
	}
	return std::unique_ptr<DiscreteDistribution>(new DiscreteDistribution(evec, pvec));
}



std::unique_ptr<src::Distribution>
src::phits::EnergyUserDefinedCreator::operator()(const std::list<inp::DataLine> &sourceInput,
												 std::list<inp::DataLine>::const_iterator &it)
{
	using StrVec = std::vector<std::string>;
	// e-type=5の場合e-type定義行を除いて3行か4行
	auto paramsMap = src::phits::readParamsMap(StrVec{"eg1", "eg2", "f(x)"}, StrVec{"nm"}, sourceInput, it);

	int numPoints = (paramsMap.find("nm") == paramsMap.end()) ? DEFAULT_TYPE5_NM
															  : utils::stringTo<int>(paramsMap.at("nm"));
	double emin = utils::stringTo<double>(paramsMap.at("eg1"));
	double emax = utils::stringTo<double>(paramsMap.at("eg2"));
	if(isWaveLength_) {
		emin = phys::nAngToMeV(emin);
		emax = phys::nAngToMeV(emax);
	}
	std::vector<double> epoints = src::phits::getPoints(emin, emax, numPoints);
	// ここでE分点データとその点での確率が得られた。しかしphitsではEと確率のペアでの分布定義を採用せず、
	// 群上限・下限と確率の入力しかサポートしていない。よって台形積分でp, Eを作成する。
	std::string funcStr = paramsMap.at("f(x)");
	std::vector<double> pvec = src::phits::getProbabilityGroup(epoints, funcStr);
	return std::unique_ptr<MultigroupDistribution>(new MultigroupDistribution(epoints, pvec));
}

std::unique_ptr<src::Distribution>
src::phits::EnergyMaxwellCreator::operator()(const std::list<inp::DataLine> &sourceInput,
											 std::list<inp::DataLine>::const_iterator &it)
{
	using StrVec = std::vector<std::string>;
	auto paramsMap = readParamsMap(StrVec{"et0", "et1", "et2"}, StrVec{"nm"}, sourceInput, it);
	int numPoints = (paramsMap.find("nm") == paramsMap.end()) ? DEFAULT_TYPE2_NM
															  : utils::stringTo<int>(paramsMap.at("nm"));
	std::string etemp = paramsMap.at("et0");
	double emin = utils::stringTo<double>(paramsMap.at("et1"));
	double emax = utils::stringTo<double>(paramsMap.at("et2"));
	if(isWaveLength_) {
		double etempv = phys::nAngToMeV(utils::stringTo<double>(etemp));
		etemp = utils::toString(etempv, 15);
		emin = phys::nAngToMeV(emin);
		emax = phys::nAngToMeV(emax);
	}
	std::vector<double> epoints = src::phits::getPoints(emin, emax, numPoints);
	std::string funcStr = "x**1.5*exp(-x/" + etemp + ")";
	std::vector<double> pvec = getProbabilityGroup(epoints, funcStr);
	return std::unique_ptr<MultigroupDistribution>(new MultigroupDistribution(epoints, pvec));
}

std::unique_ptr<src::Distribution>
src::phits::EnergyGaussianCreator::operator()(const std::list<inp::DataLine> &sourceInput,
											  std::list<inp::DataLine>::const_iterator &it)
{
	using StrVec = std::vector<std::string>;
	auto paramsMap = src::phits::readParamsMap(StrVec{"eg0", "eg1", "eg2", "eg3"}, StrVec{"nm"}, sourceInput, it);
	int numPoints = (paramsMap.find("nm") == paramsMap.end()) ? DEFAULT_TYPE2_NM
															  : utils::stringTo<int>(paramsMap.at("nm"));
	std::string ec = paramsMap.at("eg0");  // center
	std::string eh = paramsMap.at("eg1");  // 半値半幅
	double emin = utils::stringTo<double>(paramsMap.at("eg2"));
	double emax = utils::stringTo<double>(paramsMap.at("eg3"));
	if(isWaveLength_) {
		double ecv = phys::nAngToMeV(utils::stringTo<double>(ec));
		double ehv = phys::nAngToMeV(utils::stringTo<double>(eh));
		ec = utils::toString(ecv, 15);
		eh = utils::toString(ehv, 15);
		emin = phys::nAngToMeV(emin);
		emax = phys::nAngToMeV(emax);
	}

	std::vector<double> epoints = getPoints(emin, emax, numPoints);
	// NOTE eg1は半値半幅として扱っている。正しいか？ TODO 未テスト
	std::string funcStr = eh + "*sqrt(log(2)/acos(-1.0))*exp(-1*(1/" + eh +"**2)(x-" + ec +")**2)";
	std::vector<double> pvec = getProbabilityGroup(epoints, funcStr);
	return std::unique_ptr<MultigroupDistribution>(new MultigroupDistribution(epoints, pvec));
}

src::phits::EnergyDistributionCreator::EnergyDistributionCreator(const std::string &etype, bool isLen):etype_(etype), isWaveLength_(isLen){}

src::phits::EnergyDistributionCreator::~EnergyDistributionCreator(){}
