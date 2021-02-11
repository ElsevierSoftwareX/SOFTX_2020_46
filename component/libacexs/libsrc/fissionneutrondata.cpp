#include "fissionneutrondata.hpp"

#include <iostream>
#include <cstdlib>

#include "aceutils.hpp"
#include "utils/utils_conv.hpp"
#include "utils/string_util.hpp"


FissionNeutronData::FissionNeutronData(std::ifstream& ifs) {
	using namespace ace;
	const int lnu = getData<int>(ifs);
	if(lnu == 1) {
		const int NC = getData<int>(ifs);
		coefficients = getData<double>(NC, ifs);
	} else if(lnu == 2){
		const int NR = getData<int>(ifs);
		interpolateParameterNBT = getData<double>(NR, ifs);
		interpolateParameterINT = getData<double>(NR, ifs);
		const int NE = getData<int>(ifs); // number of Energies.
		epoints = getData<double>(NE, ifs);
		numberOfNeutrons = getData<double>(NE, ifs);
	} else {
		std::cerr << "Invalid LNU type = " << lnu << std::endl;
		std::exit(EXIT_FAILURE);
	}
	check();
}

FissionNeutronData::FissionNeutronData(const std::vector<std::string> &xss, const std::size_t& index)
{
	// ここのindexはfortranスタイルなので配列アクセス用インデックスは-1する必要がある。
	std::size_t cindex = index - 1;
	const int LNU = compat::stoi(xss.at(cindex));
	if(LNU == 1) {
		const int NC = compat::stoi(xss.at(cindex + 1));
		// [cindex+2, cindex+2+NC-1]をコピーしたい。
		// std::copyを使っても良いが、結局string→double変換を間に入れないと意味がないので
		// .at()を使ってシーケンシャルに実行する。
		coefficients = compat::GetPartialVec<double>(xss, cindex + 2, NC);
	} else if (LNU == 2) {
		const int NR = compat::stoi(xss.at(cindex + 1));
		interpolateParameterNBT = compat::GetPartialVec<double>(xss, cindex + 2, NR);
		interpolateParameterINT = compat::GetPartialVec<double>(xss, cindex + 2 + NR, NR);
		const int NE = compat::stoi(xss.at(cindex + 2 + NR*2));
		epoints = compat::GetPartialVec<double>(xss, cindex + 2 + NR*2 + 1, NE);
		numberOfNeutrons = compat::GetPartialVec<double>(xss, cindex + 2 + NR*2 + 1 + NE, NE);
		//dDebug() << "NR=" << NR << "NE=" << NE;
	} else {
		std::cerr << "Invalid LNU type = " << LNU << std::endl;
		std::exit(EXIT_FAILURE);
	}
	check();
}

//DelayedNeutronData::DelayedNeutronData(const std::vector<std::string> &xss, const std::size_t &index):
//	FissionNeutronData(xss, index)
//{
//	auto cindex = index - 1;
//	probabilities = numberOfNeutrons;
//	numberOfNeutrons.clear();
//	auto decayStartIndex = cindex + 3 + interpolateParameterNBT.size()*2 + epoints.size()*2;
//	decayConstants = compat::GetPartialVec<double>(xss, decayStartIndex, epoints.size());
//}

PrecursorData::PrecursorData(const std::vector<std::string> &xss, const std::size_t &index)
{
	auto cindex = index - 1;
	decayConstant_ = compat::stoi(xss.at(cindex));
	auto NR = compat::stoi(xss.at(cindex + 1));
	interpolationNBT_ = compat::GetPartialVec<double>(xss, cindex + 2, NR);
	interpolationINT_ = compat::GetPartialVec<double>(xss, cindex + 2 + NR, NR);
	auto NE = compat::stoi(xss.at(cindex + 2 + NR*2));
	epoints_ = compat::GetPartialVec<double>(xss, cindex + 2 + NR*2 + 1, NE);
	probabilities_ = compat::GetPartialVec<double>(xss, cindex + 2 + NR*2 + 1 + NE, NE);

//	dDebug() << epoints_;
//	dDebug() << probabilities_;
}
