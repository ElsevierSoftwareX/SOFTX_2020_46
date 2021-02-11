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
#include "quadric.bs.hpp"

#include <algorithm>
#include <string>
#include "surface.hpp"
#include "plane.hpp"
#include "core/utils/message.hpp"
#include "core/math/constants.hpp"
#include "core/geometry/cell/boundingbox.hpp"

namespace {
using bp_set_type = std::vector<std::vector<geom::Plane>> ;
std::vector<std::vector<geom::Plane>> WHOLESPACE()
{
    using Vec3 = math::Vector<3>;
    static const std::vector<std::vector<geom::Plane>> wholespace {
        std::vector<geom::Plane>{geom::Plane("", Vec3{1, 0, 0}, 0, false)},
        std::vector<geom::Plane>{geom::Plane("", Vec3{-11, 0, 0}, 0, false)},
    };
    return wholespace;
}

void WarnError (const std::string &name, int rk1, int rk2, int s1p)
{
    mWarning() << "Calculation of bounding surface for" << name << "failed by unknown reasons. "
               << "(rank1, rank2, sig11) = (" << rk1 << rk2 << s1p << ")"
               << "No bounding surface is returned.";
}
void WarnEmpty(const std::string &name, int rk1, int rk2, int s1p)
{
  mWarning() << "Surface =" << name << " is empty set."
              << "(rank1, rank2, sig11) = (" << rk1 << rk2 << s1p << ")"
              << "No bounding surface is returned.";
}
void WarnNoPlane(const std::string &name, const std::string &surfaceName, int rk1, int rk2, int s1p)
{
    mWarning() << "Surface =" << name << " is" << surfaceName
                << ", and no bounding plane can be defined."
                << "(rank1, rank2, sig11, sig21) = (" << rk1 << rk2 << s1p << ")"
                << "No bounding surface is returned.";
}
}


/*!
 * \brief CalcBoundingPlaneVectors_rank3 主要部ランクが3の場合の内包面セットを計算する。
 * \param [in] name 面の(ユーザー定義)名前
 * \param [in] rankPrincipal 主要部のランク
 * \param [in] translation 実空間→標準化空間への並進ベクトル
 * \param [in] rotation 実空間→標準化空間への回転行列(対角化行列)
 * \param [in] eigenValues 主要部の固有値
 * \param [in] d 二次形式の行列式/主要部の行列式 比
 * \param [out] boundingPlaneVecs 内包面セット
 * \param [out] bb 単一面から生成されるboundingbox
 * \return
 */
