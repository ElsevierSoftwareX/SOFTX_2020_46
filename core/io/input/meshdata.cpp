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
#include "meshdata.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <map>


#include "core/formula/fortran/fortnode.hpp"
#include "core/io/input/phits/phits_metacards.hpp"
#include "core/io/input/dataline.hpp"
#include "core/utils/utils.hpp"
#include "core/utils/numeric_utils.hpp"
#include "core/source/phits/phitssource.hpp"

namespace {
	const std::map<inp::MeshData::PhitsType, std::string> &getPhitsTypeMap()
    {
		static const std::map<inp::MeshData::PhitsType, std::string> phitsTypeMap{
			{inp::MeshData::PhitsType::TYPE1, "1"},
			{inp::MeshData::PhitsType::TYPE2, "2"},
			{inp::MeshData::PhitsType::TYPE3, "3"},
			{inp::MeshData::PhitsType::TYPE4, "4"},
			{inp::MeshData::PhitsType::TYPE5, "5"},
			{inp::MeshData::PhitsType::TYPE1m, "-1"},
			{inp::MeshData::PhitsType::TYPE2m, "-2"},

        };
        return phitsTypeMap;
    }

	const std::map<inp::MeshData::KIND, std::string> &getKindMap()
	{
		static const std::map<inp::MeshData::KIND, std::string> kindMap{
			{inp::MeshData::KIND::A, "a"},
			{inp::MeshData::KIND::E, "e"},
			{inp::MeshData::KIND::R, "r"},
			{inp::MeshData::KIND::X, "x"},
			{inp::MeshData::KIND::Y, "y"},
			{inp::MeshData::KIND::Z, "z"}
		};
		return kindMap;
	}
}

std::string inp::MeshData::typeString(inp::MeshData::PhitsType meshType)
{
    return getPhitsTypeMap().at(meshType);
}

inp::MeshData::PhitsType inp::MeshData::strToType(const std::string &str)
{
    for(auto &enumPair: getPhitsTypeMap()) {
        if(enumPair.second == str) return enumPair.first;
    }
    throw std::invalid_argument(str + " is invalid phits mesh type string.");
}

std::string inp::MeshData::kindString(inp::MeshData::KIND kind)
{
	return getKindMap().at(kind);
}

inp::MeshData::KIND inp::MeshData::strToKind(const std::string &str)
{
	for(auto &kpair: getKindMap()) {
		if(kpair.second == str) return kpair.first;
	}
	throw std::out_of_range("string=" + str + " is not a valid mesh kind str");
}





// inputの位置itからnum個のデータを読み取りvaluesに格納する。
void ReadMeshEntries(inp::MeshData::KIND kind,
					const std::list<inp::DataLine> &input,
                     std::list<inp::DataLine>::const_iterator &it,
                     std::vector<double> *values)
{
	auto startLine = *it;
	auto kindStr = inp::MeshData::kindString(kind);
	auto ngstr = "n" + kindStr;
	// 最初に群数を決定する。
	auto numStrs = utils::splitString(std::make_pair('{', '}'), " ", inp::phits::GetParameterValue<std::string>(ngstr, input, it).second, true);
	assert(!numStrs.empty());
	auto num = utils::stringTo<size_t>(numStrs.at(0));

	std::vector<double> tmpVec;
	// ne=n 以降の部分にデータが入っている場合はそれもデータとして読み取る。
	for(size_t i = 1 ; i < numStrs.size(); ++i) {
		tmpVec.emplace_back(fort::eq(numStrs.at(i)));
	}
	// ne=xx と同じ行にあるデータだけで数が足りなければ次の行を読み取る。
	if(tmpVec.size() < num ) {
		// 群数を読み取ったので1行進めてから再度データ読み取り
		while(++it, it != input.end()) {
			//mDebug() << "it=" << it->pos() << it->data;
			for(auto &str: utils::splitString(std::make_pair('{', '}'), " ", it->data, true)) {
				tmpVec.emplace_back(fort::eq(str));
			}
			if(tmpVec.size() >= num+1) break;
		}
	}

	//mDebug() << "after break it=" << it->pos() << it->data;

	// 読み込んだデータ数が指定の数をオーバーしたら、これは入力数が多すぎると言える。
	if (tmpVec.size() > num + 1) {
        throw std::invalid_argument("Too much entries(in a line).");
    } else if(it == input.end()) {
        // ここではitは最後に読んだDataLineを指しているはずなのでend()担っている場合はエラー
        throw std::out_of_range("Too few entries");
    }

    // 次の行は　◯＝◯式のパラメータ定義行でなければ入力が多すぎるということ。
    ++it;
    std::smatch sm;
	if(it != input.end() && !std::regex_search(it->data, sm, inp::phits::parameterPattern())) {
        throw std::invalid_argument("To much mesh entries");
    }

    *values = tmpVec;
}

