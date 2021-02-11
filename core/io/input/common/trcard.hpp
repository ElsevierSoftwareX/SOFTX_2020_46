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
#ifndef TRCARD_HPP
#define TRCARD_HPP

#include <fstream>
#include <string>
#include <unordered_map>
#include <regex>
#include "core/math/nmatrix.hpp"
#include "datacard.hpp"


namespace inp {

class DataLine;

namespace comm {

class TrCard  : public DataCard
{
public:
    static const std::regex &regex();
    static std::unordered_map<size_t, math::Matrix<4>> makeTransformMap(const std::list<DataLine> &inputLines);

    TrCard(const std::string &str, bool warnCompat = false);
    virtual ~TrCard(){}
    int idNumber() const {return std::atoi(id_.c_str());}              // TR4 の4を返す
    math::Matrix<4> trMatrix() const {return matrix_;}
    virtual std::string toString() const override;

private:
    math::Matrix<4> matrix_;  // 4x4行列
};


std::ostream& operator << (std::ostream& os, const TrCard& trc);


}  // end namespace comm
}  // end namespace inp


#endif // TRCARD_HPP