void CalcBoundingPlaneVectors_rank3(const std::string &name_,  //
                                    bool isInside,
                                    int rankMatrix,
                                    const math::Vector<3> &translation,
                                    const math::Matrix<3> &rotation,
                                    std::vector<double> eigenValues,
                                    double d,
                                    std::vector<std::vector<geom::Plane>> &boundingPlaneVecs,
                                    geom::BoundingBox & bb)
{
    using Vec3 = math::Vector<3>;
    using namespace geom;
    auto IsNonzeroPositive = [](double val) {return val > math::EPS;};

    // 標準化空間→実空間へのアフィン変換行列tMatrixを作成する。
    // translationは実空間→標準化空間への並進としているので-1をかける。
    // rotationは列ベクトルに対する標準化空間→実空間の回転変換として作っているので
    // (行ベクトル変換ルーチンである)createAffineMatrixでは逆行列(==転置行列)を与える。
    auto normToRealMatrix = math::createAffineMatrix(rotation.transposed(), -translation);
//        mDebug() << "標 → 実への回転===" << rotation.transposed();
//        mDebug() << "標 → 実への並進===" << translation;


    if(rankMatrix == 4) {  //空集合、楕円、一葉、二葉双曲面
        /*
         * A, A~にランク落ちがないので
         * 二次形式： λx^2 + μy^2 + νz^2 + |~A|/|A| と標準化される。
         * 並進ベクトル： -1*A^-1(l,m,n)
         */
        // rank2 == 4 ならgqMatrixはフルランクなのでd != 0
        // わかりやすいようにdが-1になるように係数を規格化する。
        std::vector<double> coeffA{-1*eigenValues.at(0)/d, -1*eigenValues.at(1)/d, -1*eigenValues.at(2)/d};
        d = -1; // -1*d/d = -1
//        mDebug() << "標準化二次形式="
//                 <<   std::to_string(coeffA.at(0)) + "*x'^2 + "
//                      + std::to_string(coeffA.at(1)) + "*y'^2 + "
//                      + std::to_string(coeffA.at(2)) + "*z'^2 +" << d;
        int numPositiveCoeffA = static_cast<int>(std::count_if(coeffA.cbegin(), coeffA.cend(), IsNonzeroPositive));



        // 以下ではevaluesは降順を前提としているので、dを-1に規格化した後再度sortする必要が有り、
        // かつ回転行列も並べ替える必要が有る。
        // ここでd == -1になるように規格化し、evaluesが降順になるように evalues, rotationを並べ替える。
        std::vector<size_t> indices{0, 1, 2};  // evaluesを降順ソートした時のソート後インデックス
        std::sort(indices.begin(), indices.end(), [&coeffA](size_t i1, size_t i2){return coeffA[i1] > coeffA[i2];});
        std::sort(coeffA.begin(), coeffA.end(), [](double ev1, double ev2){return ev1 > ev2;});
        std::array<math::Vector<3>, 3> colVecs;
        for(size_t i = 0; i < indices.size(); ++i) colVecs.at(i) = rotation.colVector(indices.at(i));
        normToRealMatrix = math::createAffineMatrix(math::Matrix<3>::fromColVectors({colVecs[0], colVecs[1], colVecs[2]}).transposed(), -translation);



        if(numPositiveCoeffA == 3) {  // 楕円面
            // r123はx, y, z方向の半径
            double r1 = 1.0/std::sqrt(coeffA.at(0)), r2 = 1.0/std::sqrt(coeffA.at(1)), r3 = 1.0/std::sqrt(coeffA.at(2));
            if(isInside) {
                // 内側のケース
                bb = geom::BoundingBox(-r1, r1, -r2, r2, -r3, r3);
                std::vector<geom::Plane> planes{
                    Plane("", Vec3{ 1,  0,  0}, -r1),
                    Plane("", Vec3{-1,  0,  0}, -r1),
                    Plane("", Vec3{ 0,  1,  0}, -r2),
                    Plane("", Vec3{ 0, -1,  0}, -r2),
                    Plane("", Vec3{ 0,  0,  1}, -r3),
                    Plane("", Vec3{ 0,  0, -1}, -r3)
                };
                boundingPlaneVecs = std::vector<std::vector<geom::Plane>>{std::move(planes)};
            } else {
                // 外側のケース
                bb = BoundingBox::universalBox();
                // この時はbounding面は内接面で x=a/√3， y=b/√3 ...の時体積最大となる。
                double rt3inv = 1.0/std::sqrt(3.0);
                std::vector<geom::Plane> planes{
                    Plane("", Vec3{ 1,  0,  0}, r1*rt3inv),
                    Plane("", Vec3{-1,  0,  0}, r1*rt3inv),
                    Plane("", Vec3{ 0,  1,  0}, r2*rt3inv),
                    Plane("", Vec3{ 0, -1,  0}, r2*rt3inv),
                    Plane("", Vec3{ 0,  0,  1}, r3*rt3inv),
                    Plane("", Vec3{ 0,  0, -1}, r3*rt3inv)
                };
                boundingPlaneVecs
                        = std::vector<std::vector<geom::Plane>>{
                            std::vector<geom::Plane>{std::move(planes.at(0))},
                            std::vector<geom::Plane>{std::move(planes.at(1))},
                            std::vector<geom::Plane>{std::move(planes.at(2))},
                            std::vector<geom::Plane>{std::move(planes.at(3))},
                            std::vector<geom::Plane>{std::move(planes.at(4))},
                            std::vector<geom::Plane>{std::move(planes.at(5))},
                };
            }
            // Ellipsoid終わり。

        } else if (numPositiveCoeffA == 2) {  // 一葉双曲面
            mDebug() << name_ << "は一葉双曲面";
            double lamb = coeffA.at(0), mu = coeffA.at(1), nu = -1*coeffA.at(2);
            assert(lamb > 0 && mu > 0 && nu > 0);
            mDebug() << "lambda, mu, nu ===" << lamb << mu << nu;
            if(isInside) {
                double cx = 1.0/std::sqrt(lamb);  // xz平面でのz=0でのx座標
                mDebug() << "cx===" << cx;
                double cy = 1.0/std::sqrt(mu);  // yz平面での焦点のy座標
                std::vector<geom::Plane> plusPlanes {
                    Plane("", Vec3{0, 0, 1}, math::Point{0, 0, 0}, false),
                    Plane("", Vec3{ std::sqrt(lamb), 0, std::sqrt(nu)}, math::Point{-cx, 0, 0}, false),
                    Plane("", Vec3{-std::sqrt(lamb), 0, std::sqrt(nu)}, math::Point{ cx, 0, 0}, false),
                    Plane("", Vec3{0,  std::sqrt(mu), std::sqrt(nu)}, math::Point{0, -cy, 0}, false),
                    Plane("", Vec3{0, -std::sqrt(mu), std::sqrt(nu)}, math::Point{0,  cy, 0}, false)
                };
                std::vector<geom::Plane> minusPlanes {
                    Plane("", Vec3{0, 0, -1}, math::Point{0, 0, 0}, false),
                    Plane("", Vec3{-std::sqrt(lamb), 0, -std::sqrt(nu)}, math::Point{cx, 0, 0}, false),
                    Plane("", Vec3{ std::sqrt(lamb), 0, -std::sqrt(nu)}, math::Point{-cx, 0, 0}, false),
                    Plane("", Vec3{0, -std::sqrt(mu), -std::sqrt(nu)}, math::Point{0, cy, 0}, false),
                    Plane("", Vec3{0,  std::sqrt(mu), -std::sqrt(nu)}, math::Point{0, -cy, 0}, false),
                };
                for(const auto &pl: plusPlanes) mDebug() << pl.toString();
                boundingPlaneVecs =  std::vector<std::vector<geom::Plane>>{plusPlanes, minusPlanes};
                // 一葉双曲面内側終わり。
            } else {
                // 外側に対しては z=0での楕円(λx^2 + μy^2 - d = 0)の内接四角形の各面に対応した面を返す。
                double r1 = 1.0/std::sqrt(lamb), r2 = 1.0/std::sqrt(mu);
                std::vector<geom::Plane> planes{
                    Plane("", Vec3{ 1,  0,  0}, r1, false),
                    Plane("", Vec3{-1,  0,  0}, r1, false),
                    Plane("", Vec3{ 0,  1,  0}, r2, false),
                    Plane("", Vec3{ 0, -1,  0}, r2, false)
                };
                boundingPlaneVecs
                        = std::vector<std::vector<geom::Plane>>{
                            std::vector<geom::Plane>{std::move(planes.at(0))},
                            std::vector<geom::Plane>{std::move(planes.at(1))},
                            std::vector<geom::Plane>{std::move(planes.at(2))},
                            std::vector<geom::Plane>{std::move(planes.at(3))},
                };
                // 一葉双曲面外側終わり。
            }
            // 一様双曲面終わり。


        } else if (numPositiveCoeffA == 1) {  // 二葉双曲面
            mDebug() << name_ << "は二葉双曲面, lambda, mu, nu ===" << coeffA;
            double lamb = coeffA.at(0), mu = -1*coeffA.at(1), nu = -1*coeffA.at(2);
//            mDebug() << "eigenvalues ===" << lamb << mu << nu;
            assert(lamb > 0 && mu > 0 && nu > 0);
            double root_lamb = std::sqrt(lamb), root_mu = std::sqrt(mu), root_nu = std::sqrt(nu);
            double zdist = 1.0/std::sqrt(nu);// 双曲面とz=0平面の距離
            if(isInside) {
                // 内側を内包するのは漸近線と法線をz方向に持つ面2個
                std::vector<geom::Plane> plusPlanes{
                    Plane("", Vec3{ 0,  0,  1}, zdist, false),
                    Plane("", Vec3{-root_lamb, 0, root_nu}, math::Point{0, 0, 0}, false),
                    Plane("", Vec3{+root_lamb, 0, root_nu}, math::Point{0, 0, 0}, false),
                    Plane("", Vec3{0, -root_mu, root_nu}, math::Point{0, 0, 0}, false),
                    Plane("", Vec3{0, +root_mu, root_nu}, math::Point{0, 0, 0}, false),
                };
                std::vector<geom::Plane> minusPlanes{
                    Plane("", Vec3{ 0,  0,  -1}, zdist),
                    Plane("", Vec3{-root_lamb, 0, -root_nu}, math::Point{0, 0, 0}, false),
                    Plane("", Vec3{+root_lamb, 0, -root_nu}, math::Point{0, 0, 0}, false),
                    Plane("", Vec3{0, -root_mu, -root_nu}, math::Point{0, 0, 0}, false),
                    Plane("", Vec3{0, +root_mu, -root_nu}, math::Point{0, 0, 0}, false),
                };
                boundingPlaneVecs = std::vector<std::vector<geom::Plane>>{plusPlanes, minusPlanes};
                // 2葉双曲面内側終わり
            } else {
                boundingPlaneVecs = WHOLESPACE();
                // 2葉双曲面を外側を内包?できる平面セットは漸近線を使えば定義できる。一応
                // あまり意味があるとも思えないが(BB計算時間が伸びる割にBBを小さくできるない。)
                // しかも下の内包面セットは間違っている。
//                    double cxz = std::sqrt((lamb+nu)/(lamb*nu)), cyz = std::sqrt((mu+nu)/(mu*nu));
//                    math::Point focus = math::Point{0, 0, std::max(cxz, cyz)};
//                    std::vector<geom::Plane> plusPlanes{
//                        Plane("", Vec3{root_lamb, 0, -root_nu}, focus),
//                        Plane("", Vec3{root_lamb, 0, root_nu}, focus),
//                        Plane("", Vec3{0, root_mu, -root_nu}, focus),
//                        Plane("", Vec3{0, root_mu,  root_nu}, focus)
//                    };
//                    std::vector<geom::Plane> minusPlanes{
//                        Plane("", Vec3{-root_lamb, 0, -root_nu}, -1*focus),
//                        Plane("", Vec3{root_lamb, 0, -root_nu}, -1*focus),
//                        Plane("", Vec3{0,-root_mu, -root_nu}, -1*focus),
//                        Plane("", Vec3{0, root_mu, -root_nu}, -1*focus)
//                    };
//                    boundingPlaneVecs = std::vector<std::vector<geom::Plane>>{plusPlanes, minusPlanes};
                // 二葉双曲面外側終わり
            }
            // 二葉双曲面終わり。

        } else if (numPositiveCoeffA == 0) {  // 空集合
            // 空集合
            WarnEmpty(name_, 3, rankMatrix, numPositiveCoeffA);
        }

    } else if(rankMatrix == 3) {  // 点、楕円錐面
//        mDebug() << "標準化二次形式="
//                 << std::to_string(eigenValues.at(0)) + "*x'^2 + "
//                 << std::to_string(eigenValues.at(1)) + "*y'^2 + "
//                 << std::to_string(eigenValues.at(2)) + "*z'^2 +" << 0;
        int numPositiveValues = static_cast<int>(std::count_if(eigenValues.cbegin(), eigenValues.cend(), IsNonzeroPositive));
        if(numPositiveValues == 2 || numPositiveValues == 1) {
            // numPositiveValuesが1ならばx'の係数がev1 >0、y'z'の係数が負(0 > ev2 > ev3)なので、x'を軸とする楕円錐となっている。
            // 故に軸がz'方向に来てかつx'の係数が最も大きくなるように回転をかけ、最終的に
            // x'の係数が正かつ最大となるようにする
            // 要するにy'軸周り90度回転してx'とz'を入れ替えれば良い。xyzすべての方向で偶関数なので際正負の方向は問題にならない。
            if(numPositiveValues == 1) {
                normToRealMatrix.setRotationMatrix(rotation*math::generateRotationMatrix2(math::Vector<3>{0, 1, 0}, 0.5*math::PI));
                for(auto &evalue: eigenValues) evalue *= -1;
                std::swap(eigenValues.at(0), eigenValues.at(2));
//                mDebug() << "再標準化二次形式="
//                         << std::to_string(eigenValues.at(0)) + "*x'^2 + "
//                         << std::to_string(eigenValues.at(1)) + "*y'^2 + "
//                         << std::to_string(eigenValues.at(2)) + "*z'^2 +" << 0;
            }

            /*
             *  楕円錐面
             * 長軸がx'方向、短軸がy', 円錐軸がz'方以降の楕円錐
             */
            mDebug() << name_ << "は楕円錐面";
            if(isInside) {
                // 内側
                mDebug() << name_ << "は楕円錐面内側";
                double cx = std::sqrt(eigenValues.at(0)), cy =std::sqrt(eigenValues.at(1)), cz = std::sqrt(-eigenValues.at(2));
                math::Vector<3> normalxz1 = (math::Vector<3>{-cx, 0, cz}).normalized();
                math::Vector<3> normalxz2 = (math::Vector<3>{ cx, 0, cz}).normalized();
                math::Vector<3> normalyz1 = (math::Vector<3>{ 0, -cy, cz}).normalized();
                math::Vector<3> normalyz2 = (math::Vector<3>{ 0,  cy, cz}).normalized();
                // +z部分と-z部分
                std::vector<geom::Plane> planesPlus{
                    Plane("", normalxz1, 0, false),
                    Plane("", normalxz2, 0, false),
                    Plane("", normalyz1, 0, false),
                    Plane("", normalyz2, 0, false),
                    Plane("", Vec3{ 0,  0,  1}, 0, false),
                };
                std::vector<geom::Plane> planesMinus{
                    Plane("", -normalxz1, 0, false),
                    Plane("", -normalxz2, 0, false),
                    Plane("", -normalyz1, 0, false),
                    Plane("", -normalyz2, 0, false),
                    Plane("", Vec3{ 0,  0,  -1}, 0, false),
                };
                boundingPlaneVecs = std::vector<std::vector<geom::Plane>>{planesPlus, planesMinus};
            } else {
                // 外側のbouning surfaceはおそらくほぼ意味がないのでとりあえず全空間を内包する平面セットを返す。
                boundingPlaneVecs = WHOLESPACE();
                WarnNoPlane(name_, "inside of elliptic cone(not implemented yet)", 3, rankMatrix, numPositiveValues);
            }
        } else {
            // 空集合というか1点なのでジオメトリ定義には不適切
            WarnEmpty(name_, 3, rankMatrix, numPositiveValues);
        }
    }
    for(auto &planeVector: boundingPlaneVecs) {
        for(auto &plane: planeVector) {
            plane.transform(normToRealMatrix);
        }
    }
    //bb.transform(normToRealMatrix);
}