// inputの位置itから定義されている、kind種類の最大最小値をrangeに、群数をnumGroupに格納する
void ReadRange(inp::MeshData::KIND kind,
               const std::list<inp::DataLine> &input,
               std::list<inp::DataLine>::const_iterator &it,
			   std::pair<double, double> *range, int *numGroup)
{
	auto startLine = *it;
	auto kindStr = inp::MeshData::kindString(kind);
	auto minstr = kindStr + "min";
	auto maxstr = kindStr + "max";
	auto ngstr = "n" + kindStr;
    std::smatch sm;
    std::unordered_map<std::string, std::string> parameterMap;
	auto pattern = inp::phits::parameterPattern();

	// 群数、最大、最小の3データ読み取り
	try {
		for(size_t i = 0; i < 3; ++i) {
			if(it == input.end()) {
				throw std::out_of_range(std::string("Unexpected end of data whie reading mesh min/max"));
			}
			if(std::regex_search(it->data, sm, pattern)) {
				auto nameValuePair =  inp::phits::getParameterPair(it->data);
				std::string paramName = nameValuePair.first;
				std::string paramValue = nameValuePair.second;
				parameterMap[paramName] = paramValue;
			}
			++it;
		}

		if(parameterMap.find(minstr) == parameterMap.end()) {
			throw std::invalid_argument(minstr + " is absent.");
		} else if (parameterMap.find(maxstr) == parameterMap.end()) {
			throw std::invalid_argument(maxstr + " is absent.");
		} else if (parameterMap.find(ngstr) == parameterMap.end()) {
			throw std::invalid_argument(ngstr + " is absent.");
		}
		*numGroup = utils::stringTo<int>(parameterMap.at(ngstr));
		range->first = fort::eq(parameterMap.at(minstr));
		range->second = fort::eq(parameterMap.at(maxstr));
	} catch (std::exception &e) {
        throw std::invalid_argument(std::string("In mesh subsection started from ") + startLine.pos() + " " + e.what());
	}
}


// itの行がすでにn-typeの後の行を指しているとする。
inp::MeshData inp::MeshData::fromPhitsInput(std::list<inp::DataLine>::const_iterator &it,
											const std::list<inp::DataLine> &input,
											KIND kind, const std::string &typeStr)
{
	PhitsType ptype = strToType(typeStr);
	// 角度メッシュ
	if(kind == KIND::A) {
		switch (ptype) {
		case PhitsType::TYPE1:
		case PhitsType::TYPE1m:
			{
			std::vector<double> values;
			ReadMeshEntries(kind, input, it, &values);
			if(ptype == PhitsType::TYPE1) {
				for(auto &value: values) {
					value = std::acos(value);
				}
			}
			return MeshData(values);
			}
			break;
		case PhitsType::TYPE2:
		case PhitsType::TYPE2m:
			{
			std::pair<double, double> range;
			int na;
			ReadRange(kind, input, it, &range, &na);
			if(ptype == PhitsType::TYPE2) {
				range.first = std::acos(range.first);
				range.second = std::acos(range.second);
			}
			MeshData retmesh (na, range, InterPolate::LIN);
			return retmesh;
			}
			break;
		default:
			throw std::invalid_argument("Only +-1/2 are acceptable for a-type=");
		}
	// 角度メッシュ以外の場合
	} else {
	// その他角度以外
		switch (ptype) {
		case PhitsType::TYPE1:
		case PhitsType::TYPE1m:
			{
			std::vector<double> values;
			ReadMeshEntries(kind, input, it, &values);
			return MeshData(values);
			}
			break;
		case PhitsType::TYPE2:
		case PhitsType::TYPE3:
			{
			std::pair<double, double> range;
	//		auto startLine = it;
			int ng;
			ReadRange(kind, input, it, &range, &ng);
	//		try {
				MeshData retmesh (ng, range, (strToType(typeStr) == PhitsType::TYPE2) ? InterPolate::LIN : InterPolate::LOG);
				return retmesh;
	//		} catch(std::invalid_argument &e) {
	//			std::cerr << "Error: " << startLine->pos() << " " << e.what() << std::endl;
	//			std::exit(EXIT_FAILURE);
	//		}
			}
			break;
		default:
			std::cerr << "Sorry, Type 4/5 mesh are not implemented yet." << std::endl;
			std::exit(EXIT_FAILURE);
		}
		// input中のnum個の自由形式入力値を位置itの行からvalueに読み取る。
	}
	abort();
}

