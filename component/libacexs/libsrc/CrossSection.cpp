/*
 * CrossSection.cpp
 *
 *  Created on: 2010/01/22
 *      Author: sohnishi
 */

#include "acefile.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <sstream>


#include "aceutils.hpp"
#include "utils/utils_conv.hpp"
#include "utils/string_util.hpp"
#include "mt.hpp"

using namespace std;

namespace
{
static const std::string HEADER_STR ="#! ";
static const std::string COMM_STR="# ";
static const std::string SPACE="  ";
}

ace::CrossSection::CrossSection(const std::vector<double> &ep,
                                const std::vector<double> &xs,
                                const ace::Reaction &react,
                                const int &r_n,
                                const double &qv,
                                const long &eoff,
                                const int &af)
    : epoints(ep), xs_value(xs), mt(react),release_n(r_n), Qval(qv), e_offset(eoff), angular_flag(af)
{
	if(!std::is_sorted(ep.begin(), ep.end())) {
		throw std::invalid_argument("Energy points are not ascendant.");
	}
	angular_dists.clear();
}

void ace::CrossSection::dump()
{
	string ofname = ace::mt::toMtString( this->mt )+".dat";
	ofstream ost( ofname.c_str() );
	dump(ost);
}



void ace::CrossSection::dump(std::ostream &ost)
{
	assert( epoints.size() == xs_value.size() );
	ost << HEADER_STR << "MT="                 << ace::mt::toNumber( this->mt )  << endl;
	ost << HEADER_STR << "Q="                  << this->Qval << endl;
	ost << HEADER_STR << "offset="             << this->e_offset << endl;
	ost << HEADER_STR << "angular_dist_flag="  << this->angular_flag << endl;
	ost << COMM_STR   << ace::mt::description(this->mt)    << endl;
	ost << COMM_STR   << "Energy        Value" << endl;


	ost.setf(std::ios_base::scientific);
	for( unsigned int i=0; i<this->epoints.size(); i++ )	{
		ost << std::left << std::setprecision(7) << setw(14) << epoints.at(i)
			 << std::right << std::setprecision(8) << setw(16) << this->xs_value.at(i) <<endl;
	}

	// 角度分布のある反応の核データ出力
	if( this->angular_flag !=0 )
	{
		string ofname = ace::mt::toMtString( this->mt )+".angle.dat";
		ofstream ofs(ofname.c_str());
		ofs << "# Energy  angular_point   pdf   cdf" << endl;
		for(std::size_t i=0; i<this->angular_dists.size(); i++){
			AngularDistribution adist = this->angular_dists.at(i);

			ofs.setf(std::ios_base::scientific);
			for(std::size_t j=0; j<adist.angular_points.size(); j++){
				ofs << std::left << std::setprecision(5) << std::setw(13) << adist.energy
					<< std::right << std::setprecision(5) << std::setw(12) << adist.angular_points.at(j)
					<< std::setw(14) << std::setprecision(7) << adist.pdf.at(j)
					<< std::setw(14) << std::setprecision(7) << adist.cdf.at(j) << endl;
			}
			ofs<<endl;
		}
	}
}

// energy を与えて 対応する断面積値を返す。
double ace::CrossSection::getValue(double energy) const
{
	if(epoints.empty()) return 0;  // 断面積が読み込まれていない場合は0を返す
	if(energy < epoints.front() || energy >= epoints.back())  {
		throw std::out_of_range(std::string("Energy = ") + std::to_string(energy)
		                        + " is out of xs table. min,max = ("
		                        + std::to_string(epoints.front()) + ", " + std::to_string(epoints.back())
		                        + ")");
	}

	//指定された要素以上(指定値を含む)の値が現れる最初の位置のイテレータを取得する
	auto lb = std::lower_bound(epoints.begin(), epoints.end(), energy);

	// epoints[eindex-1]  <= energy < epoints[eindex] なるindex
	std::size_t eindex = std::distance(epoints.begin(), lb);


	// LOGLOG補間して返す。
	// もっともJENDL4-photoatomic-fileではconsistancyを考えるなら各反応のloglog補間を足し合わせろ、とある。
	double w = (energy - epoints[eindex-1])/(epoints[eindex] - epoints[eindex-1]);
//	std::cout << "energy=" << energy << ", eindex=" << eindex
//	          << ", epoint[eindex] =" << epoints[eindex] << ", ep[eindex-1] =" << epoints[eindex-1] << std::endl;
//	std::cout << "w=" << w << ", f1=" << std::pow(xs_value[eindex-1], 1-w) << " f2=" << std::pow(xs_value[eindex], w)<< std::endl;
	return std::pow(xs_value[eindex-1], 1-w) * std::pow(xs_value[eindex], w);

//	return ((epoints[eindex] - energy)*xs_value[eindex-1] + (energy - epoints[eindex-1])*xs_value[eindex])
//	        /(epoints[eindex] - epoints[eindex-1]);
}

std::vector<ace::AngularDistribution> ReadAngularTable(const std::vector<string> &xss, const size_t andBlockPos, const size_t angularArrayPos)
{
	//dDebug() << "jxs7=" << andBlockPos << "angularArraypos=" << angularArrayPos;
	// andBlockPOS = jxs(7), angularArrayPos = LOCB
	auto findex = andBlockPos + angularArrayPos -1;
	auto cindex = findex - 1; // fortran index -1  -> c index
	// AND blockの最初のエントリはエネルギー分点数
	long int num_epoints = compat::stoli(xss.at(cindex));
	// 次にエネルギー分点データ
	std::vector<double> angular_epoints = compat::GetPartialVec<double>(xss, cindex + 1, num_epoints);
	// 断面積データの位置。JSX(9)からの相対指定。データの数等に矛盾がなければそのまま読んでいけば正しい値になる。
	// TODO_low エネルギー分点の並びが単調増加/減少でなければこの扱いは間違いになる。
	std::vector<long int> locations = compat::GetPartialVec<long int>(xss, cindex + 1 + num_epoints, num_epoints);
	//dDebug() << "epoints=" << num_epoints << "\n angularEpoints=\n" << angular_epoints << "locations=\n" << locations;
	/*
	 * location > 0 等確率32ビン
	 * location < 0 table
	 * location = 0 等方
	 */

	// elasticの場合 index = JXS(9)+LOCB1 −1
	std::vector<ace::AngularDistribution> retvec;
	// TODO_xs location > 0 の等確率ビンデータと =0の等方分布読み取り
	for(std::size_t i = 0; i < angular_epoints.size(); i++) {
		auto refPos = andBlockPos + std::abs(locations.at(i)) - 1 - 1;  // cindex化するため JXS(7)+ LC(J)-1からさらに1引く
		double energy = angular_epoints.at(i);
		long int interpolation = compat::stoli(xss.at(refPos));
		long int num_apoints   = compat::stoli(xss.at(refPos + 1));
		std::vector<double> apoints = compat::GetPartialVec<double>(xss, refPos + 2, num_apoints);
		std::vector<double> pdf     = compat::GetPartialVec<double>(xss, refPos + 2 + num_apoints, num_apoints);
		std::vector<double> cdf     = compat::GetPartialVec<double>(xss, refPos + 2 + num_apoints*2,  num_apoints);
		retvec.emplace_back(ace::AngularDistribution(energy, interpolation, apoints, pdf, cdf));
		//dDebug() << "interpolate=" << interpolation << "numApoints=" << num_apoints;
	}
	return retvec;
}