void CalcBoundingPlaneVectors_rank2(const std::string &name_, bool isInside,
                                    int rankMatrix, const math::Vector<3> &pqr,
                                    const math::Matrix<3> &rotation,
                                    std::vector<double> eigenValues, double d,
                                    std::vector<std::vector<geom::Plane>> &boundingPlaneVecs)
{

    using Matrix4 = math::Matrix<4>;
    using Vec3 = math::Vector<3>;

    auto IsNonzeroPositive = [](double d) {return d > math::EPS;};
    double p = pqr.x(), q = pqr.y(), r = pqr.z();
    auto normToRealMatrix = Matrix4::IDENTITY();
    if(rankMatrix == 4) {  // 楕円 or 双曲放物面
        mDebug() << name_ << "は楕円/双曲放物面";
        /*
         * a1*x^2 + a2*y^2 + 2*r*z = 0 に帰着される。
         */
        math::Vector<3> translation
                = math::Vector<3>{
                            p/eigenValues.at(0),
                            q/eigenValues.at(1),
                            0.5/r*(d - p*p/eigenValues.at(0) - q*q/eigenValues.at(1))
                 }*rotation.transposed();
        normToRealMatrix = math::createAffineMatrix(rotation.transposed(), -translation);
        // 単純のためzの係数を-2に規格化する。(rを-1に規格化する。)
        eigenValues.at(0) = -1*eigenValues.at(0)/r;
        eigenValues.at(1) = -1*eigenValues.at(1)/r;
        auto it1 = eigenValues.cbegin(), it2 = eigenValues.cbegin();
        auto numPositiveValues = static_cast<int>(std::count_if(it1, ++it2, IsNonzeroPositive));
        mDebug() << "eigenValues===" << eigenValues << ", numpositives===" << numPositiveValues;
        if(numPositiveValues == 2) {
            double lamb = eigenValues.at(0), mu = eigenValues.at(1);
            // 楕円放物面
            if(isInside) {
                double root2inv = std::sqrt(2.0)*0.5;  // 1/sqrt(2)
                mDebug() << "楕円放物面内側";
                // 楕円放物面内側を内包する自明なBounding surfaceは原点での接面だけだが、
                // 45度で接する面も追加した
                boundingPlaneVecs = bp_set_type{
                    std::vector<geom::Plane>{
                        geom::Plane("", Vec3{0, 0, 1}, 0, false),
                        geom::Plane("", Vec3{-root2inv, 0, root2inv}, math::Point{ 0.5/lamb, 0, 0.5/lamb}, false),
                        geom::Plane("", Vec3{ root2inv, 0, root2inv}, math::Point{-0.5/lamb, 0, 0.5/lamb}, false),
                        geom::Plane("", Vec3{0, -root2inv, root2inv}, math::Point{0,  0.5/mu, 0.5/mu}, false),
                        geom::Plane("", Vec3{0,  root2inv, root2inv}, math::Point{0, -0.5/mu, 0.5/mu}, false)
                    }
                };
            } else {
                mDebug() << "楕円放物面外側";
                // 楕円放物面外側を定義するBouding面の組み合わせは無限通りにある。
                // 楕円面と違って体積最大などという制約条件はあり得ないというのが難点
                // ゆえに楕円放物面外側のBoundingSurfaceは全空間を内包する面セットとする。
                boundingPlaneVecs = WHOLESPACE();
                WarnNoPlane(name_, "Hyperbolic paraboloid", 2, rankMatrix, numPositiveValues);
            }
        } else if (numPositiveValues == 1) {
            mDebug() << "双曲放物面";
            // 双曲放物面のboundingPlaneは複雑なので、暫定的に内側はなし、外側は全空間をセット
            if(!isInside) boundingPlaneVecs = WHOLESPACE();
            WarnNoPlane(name_, "outside/inside of elliptic paraboloid", 2, rankMatrix, numPositiveValues);
        } else {
            // エラー
            // 数値誤差の関係でここへ来ることはある。
            // FIXME rankとsignatureの計算結果が一貫していない場合。前提条件が満たされないのでなんとかする。
            mDebug() << "その他？";
            boundingPlaneVecs = WHOLESPACE();
            WarnError(name_, 2, rankMatrix, numPositiveValues);
        }

    } else if(rankMatrix == 3) {  // 楕円/双曲柱面";
        // rank(A)=2, rank(~A) = 3
        mDebug() << name_ << "は楕円/双曲柱面";
        // rank1 が2なので固有値は2個。二次の係数も2個
        // a1*x^2 + a2*y^2 + d = 0 型に帰着される
        math::Vector<3> translation
                = math::Vector<3>{
                            p/eigenValues.at(0),
                            q/eigenValues.at(1),
                            0  // z方向には対称かつ無限に続くので並進はしてもしなくても変わらない。
                 }*rotation.transposed(); // 後ろからの作用なのでtransposedさせたものを適用


        // 敢えて言うならここのrotation.transposed()は 本来的にはrotation.inverse().inverse().transposed()
        // 座標の変換と座標上のオブジェクトの移動は逆方向になるのでtranslationにはマイナスがつく
        normToRealMatrix = math::createAffineMatrix(rotation.transposed(), -translation);
        //mDebug() << "標準化空間から実座標系へ戻す行列 ===" << tMatrix;
        // 定数項を-1に規格化する。
        double k = d - p*p/eigenValues.at(0) - q*q/eigenValues.at(1);
        // 見やすいように kが-1になるように規格化
        assert(std::abs(k) > math::EPS);
        std::vector<double> coeff{-1*eigenValues.at(0)/k, -1*eigenValues.at(1)/k};
        k = -1;
        auto numPositiveCoeffs = static_cast<int>(std::count_if(coeff.cbegin(), coeff.cend(), IsNonzeroPositive));
        if(numPositiveCoeffs == 2) {  // 楕円柱
            double r1 = 1.0/std::sqrt(coeff.at(0)), r2 = 1.0/std::sqrt(coeff.at(1));
            const double RT2inv = 1.0/std::sqrt(2.0);
            mDebug() << "楕円柱面";
            if(isInside) {
                // 内側のケース
                std::vector<geom::Plane> planes {
                    geom::Plane("", math::Vector<3>{ 1,  0,  0}, -r1, false),
                    geom::Plane("", math::Vector<3>{-1,  0,  0}, -r1, false),
                    geom::Plane("", math::Vector<3>{ 0,  1,  0}, -r2, false),
                    geom::Plane("", math::Vector<3>{ 0, -1,  0}, -r2, false),
                };
                boundingPlaneVecs = std::vector<std::vector<geom::Plane>>{std::move(planes)};

            } else {
                // 外側のケース
                std::vector<geom::Plane> planes {
                    geom::Plane("", math::Vector<3>{ 1,  0,  0}, r1*RT2inv, false),
                    geom::Plane("", math::Vector<3>{-1,  0,  0}, r1*RT2inv, false),
                    geom::Plane("", math::Vector<3>{ 0,  1,  0}, r2*RT2inv, false),
                    geom::Plane("", math::Vector<3>{ 0, -1,  0}, r2*RT2inv, false),
                };
                boundingPlaneVecs
                        = std::vector<std::vector<geom::Plane>>{
                            std::vector<geom::Plane>{std::move(planes.at(0))},
                            std::vector<geom::Plane>{std::move(planes.at(1))},
                            std::vector<geom::Plane>{std::move(planes.at(2))},
                            std::vector<geom::Plane>{std::move(planes.at(3))}
                        };
            }
        } else if(numPositiveCoeffs == 1) { // 双曲柱
            mDebug() << "双曲柱面(未実装)";
            if(coeff.at(0) < coeff.at(1)) {
                // 現在のcoeffが大きい方がxに来るように回転させる。
                std::swap(coeff.at(0), coeff.at(1));
                normToRealMatrix.setRotationMatrix(normToRealMatrix.rotationMatrix()
                                                   *math::generateRotationMatrix2(Vec3{0, 0, 1}, 0.5*math::PI));
            }
            // lamb, muは正の定数
            double lamb = coeff.at(0), mu = -coeff.at(1);
            assert(lamb > 0 && mu > 0);
            double root_lamb = std::sqrt(lamb), root_mu = std::sqrt(mu);
            math::Point focus1{1.0/root_lamb, 0, 0};
            math::Point focus2 = -focus1;
            if(isInside) {
                boundingPlaneVecs = bp_set_type{
                    std::vector<geom::Plane>{
                        geom::Plane("", Vec3{0, 0, 1}, 0, false),
                        geom::Plane("", Vec3{-root_lamb, root_mu, 0}, focus1, false),
                        geom::Plane("", Vec3{ root_lamb, root_mu, 0}, focus2, false)
                    },
                    std::vector<geom::Plane>{
                        geom::Plane("", Vec3{0, 0, -1}, 0),
                        geom::Plane("", Vec3{-root_lamb, -root_mu, 0}, focus1, false),
                        geom::Plane("", Vec3{ root_lamb, -root_mu, 0}, focus2, false)
                    }
                };
            } else {
                //boundingPlaneVecs = WHOLESPACE();
                boundingPlaneVecs = bp_set_type{
                    std::vector<geom::Plane>{
                        geom::Plane("", Vec3{0, 0, -1}, 0, false),
                        geom::Plane("", Vec3{+root_lamb, -root_mu, 0}, 0, false),
                        geom::Plane("", Vec3{-root_lamb, -root_mu, 0}, 0, false)
                    },
                    std::vector<geom::Plane>{
                        geom::Plane("", Vec3{0, 0, 1}, 0),
                        geom::Plane("", Vec3{+root_lamb, +root_mu, 0}, 0, false),
                        geom::Plane("", Vec3{-root_lamb, +root_mu, 0}, 0, false)
                    }
                };
            }

        } else { // "空集合" 空集合の内側内包面は空集合、外側内包面は全空間
            WarnEmpty(name_, 2, rankMatrix, numPositiveCoeffs);
            if(!isInside) boundingPlaneVecs = WHOLESPACE();
        }

    } else if(rankMatrix == 2) {
        // 一直線、交わる２平面
        // λ、μが同符号なら直線、そうでなければ交わる2面
        if(eigenValues.at(0)*eigenValues.at(1) > 0) {
            mDebug() << name_ << "は直線";
            WarnNoPlane(name_, "a line", 2, rankMatrix, 0);
            if(!isInside) boundingPlaneVecs = WHOLESPACE();
        } else {
            // y = ±sqrt(λ/μ)x
            mDebug() << name_ << "は交わる2平面";
            double c = std::sqrt(-1*eigenValues.at(0)/eigenValues.at(1));
            if(isInside) {
                boundingPlaneVecs = std::vector<std::vector<geom::Plane>> {
                    std::vector<geom::Plane>{
                        geom::Plane("", Vec3{c, -1, 0}, 0, false),
                        geom::Plane("", Vec3{c,  1, 0}, 0, false),
                        geom::Plane("", Vec3{1,  0, 0}, 0, false)
                    },
                    std::vector<geom::Plane>{
                        geom::Plane("", Vec3{-c, 1, 0}, 0, false),
                        geom::Plane("", Vec3{-c, -1, 0}, 0, false),
                        geom::Plane("", Vec3{-1,  0, 0}, 0, false)
                    }
                };
            } else {
                boundingPlaneVecs = std::vector<std::vector<geom::Plane>> {
                    std::vector<geom::Plane>{
                        geom::Plane("", Vec3{-c, 1, 0}, 0, false),
                        geom::Plane("", Vec3{c,  1, 0}, 0, false),
                        geom::Plane("", Vec3{0,  0, 1}, 0, false)
                    },
                    std::vector<geom::Plane>{
                        geom::Plane("", Vec3{c, -1, 0}, 0, false),
                        geom::Plane("", Vec3{-c, -1, 0}, 0, false),
                        geom::Plane("", Vec3{0,  0, -1}, 0, false)
                    }
                };
            }
        }
    } else {
        WarnEmpty(name_, 2, rankMatrix, 0);
    }




    for(auto &planeVector: boundingPlaneVecs) {
        for(auto &plane: planeVector) {
            plane.transform(normToRealMatrix);
        }
    }
    //bb.transform(normToRealMatrix);
    // rank(A)=2終わり
}




