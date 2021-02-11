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
#include "bb_utils.hpp"

#include "core/geometry/surface/surfacemap.hpp"
#include "core/geometry/surface/plane.hpp"
#include "core/geometry/cell/boundingbox.hpp"


/*
 *  boundingSurfaceのベクトルベクトルを作成する。
 *  vec<vec<plane>> の要素vec<plane>同士は論理OR連結されているものとし、
 * さらにその要素planeは論理AND連結されていると解釈される。
 *
 *  vvp.at(i).at(j)
 *  の同じiに所属する面同士はAND連結で
 *  vvpの要素同士はOR連結とする。
 *
 *
 */
std::vector<std::vector<geom::Plane>> geom::bb::boundingSurfaces(const lg::LogicalExpression<int> &poly,
                                                           const std::atomic_bool *timeoutFlag,
                                                           const geom::SurfaceMap &smap)
{
//    mDebug() << "ENter polynomial::boundingsurfaces timeout=" << timeoutFlag->load() << "eq=" << poly.toString();
    if(timeoutFlag && timeoutFlag->load()) throw std::runtime_error("Polynomial::boundingSurfaces timeout");
    if(poly.empty()) throw std::invalid_argument("In Polynomial::boundingSurfaces(), polynomial term is empty");

    std::vector<std::vector<geom::Plane>> planeVectors;
    if(!poly.factors().empty()) {  // 因子facが存在する場合and連結する。
        for(const auto &fac: poly.factors()) {
            // PlaneセットをAndでマージする場合
            // ここで場合の数で要素数が爆発する危険性に注意。
            //	mDebug() << "surface=" << smap.at(fac)->name();
            //	for(auto &plvec: smap.at(fac)->boundingPlanes()) {
            //		mDebug() << "plane set";
            //		for(auto pl: plvec) {
            //			mDebug()<< "planes=" << pl.toString();
            //		}
            //	}
            const std::shared_ptr<const geom::Surface> & surf = smap.at(fac);
            planeVectors = geom::BoundingBox::mergePlaneVectorsAnd(timeoutFlag, planeVectors, surf->boundingPlaneVectors());
        }
        // ここで再帰は終端される。
    } else if(!poly.factorPolys().empty()) {
        // factorPoly間はAND関係nなのでmaergeAndする。
        for(const auto &fpoly: poly.factorPolys()) {
            planeVectors = geom::BoundingBox::mergePlaneVectorsAnd(timeoutFlag, planeVectors,
                                                                   boundingSurfaces(fpoly, timeoutFlag, smap));
        }
    } else {
        // 論理多項式の項が2個以上の場合 OR で連結するのでマージではなく単なるstd::vector::insertする。
        planeVectors = boundingSurfaces(poly.terms().front(), timeoutFlag, smap);
        for(size_t i = 1; i < poly.terms().size(); ++i) {
            auto tmpVec = boundingSurfaces(poly.terms().at(i), timeoutFlag, smap);
            planeVectors.insert(planeVectors.cend(), tmpVec.cbegin(), tmpVec.cend());
        }
    }
    return planeVectors;
}

///*
// * mergePlaneVectorsANDでも面のAND演算する時に、表面の場合Bounging面が大量に生じて
// * で式を展開すると計算量が爆発するため、
// * (boundingSurfacesベクトルの要素が1個の)マルチピースを生じない面のAND因子の部分だけで
// * BoundingPlaneをまとめてBBを計算し、 あとはBB間演算子で組み合わせる。
// *
// * 1因子1BBで計算するよりは計算可能性が改善する。
// */
//// この関数はcreateBoundingBox2とほぼ同一なので共通化した。


/*
 * mergePlaneVectorsOrで式を展開すると計算量が爆発するため、
 * AND因子の部分だけでBoundingPlaneをまとめてBBを計算し、あとは演算子で組み合わせる。
 * 1因子1BBで計算するよりは計算可能性が改善する。
 *  第四引数がfalseならセルのマルチピースを生むような部分は無視して計算する。
 */
geom::BoundingBox geom::bb::createBoundingBox2(const lg::LogicalExpression<int> &poly,
                                         const geom::SurfaceMap &smap,
                                         std::atomic_bool *stopFlag,
                                         bool acceptMultiPiece)
{
    // 論理多項式ツリーを辿りながらBoundingBoxを作成する。
    if(poly.empty()) throw std::invalid_argument("In createBoundingBox2, polynomial term is empty");


    // 複数因子からなる論理式の場合、これらの複数因子の面を集めてBBを計算する。
    if(!poly.factors().empty()) {
        // factors()内は全てAND連結なのでこれらの面を集めてBB::fromPlanesを実行する。
        std::vector<std::vector<geom::Plane>> planeVectors;
        for(const auto& fac: poly.factors()) {
            auto pVecs = smap.at(fac)->boundingPlaneVectors();
            // 第4引数がtrueか、pVecsのサイズが1(=マルチピースを産まない面の集合)の場合planeVectorsに追加
            if(acceptMultiPiece || pVecs.size() == 1) {
                planeVectors = geom::BoundingBox::mergePlaneVectorsAnd(stopFlag, planeVectors, pVecs);
            }
        }
        // 再帰関数が終端されるのはここだけ。
        //mDebug() << "Creating BB from planeVectors num vecs===" << planeVectors.size();
        auto retBB = geom::BoundingBox::fromPlanes(stopFlag, planeVectors);
        //・mDebug() << "result===" << retBB.toInputString();
        return retBB;
    } else if(!poly.factorPolys().empty()) {
        geom::BoundingBox bbox = geom::BoundingBox::universalBox();
        for(const auto &fpoly: poly.factorPolys()) {
            geom::BoundingBox rhs = createBoundingBox2(fpoly, smap, stopFlag, acceptMultiPiece);
            bbox = geom::BoundingBox::AND(bbox, rhs);
        }
        return bbox;
    } else {
        geom::BoundingBox bbox = createBoundingBox2(poly.terms().at(0), smap, stopFlag, acceptMultiPiece);
        // 論理多項式の項が2個以上の場合 BoundingBoxの和を取る。
        for(size_t i = 1; i < poly.terms().size(); ++i) {
            bbox = geom::BoundingBox::OR(bbox, createBoundingBox2(poly.terms().at(i), smap, stopFlag, acceptMultiPiece));
        }
        return bbox;
    }
}
/*
 * 1つの面から1つのBBを生成させ、その論理演算で全体のBBを計算している。
 */
