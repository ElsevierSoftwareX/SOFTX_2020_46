#ifndef FISSIONNEUTRONDATA_HPP
#define FISSIONNEUTRONDATA_HPP


#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class FissionNeutronData {
public:
	FissionNeutronData(){;}
	// 現在のifstream読み込み位置に核分裂中性子データがあるとしてtableを読み込む
	FissionNeutronData(std::ifstream& ifs);
	// xss配列と核分裂中性子データの先頭位置(fortranスタイル)を与えてtableを読み込む
	FissionNeutronData(const std::vector<std::string>& xss, const std::size_t &index);

	int type() const {
		if(!coefficients.empty()) {
			return 1;  // LNU=1 type
		} else if (!epoints.empty()) {
			return 2;  // LNU=2 type
		} else {
			return 0;  // not set
		}
	}

	bool empty() const {
		return (coefficients.empty() && epoints.empty());
	}

	void check() const {
		if(!coefficients.empty() && !epoints.empty()) {
			std::cerr << "Error! Coefficients and epoints are set at the same time." << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}

protected:
	// LNU=1型 coefficient
	std::vector<double> coefficients;
	// LNU=2型 table
	std::vector<double> interpolateParameterNBT;
	std::vector<double> interpolateParameterINT;
	std::vector<double> epoints;
	std::vector<double> numberOfNeutrons;
};

class PrecursorData {
public:
	PrecursorData(){;}
	PrecursorData(const std::vector<std::string>& xss, const std::size_t &index);
	std::vector<double> epoints() const {return epoints_;}
	std::vector<double> nbt() const {return interpolationNBT_;}
private:
	double decayConstant_;
	std::vector<double> interpolationNBT_;
	std::vector<double> interpolationINT_;
	std::vector<double> epoints_;
	std::vector<double> probabilities_;

};

//class DelayedNeutronData: public FissionNeutronData {
//public:
//	DelayedNeutronData(){;}
//	DelayedNeutronData(const std::vector<std::string>& xss, const std::size_t &index);

//private:
//	// 遅発中性子はdecayconstantとprobabilityが追加される。
//	std::vector<double> probabilities;
//	std::vector<double> decayConstants;

//};


#endif // FISSIONNEUTRONDATA_HPP