void CalcBoundingPlaneVectors_rank1(const std::string &name_,
                                    bool isInside,
                                    int rankMatrix,
                                    math::Vector<3> pqr,
                                    math::Matrix<3> rotation,
                                    std::vector<double> eigenValues,
                                    double d,
                                    std::vector<std::vector<geom::Plane>> &boundingPlaneVecs)
{
    using Vec3 = math::Vector<3>;
    double p = pqr.x(), q = pqr.y(), r = pqr.z();
    auto normToRealMatrix = math::Matrix<4>::IDENTITY();
    if(rankMatrix == 3) {
        double lamb = eigenValues.at(0);
        mDebug() << name_ << "は放物柱面";
        // 放物柱面.ここではpqrのqかrはゼロ。qがゼロになるように適当に変換する。
        if(std::abs(q) > std::abs(r)) {
            // pqrのqrとrotation行列の2列3列を交換する。固有値は元々どちらもゼロ
            std::swap(q, r);
            auto col1 = rotation.colVector(0);
            auto col2 = rotation.colVector(1);
            auto col3 = rotation.colVector(2);
            rotation = math::Matrix<3>::fromColVectors({col1, col3, col2});
        }
        Vec3 translation = Vec3{p/lamb, 0, 0.5*r*(d-p*p/lamb)};
        normToRealMatrix = math::createAffineMatrix(rotation.transposed(), translation);

        // TODO 放物面に接する面としてとりあえず原点での接面を返すが、もう少し接面は増やしても良い。
        double zfwd_dir = (-lamb/r > 0) ? 1 : -1;
        if(isInside) {
            boundingPlaneVecs = std::vector<std::vector<geom::Plane>>{
                std::vector<geom::Plane>{geom::Plane("", Vec3{0, 0, zfwd_dir}, 0)}
            };
        } else {
            boundingPlaneVecs = std::vector<std::vector<geom::Plane>>{
                std::vector<geom::Plane>{geom::Plane("", Vec3{0, 0, -zfwd_dir}, 0)}
            };
        }

    } else if (rankMatrix == 2) { // 平行２平面、空集合
        double k = d - p*p/eigenValues.at(0); // 定数項
        assert(std::abs(k) >  0);
        // 定数項をマイナス１に規格化してから符号を確定させる。
        double lamb = -1*eigenValues.at(0)/k;
        normToRealMatrix = math::createAffineMatrix(rotation.transposed(), Vec3{p/lamb, 0, 0});
        k = -1;
        // ここで陰関数は λx'^2 -1 = 0となっている。のでλの符号で場合分け
        if(lamb > 0) { // 平行2平面
            mDebug() << name_ << "は並行２平面";
            // x = ±1/λ
            auto xpvec = Vec3{1, 0, 0}, xmvec = Vec3{-1, 0, 0};
            double xpos = 1.0/lamb;
            if(isInside) {
                boundingPlaneVecs = std::vector<std::vector<geom::Plane>> {
                        std::vector<geom::Plane>{geom::Plane("", xmvec, -xpos), geom::Plane("", xpvec, -xpos)}
                };
            } else {
                boundingPlaneVecs = std::vector<std::vector<geom::Plane>> {
                        std::vector<geom::Plane>{geom::Plane("", xpvec, xpos)},
                        std::vector<geom::Plane>{geom::Plane("", xmvec, xpos)}
                };
            }
        } else {  // 空集合
            mDebug() << name_ << "は空集合";
            WarnEmpty(name_, 1, rankMatrix, 0);
            if(!isInside) boundingPlaneVecs = WHOLESPACE();
        }

    } else if(rankMatrix == 1) { // １平面
        mDebug() << name_ << "は1平面";
        normToRealMatrix = math::createAffineMatrix(rotation.transposed(), Vec3{0, 0, 0});
        auto normToRealMatrix = math::createAffineMatrix(rotation.transposed(), Vec3{0, 0, 0});
        auto normal = isInside ? Vec3{-1, 0, 0} : Vec3{1, 0, 0};
        auto plane = geom::Plane("", normal, 0);
        plane.transform(normToRealMatrix);
        boundingPlaneVecs = std::vector<std::vector<geom::Plane>>{std::vector<geom::Plane>{plane}};
    }

    // ここでboundingplanevecsの要素をtransform
    for(auto &planeVector: boundingPlaneVecs) {
        for(auto &plane: planeVector) {
            plane.transform(normToRealMatrix);
        }
    }
}