geom::BoundingBox geom::bb::createBoundingBox(const lg::LogicalExpression<int> &poly, const geom::SurfaceMap &smap)
{
    // 論理多項式ツリーを辿りながらBoundingBoxを作成する。
    if(poly.empty()) throw std::invalid_argument("In createBoundingBox, polynomial term is empty");
    //mDebug() << "\ncreating BB from poly===" << poly.toString();

    geom::BoundingBox bbox;  // BBのデフォルトコンストラクタは体積ゼロボックスなので適当な初期化が必要であることに注意。
    if(!poly.factors().empty()) {
        // 項内の多項式(polynomials_)と因子(factors_)のうち、
        // まずfactors_の方からBBを作成する。。factorTypeはintなのでこのintからSurfaceインスタンスを取得する。
        bbox = geom::BoundingBox::universalBox();
        for(const auto &fac: poly.factors()) {
//			mDebug() << "ANDを取る右辺BBを生成する平面===" << smap.at(fac)->toString();
//					 << "BB===" << smap.at(fac)->generateBoundingBox().toInputString();
            geom::BoundingBox rhs = smap.at(fac)->generateBoundingBox();
//			mDebug() << "lhs===" << bbox.toInputString() << ", rhs===" << rhs.toInputString();
            bbox = geom::BoundingBox::AND(bbox, rhs);
//			mDebug() << "AND result ===" << bbox.toInputString() << "\n";
        }
        // ここで再帰終端される
    } else if(!poly.factorPolys().empty()) {
        bbox = geom::BoundingBox::universalBox();
        for(const auto &fpoly: poly.factorPolys()) {
            geom::BoundingBox rhs = createBoundingBox(fpoly, smap);
            bbox = geom::BoundingBox::AND(bbox, rhs);
        }
    } else {
        bbox = createBoundingBox(poly.terms().at(0), smap);
        // 論理多項式の項が2個以上の場合 BoundingBoxの和を取る。
        for(size_t i = 1; i < poly.terms().size(); ++i) {
            bbox = geom::BoundingBox::OR(bbox, createBoundingBox(poly.terms().at(i), smap));
        }
    }

//	mDebug() << "poly===" << poly.toString() << "に対してReturnされるbb===" << bbox.toInputString();
    return bbox;
}


#ifdef ENABLE_GUI
#include <vtkImplicitBoolean.h>
#include "core/geometry/surface/surfacemap.hpp"
// 論理多項式ツリーを辿りながらvtkImplicitFunctionを作成する。
vtkSmartPointer<vtkImplicitFunction> geom::bb::createImplicitFunction(const lg::LogicalExpression<int> &poly, const geom::SurfaceMap &smap)
{
    if(poly.empty()) throw std::invalid_argument("In generateImplicitFunction(), polynomial is empty");

    auto bop = vtkSmartPointer<vtkImplicitBoolean>::New();
    if(!poly.factors().empty()) {
        // まずfactors_の方から。factorTypeはintなのでこのintからSurfaceインスタンスを取得する。
        if(poly.factors().size() == 1)  return smap.at(poly.factors().front())->generateImplicitFunction();
        for(auto &fac: poly.factors()) {
            vtkSmartPointer<vtkImplicitFunction> rhs = smap.at(fac)->generateImplicitFunction();
            bop->AddFunction(rhs);
        }
        bop->SetOperationTypeToIntersection();
    } else if(!poly.factorPolys().empty()) {
        // サイズが1ならbinary opせずにreturnする。
        if(poly.factorPolys().size() == 1) 	return createImplicitFunction(poly.factorPolys().front(), smap);
        //auto bop = vtkSmartPointer<vtkImplicitBoolean>::New();
        for(const auto &fpoly: poly.factorPolys()) {
            vtkSmartPointer<vtkImplicitFunction> rhs = createImplicitFunction(fpoly,smap);
            bop->AddFunction(rhs);
        }
        bop->SetOperationTypeToIntersection();
    } else {
        // ※vtkImplicitFunctionはpure virtualなのでNew()できないことに留意
        vtkSmartPointer<vtkImplicitFunction> lhs = createImplicitFunction(poly.terms().front(), smap);
        // 多項式の項が1個ならそのままリターン
        if(poly.terms().size() == 1) return lhs;
        // 論理多項式の項が2個以上の場合
        bop->AddFunction(lhs);
        for(size_t i = 1; i < poly.terms().size(); ++i) {
            vtkSmartPointer<vtkImplicitFunction> rhs = createImplicitFunction(poly.terms().at(i), smap);
            bop->AddFunction(rhs);
        }
        bop->SetOperationTypeToUnion();
    }

    return bop;
}
#endif