// Phits入力からメッシュを作る。1つのセクションに同タイプのメッシュがあることはないので
// 最初に出てきた該当メッシュを作成する。
inp::MeshData inp::MeshData::fromPhitsInput(const std::list<inp::DataLine> &input,
											inp::MeshData::KIND kind)
{
    auto kindstr = kindString(kind);
	std::regex typeSettingPattern(kindstr + "-type *= *(-*[1-5]+)");
    auto it = input.begin();
    std::string typeStr;
    std::smatch sm;
    while(it != input.end()) {
        if(std::regex_search(it->data, sm, typeSettingPattern)) {
            typeStr = sm.str(1);
			break;
        }
        ++it;
    }
	++it;  // *-typeの行から1行進める
    if(typeStr.empty()) {
        throw std::invalid_argument(std::string("No mesh type spcifier, \"")
                                    + kindstr + "-type =\", found.");
    }

	// ここからタイプ別。但し角度メッシュはマイナス符号時の扱いが他の場合と異なるので分ける必要がる。
	PhitsType ptype = strToType(typeStr);
	// 角度メッシュ
	if(kind == KIND::A) {
		switch (ptype) {
		case PhitsType::TYPE1:
		case PhitsType::TYPE1m:
			{
			std::vector<double> values;
			ReadMeshEntries(kind, input, it, &values);
			if(ptype == PhitsType::TYPE1) {
				for(auto &value: values) {
					value = std::acos(value);
				}
			}
			return MeshData(values);
			}
			break;
		case PhitsType::TYPE2:
		case PhitsType::TYPE2m:
			{
			std::pair<double, double> range;
			int na;
			ReadRange(kind, input, it, &range, &na);
			if(ptype == PhitsType::TYPE2) {
				range.first = std::acos(range.first);
				range.second = std::acos(range.second);
			}
			MeshData retmesh (na, range, InterPolate::LIN);
			return retmesh;
			}
			break;
		default:
			throw std::invalid_argument("Only +-1/2 are acceptable for a-type=");
		}
	// 角度メッシュ以外の場合
	} else {
	// その他角度以外
		switch (ptype) {
		case PhitsType::TYPE1:
		case PhitsType::TYPE1m:
			{
			std::vector<double> values;
			ReadMeshEntries(kind, input, it, &values);
			return MeshData(values);
			}
			break;
		case PhitsType::TYPE2:
		case PhitsType::TYPE3:
			{
			std::pair<double, double> range;
	//		auto startLine = it;
			int ng;
			ReadRange(kind, input, it, &range, &ng);
	//		try {
				MeshData retmesh (ng, range, (strToType(typeStr) == PhitsType::TYPE2) ? InterPolate::LIN : InterPolate::LOG);
				return retmesh;
	//		} catch(std::invalid_argument &e) {
	//			std::cerr << "Error: " << startLine->pos() << " " << e.what() << std::endl;
	//			std::exit(EXIT_FAILURE);
	//		}
			}
			break;
		default:
			std::cerr << "Sorry, Type 4/5 mesh are not implemented yet." << std::endl;
			std::exit(EXIT_FAILURE);
		}
		// input中のnum個の自由形式入力値を位置itの行からvalueに読み取る。
	}
	abort();
}


/*
 * 離散値用のメッシュ
 * values[i]をメッシュ中心とした不等間隔上下非対称メッシュを生成する。
 */
inp::MeshData inp::MeshData::fromDiscreteValues(const std::vector<double> &values)
{
	if(values.size() == 1) {
		MeshData singleMesh({values.front()-1.0, values.front()+1.0});
		singleMesh.centers_ = values;
		return singleMesh;
	}
	std::vector<double> tmpBounds;
	for(size_t i = 0; i < values.size()-1; ++i) {
		tmpBounds.emplace_back((values.at(i) + values.at(i+1))*0.5);
	}
	tmpBounds.emplace_back(values.back() + (values.back() - tmpBounds.back()));

	std::sort(tmpBounds.begin(), tmpBounds.end());
	std::vector<double>firstComponent {tmpBounds.front()-1}; // 離散点からメッシュを作るときに最下限値は最小値で作る。
	tmpBounds = utils::makeConcatVector(firstComponent, tmpBounds);
	MeshData retMesh(tmpBounds);
	retMesh.centers_ = values;
	return retMesh;
}

