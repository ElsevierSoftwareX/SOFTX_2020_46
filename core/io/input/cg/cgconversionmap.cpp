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
#include "cgconversionmap.hpp"

#include <cmath>
#include <stdexcept>
#include <sstream>
#include <vector>

#include "core/utils/matrix_utils.hpp"
#include "core/math/constants.hpp"
#include "core/math/nmatrix.hpp"

namespace {
// cgconv_functype にパラメータ数チェック用の引数(第一引数)を追加した型。
// この第一引数にbindで数値をあててconvfunc_typeのマップへ代入する。
using convproto_type = std::function<std::string(size_t, int, const std::string&, const std::vector<double>&)>;


void checkNumParams(size_t numParams, int id, const std::string mnemonic, const std::vector<double> &params)
{
    // QAD-CGでは固定長読み取りのせいで、パラメータに余計な０が複数ついている場合がある。
    // 何個０を削除するかCG依存なので、ここではパラメータが少なすぎないかだけをチェックする。
    if(numParams > params.size()) {
        std::stringstream sse;
        sse << "CG body id = " << id << ", number of parameters for itype = " << mnemonic << " should be "
            << numParams << ". actual = " <<  params.size();
        throw std::invalid_argument(sse.str());
    }
}

// FIXME 未テスト
// MARS流の回転角指定からMCNPのTR文字列を作成する
// 第一引数から、z軸周り、 x軸周り、y軸周りの順に回転させる。
// 単位は度で、SCINFUL CGのマニュアルによると、右手系で時計回りが正
std::string marsDegreeToTrString(double degree1, double degree2, double degree3)
{
    auto rad1 = math::toRadians(degree1), rad2 = math::toRadians(degree2), rad3 = math::toRadians(degree3);
    // MARSのCGでは右手系が採用されているが、回転角度は右ねじの逆になっているので回転角は負として扱う。
    math::Matrix<3> rotMatrix = math::Matrix<3>::IDENTITY();
    rotMatrix = rotMatrix*math::generateRotationMatrix2(math::Vector<3>{0, 0, 1}, -rad1);
    rotMatrix = rotMatrix*math::generateRotationMatrix2(math::Vector<3>{1, 0, 0}, -rad2);
    rotMatrix = rotMatrix*math::generateRotationMatrix2(math::Vector<3>{0, 1, 0}, -rad3);
    auto affineMatrix = math::Matrix<4>::IDENTITY();
    affineMatrix.setRotationMatrix(rotMatrix);
    return utils::toTrclString(affineMatrix);
}

const std::unordered_map<std::string, size_t> &getNumArgs()
{
    static std::unordered_map<std::string, size_t> numMap {
        {"arb", 30}, {"box", 12}, {"bpp", 9}, {"c", 7}, {"cx", 4}, {"cy", 4},
        {"cz", 4}, {"ell", 7}, {"gel", 12}, {"p", 4}, {"ps", 7}, {"px", 1},
        {"py", 1}, {"pz", 1}, {"qua", 10}, {"rcc", 7}, {"rec", 12}, {"rpp", 6},
        {"sph", 4}, {"tor", 9}, {"trc", 8}, {"wed", 12}, {"wpp", 15},
    };

    return numMap;
}

std::unordered_map<std::string, inp::cgconv_functype> initCgConvFuncMap() {
    namespace sp = std::placeholders;
    std::unordered_map<std::string, inp::cgconv_functype> fmap;
    // MCNP互換のCGはそのまま出力するだけ。
    convproto_type directDump = [](size_t num, int id, const std::string &mnemonic, const std::vector<double>& params){
        checkNumParams(num, id, mnemonic, params);
        std::stringstream ss;
        ss << id << "  " << mnemonic << "  ";
        for(size_t i = 0; i < num; ++i) {
            ss << params.at(i);
            if(i != params.size()-1) ss << "  ";
        }
        return ss.str();
    };
	// MCNPのマクロボディと互換性のある(或いは互換性が無いけど拡張した)CGリスト
    std::vector<std::string> compatCgList{"arb", "box", "cx", "cy", "cz",
                                          "ell", "p", "px", "py", "pz",
                                          "rec", "rcc", "rpp", "sph",
										  "trc", "wed", "qua", "tor"};
    // 互換性のあるCGは引数チェックのみでそのままダンプする。
    for(const auto &cg: compatCgList) {
        fmap[cg] = std::bind(directDump, getNumArgs().at(cg), sp::_1, sp::_2, sp::_3);
    }


    // C はgxsview独自拡張CAと同じ
    fmap["c"] = [](int id, const std::string &mnemonic, const std::vector<double>& params){
        const size_t minNumParams = getNumArgs().at("c");
        checkNumParams(minNumParams, id, mnemonic, params);
        std::stringstream ss;
        ss << id << "  ca ";
        for(const auto &p: params) ss << " " << p;
        return ss.str();
    };

    // PSはP+並進移動と解釈
    fmap["ps"] = [](int id, const std::string &mnemonic, const std::vector<double>& params){
        const size_t minNumParams = getNumArgs().at("ps");
        checkNumParams(minNumParams, id, mnemonic, params);
        std::stringstream ss;
        ss << id << "  p ";
        math::Vector<3> displacement{params.at(0), params.at(1), params.at(2)};
        auto normal = (math::Vector<3>{params.at(3), params.at(4), params.at(5)}).normalized();
        double dist = params.at(6) + math::dotProd(normal, displacement);
        ss << normal.x() << " " << normal.y() << " " << normal.z() << "  " << dist;
        return ss.str();
    };

    // ここから非互換マクロボディ
    fmap["bpp"] = [](int id, const std::string &mnemonic, const std::vector<double>& params){
        const size_t minNumParams = getNumArgs().at("bpp");
        checkNumParams(minNumParams, id, mnemonic, params);
        std::stringstream ss;
        ss << id << "  rpp  ";
        for(size_t i = 0; i < minNumParams - 3; ++i) ss << params.at(i) << " ";
        // bpp はRPP+TRなのでTRSF拡張で処理する。
        ss << " trsf=" << marsDegreeToTrString(params.at(minNumParams - 3),
                                               params.at(minNumParams - 2),
                                               params.at(minNumParams - 1));
        return ss.str();
    };
    // WPPについては9引数をどう解釈するかわからないので後回し。scinful-cgの誤植ではないか
    // とりあえずWED + 回転角３つと解釈する
    fmap["wpp"] = [](int id, const std::string &mnemonic, const std::vector<double>& params){
        const size_t minNumParams = getNumArgs().at("wpp");
        checkNumParams(minNumParams, id, mnemonic, params);
        std::stringstream ss;
        ss << id << "  wed  ";
        for(size_t i = 0; i < minNumParams - 3; ++i) ss << params.at(i) << " ";
        // wpp はwed+TRなのでTRSF拡張で処理する。
        ss << " trsf=" << marsDegreeToTrString(params.at(minNumParams - 3),
                                               params.at(minNumParams - 2),
                                               params.at(minNumParams - 1));
        return ss.str();
    };

    // 一般楕円体は回転楕円体(ELL)では表現できないのでSQ+TRFSで表現する
    fmap["gel"] = [](int id, const std::string &mnemonic, const std::vector<double>& params){
        using Vec = math::Vector<3>;
        const size_t minNumParams = getNumArgs().at("gel");
        checkNumParams(minNumParams, id, mnemonic, params);
        Vec center{params.at(0), params.at(1), params.at(2)};
        Vec r1{params.at(3), params.at(4), params.at(5)};
        Vec r2{params.at(6), params.at(7), params.at(8)};
        Vec r3{params.at(9), params.at(10), params.at(11)};
        /*
         * 3半径ベクトルが直交していない場合は最初のベクトルから順に
         * gram-schimidtで直交化する。
         */
        if(!math::isOrthogonal(r1, r2) || !math::isOrthogonal(r2, r3)) {
            mWarning() << "In GEL cg body, radius vectors are not orthogonal"
                       << "vectors = " << r1 << r2 << r3;
            math::orthogonalize({&r1, &r2, &r3}, 10);
            mWarning() << "Orthogonalized = " << r1 << r2 << r3;
        }

        /*
         *  回転行列の作成
         * 軸並行一般楕円体を任意の位置の楕円体へアフィン変換する行列を作成する。
         * 1. x軸を第一半径ベクトルへ回転させる行列を作成
         * 2. 第一半径ベクトルを軸として、y'軸(上記回転後のy軸)を第二半径ベクトルへ回転させる行列を作成
         * 3.　これらを合成
         */
		auto rot1 = generateRotationMatrix1(r1, math::Vector<3>{1, 0, 0});
        auto yaxis = (math::Vector<3>{0, 1, 0}*rot1).normalized();
		auto yradians = std::acos(math::dotProd(r2.normalized(), yaxis));
        auto rot2 = generateRotationMatrix2(r1, -yradians);  // FIXME 回転方向はあっているか未テスト
        auto affineMatrix = math::Matrix<4>::IDENTITY();
        affineMatrix.setRotationMatrix(math::Matrix<3>::IDENTITY()*rot1*rot2);
        affineMatrix.setTranslationVector(center);
        double a = r1.abs(), b = r2.abs(), c = r3.abs();

        std::stringstream ss;
		ss << id << "  sq " << b*b*c*c << " " << a*a*c*c << " " << a*a*b*b << " "
           << 0 << " " << 0 << " " << 0 << " " << -a*a*b*b*c*c << " " << 0 << " " << 0 << " " << 0;
		ss << " TRSF=(" << utils::toTrclString(affineMatrix) <<")";
        // FIXME BBもここで計算しておかないと実用的ではないかもしれない。その時はBBのTRも実装する必要がある。
        return ss.str();
    };

    /*
     * QUA
     * QUAはMCNPマクロボディと互換性がない上に、複数平面から構成される。
     * 従ってこの時点でマクロボディではなく通常の平面複数に展開する必要がある。
     * しかし、それをするとセルカード(相当部分)のほうでも展開が必要となる。
     * なので、qadモードの方ではなく、本体のマクロボディ展開部分に実装する。
     */


    return fmap;
}


}  // end anonymous namaspace




const std::unordered_map<std::string, inp::cgconv_functype> &inp::getConvFuncMap() {
    static std::unordered_map<std::string, cgconv_functype> funcMap = initCgConvFuncMap();
    return funcMap;
}