inp::MeshData::MeshData(const std::vector<double> &vec, InterPolate ipmode)
	:bounds_(vec)
{
    if(bounds_.size() <= 1) {
        throw std::invalid_argument("Number of bounds shoud be more than 1.");
    }
    if(!utils::isAscendant(bounds_, false)) {
		throw std::invalid_argument("Mesh bounds should be ascendant order.");
    }
	for(size_t i = 0; i < bounds_.size() - 1; ++i) {
		centers_.emplace_back(center(bounds_.at(i), bounds_.at(i+1), ipmode));
	}
}

inp::MeshData::MeshData(const std::initializer_list<double> &dlist, InterPolate ipmode)
	:MeshData(std::vector<double>(dlist), ipmode)
{;}

inp::MeshData::MeshData(size_t num,
                        const std::pair<double, double> &range,
						inp::MeshData::InterPolate interpolate)
{
	// パラメータチェック
    if(range.second < range.first) {
        throw std::invalid_argument("Lower bound is larger than upper bound.");
    } else if(num == 0) {
        throw std::invalid_argument("Number of mesh should be larger than 1.");
    }

	// 下限と上限が等しい場合は要素が1の場合のみOKとする。
	if(utils::isSameDouble(range.first, range.second)) {
		if(num != 1) {
			throw std::invalid_argument("Lower and upper bounds are the same, though num != 1");
		} else {
			//上限下限等しい場合は離散値から作成
			*this = MeshData::fromDiscreteValues(std::vector<double>{range.first});
			return;
		}
	}

	std::vector<double> boundsVec(num+1);
    switch(interpolate) {
    case InterPolate::LIN:
		for(size_t i = 0; i < boundsVec.size(); ++i) {
			boundsVec.at(i) = range.first + ((range.second - range.first)/num)*i;
        }
        break;
    case InterPolate::LOG:
        if(range.first <= 0 || range.second <= 0) {
            throw std::invalid_argument("Log-mesh bounds should be positive.");
        }
		for(size_t i = 0; i < boundsVec.size(); ++i) {
            double delta = std::log(range.second/range.first)/num;
			boundsVec.at(i) = std::exp(std::log(range.first) + delta*i);
			//mDebug() << "i=" << i <<  ",range=" << range.first << range.second << ", bounds=" << boundsVec.at(i);
        }
        break;
	default:
		std::cerr << "ProgramError: Only Lin/Log is acceptable." << std::endl;
		std::exit(EXIT_FAILURE);
    }

	*this = MeshData(boundsVec, interpolate);

}


std::vector<std::pair<double, double>> inp::MeshData::widthPairs() const
{
	std::vector<std::pair<double, double>> retPairs(bounds_.size() - 1);

	for(size_t i = 0; i < retPairs.size(); ++i) {
		retPairs.at(i).first = centers_.at(i) - bounds_.at(i);  // メッシュ下側幅
		retPairs.at(i).second = bounds_.at(i+1) - centers_.at(i); // メッシュ上側幅
		assert(retPairs.at(i).first >= 0);
		assert(retPairs.at(i).second >= 0);
    }

	return retPairs;
}

std::string inp::MeshData::toString() const
{
	std::stringstream ss;
	ss << "bounds";
	::operator<<(ss, bounds());
	ss << "centers";
	::operator<<(ss, centers());
	return ss.str();
}




double inp::MeshData::center(double v1, double v2, inp::MeshData::InterPolate ipMode)
{
    switch (ipMode) {
    case InterPolate::LIN:
        return 0.5*(v1 + v2);
        break;
    case InterPolate::LOG:
        if(v1 < 0 || v2 < 0) {
            throw std::invalid_argument("Interpolation in log requires positive bounds.");
        }
		return std::exp(0.5*(std::log(v1*v2)));
		break;
    default:
		std::cerr << "ProgramError: Interpolation should be lin/log." << std::endl;
        std::exit(EXIT_FAILURE);
        break;
    }
}
