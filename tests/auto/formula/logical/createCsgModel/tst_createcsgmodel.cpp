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
﻿#include <QtTest>

#include <cmath>
#include <string>
#include <unordered_map>

#include "../../../../../core/formula/logical/lpolynomial.hpp"
#include "../../../../../core/geometry/surface/surfacemap.hpp"
#include "../../../../../core/geometry/surface/surface.hpp"
#include "../../../../../core/geometry/surface/plane.hpp"
#include "../../../../../core/geometry/surface/sphere.hpp"
#include "../../../../../core/geometry/surface/cylinder.hpp"
#include "../../../../../core/geometry/surface/cone.hpp"
#include "../../../../../core/geometry/surface/torus.hpp"
#include "../../../../../core/geometry/surf_utils.hpp"

#include "../../../../../core/math/nvector.hpp"
#include "../../../../../core/utils/message.hpp"

#include "../../../../../component/csgjs-cpp/csgjs.hpp"
#include "../../../../../component/csgjs-cpp/csgjsvertex.hpp"
#include "../../../../../component/csgjs-cpp/csgjs_utils.hpp"

namespace {
constexpr double LEN = 20;
constexpr double DX = 10, DY = 2.2, DZ = -5.5;  // 直方体の中心
//constexpr double DX = 0, DY = 0, DZ = 0;
constexpr double SX=-5, SY=2.2, SZ=-5.5, SR=10;  // OK 球の中有心xyz及び半径

// 円筒
//constexpr double CY_X=5, CY_Y=6, CY_Z=-1.5, CY_R=5;  // 位置
//constexpr double CY_DX=1, CY_DY=2, CY_DZ=3;  // 軸ベクトル
constexpr double CY_X=0, CY_Y=0, CY_Z=0, CY_R=10;  // 位置
constexpr double CY_DX=0, CY_DY=0, CY_DZ=1;  // 軸ベクトル
// 円錐
constexpr double CN_X=1, CN_Y=1, CN_Z=5.5, CN_DX=1, CN_DY = 1, CN_DZ=1, CN_R=1.2;

//constexpr double TR_X = -10, TR_Y=10, TR_Z=10, TR_DX=-1, TR_DY=0, TR_DZ=1, TR_R=10, TR_VR=2.5, TR_HR=1.1;
constexpr double TR_X = -10, TR_Y= 0, TR_Z= 0, TR_DX=-1, TR_DY=1, TR_DZ=1, TR_R=15, TR_VR=8, TR_HR=5;

constexpr double EXTENT = 500;
constexpr int DIV = 1;

bool isInBox(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax, const csgjs::Vertex& v)
{
    return xmin <= v.pos.x  && v.pos.x <= xmax
            && ymin <= v.pos.y  && v.pos.y <= ymax
            && zmin <= v.pos.z  && v.pos.z <= zmax;
}

bool compareDouble(double d1, double d2, double eps) {return std::abs(d1-d2) < eps;}


}  // end anonymous namespace

using Pt = math::Point;
using Vec3 = math::Vector<3>;
using Vert = csgjs::Vertex;
using Lg = lg::LogicalExpression<int>;

class createCsgModel : public QObject
{
    Q_OBJECT

public:
    createCsgModel();
    ~createCsgModel();

private slots:
    void testCylinderAndPlanes();
//    void testTorus();
//    void testCone();
//    void testCylinder();
//    void testPlanes();
//    void testSphere();

//        void testCreateCubeModelFromPlanes();
//    void testCubeAndSphere();
//    void testCubeAndTorus();

private:
    // 文字列からFactorTypeへの変換テーブル
    std::unordered_map<std::string, int> convMap_;
    geom::SurfaceMap smap_;
};

createCsgModel::createCsgModel()
{
//    // convmapはとりあえず数字を文字列化して作成。
//    for(int i = -100; i < 100; ++i) {
//        convMap_.emplace(std::to_string(i), i);
//    }
    // とりあえず普通の(面法線が内向きの)cubeを作るための平面を定義する。。
//    std::shared_ptr<const geom::Surface> p1 = geom::Plane::fromString("1 PX -25", false);
//    std::shared_ptr<const geom::Surface> p2 = geom::Plane::fromString("2 PX  25", false);
//    std::shared_ptr<const geom::Surface> p3 = geom::Plane::fromString("3 PY -25", false);
//    std::shared_ptr<const geom::Surface> p4 = geom::Plane::fromString("4 PY  25", false);
//    std::shared_ptr<const geom::Surface> p5 = geom::Plane::fromString("5 PZ -25", false);
//    std::shared_ptr<const geom::Surface> p6 = geom::Plane::fromString("6 PZ  25", false);

    std::shared_ptr<const geom::Surface> p1 = std::make_shared<geom::Plane>("1", Vec3{ 1,  0,  0}, -0.5*LEN+DX);  // -x位置
    std::shared_ptr<const geom::Surface> p2 = std::make_shared<geom::Plane>("2", Vec3{ 1,  0,  0},  0.5*LEN+DX);  // +x
    std::shared_ptr<const geom::Surface> p3 = std::make_shared<geom::Plane>("3", Vec3{ 0,  1,  0}, -0.5*LEN+DY);  // -y
    std::shared_ptr<const geom::Surface> p4 = std::make_shared<geom::Plane>("4", Vec3{ 0,  1,  0},  0.5*LEN+DY);  // +y
    std::shared_ptr<const geom::Surface> p5 = std::make_shared<geom::Plane>("5", Vec3{ 0,  0,  1}, -0.5*LEN+DZ);  // -z
    std::shared_ptr<const geom::Surface> p6 = std::make_shared<geom::Plane>("6", Vec3{ 0,  0,  1},  0.5*LEN+DZ);  // +z
    smap_.registerSurface(p1->getID(), p1); convMap_.emplace(p1->name(), p1->getID());
    smap_.registerSurface(p2->getID(), p2); convMap_.emplace(p2->name(), p2->getID());
    smap_.registerSurface(p3->getID(), p3); convMap_.emplace(p3->name(), p3->getID());
    smap_.registerSurface(p4->getID(), p4); convMap_.emplace(p4->name(), p4->getID());
    smap_.registerSurface(p5->getID(), p5); convMap_.emplace(p5->name(), p5->getID());
    smap_.registerSurface(p6->getID(), p6); convMap_.emplace(p6->name(), p6->getID());
    // 球
    std::shared_ptr<const geom::Surface> s1 = std::make_shared<geom::Sphere>("7", Vec3{SX, SY, SZ}, SR);
    smap_.registerSurface(s1->getID(), s1); convMap_.emplace(s1->name(), s1->getID());
    // 円柱
    std::shared_ptr<const geom::Surface> cy1
          = std::make_shared<geom::Cylinder>("8", Pt{CY_X, CY_Y, CY_Z}, Vec3{CY_DX, CY_DY, CY_DZ}, CY_R);
    smap_.registerSurface(cy1->getID(), cy1); convMap_.emplace(cy1->name(), cy1->getID());
    // 円錐
    std::shared_ptr<const geom::Surface> cn1
        = std::make_shared<geom::Cone>("9", Pt{CN_X, CN_Y, CN_Z}, Vec3{CN_DX, CN_DY, CN_DZ}, CN_R, 0);
    smap_.registerSurface(cn1->getID(), cn1); convMap_.emplace(cn1->name(), cn1->getID());
    std::shared_ptr<const geom::Surface> cn2
        = geom::Cone::createCone("91",
//            std::vector<double>{0.0, 27,  71.5,  0.151234568, -1},
            std::vector<double>{0.0, 27,  71.5,  1, -1},
            math::Matrix<4>::IDENTITY(),
            geom::Cone::TYPE::KY, false);
    smap_.registerSurface(cn2->getID(), cn2); convMap_.emplace(cn2->name(), cn2->getID());

    // トーラス
    std::shared_ptr<const geom::Surface> tor1
        = std::make_shared<geom::Torus>("10", Pt{TR_X, TR_Y, TR_Z}, Vec3{TR_DX, TR_DY, TR_DZ}, TR_R, TR_VR, TR_HR);
    smap_.registerSurface(tor1->getID(), tor1); convMap_.emplace(tor1->name(), tor1->getID());


    // その他
    std::shared_ptr<const geom::Surface> p2031
        = std::make_shared<geom::Cylinder>("2031", Pt{0,0,0}, Vec3{0,0,-1}, 2.6);
    std::shared_ptr<const geom::Surface> p2032 = geom::Plane::fromString("2032 p 0 0 -1 100", false);
    std::shared_ptr<const geom::Surface> p2033 = geom::Plane::fromString("2033 p 0 0  1 0", false);
    smap_.registerSurface(p2033->getID(), p2033); convMap_.emplace(p2033->name(), p2033->getID());
    smap_.registerSurface(p2032->getID(), p2032); convMap_.emplace(p2032->name(), p2032->getID());
    smap_.registerSurface(p2031->getID(), p2031); convMap_.emplace(p2031->name(), p2031->getID());

    // ウラ面準備
    utils::addReverseSurfaces(&smap_);
    auto tmpMap = convMap_;
    for(const auto& p: tmpMap) {
        if(p.first.front() != '-' )
        convMap_.emplace("-"+p.first, -1*p.second);
    }

}

createCsgModel::~createCsgModel() {}

void createCsgModel::testCylinderAndPlanes()
{
    // FIXME 円筒の軸ベクトルが-z方向の時表裏が逆転してしまう。 表面ならstl法線は内側が正しい。
    auto cyl =  lg::createCsgModel(Lg::fromString("2031", convMap_), smap_, DIV, EXTENT);
    saveToStl(cyl, "cyl.stl");

    auto p1 =  lg::createCsgModel(Lg::fromString("-2032", convMap_), smap_, DIV, EXTENT);
    auto p2 =  lg::createCsgModel(Lg::fromString("-2033", convMap_), smap_, DIV, EXTENT);
    auto res = lg::createCsgModel(Lg::fromString("-2031 -2032 -2033", convMap_), smap_, DIV, EXTENT);
    cyl.flip();
    saveToStl(cyl, "cylr.stl");

    saveToStl(p1, "p1.stl");
    saveToStl(p2, "p2.stl");
    saveToStl(res, "res.stl");
}


//void createCsgModel::testPlanes()
//{
//    /*
//     * 頂点位置が異なっても同様の3Dオブジェクトになり得る、
//     * ため3Dオブジェクトの等価判定は難しい。→ テストも難しい。
//     * 具体的な頂点位置はどちらかというと隠蔽されるべきprivateな
//     * メンバ変数に属する。
//     *
//     * 故に平面ごとの頂点位置はテストしない。
//     * 平面定義を何点でやるか、その位置はどうするか、あたりは多分今後また変わる。
//     *
//     * なので平面テストについては出力結果をstlにして
//     * 必要が生じればparaviewででも見てチェック
//     */
//    const std::string str1 = "1";
//    lg::LogicalExpression<int> poly1 = lg::LogicalExpression<int>::fromString(str1, convMap_);
//    csgjs::Model model1 = lg::createCsgModel(poly1, smap_, DIV, EXTENT);

//    //QCOMPARE(model1.vertices.size(), 3);
//    // この場合面上v1には(0, 1, 0)が選択され、v2 = normal × v1 = (0 0 1)
//    //    Vert vert3(-0.5*LEN, EXTENT, 0),  vert2(-0.5*LEN, 0, EXTENT);
//    //    Vert vert1(-0.5*LEN, -EXTENT, 0), vert4(-0.5*LEew N, 0, -EXTENT);
//    //    std::vector<csgjs::Vertex> ref_verts1{
//    //        vert1, vert2, vert3,  vert3, vert4, vert1
//    //    };
//    //    for(size_t i = 0; i < ref_verts1.size(); ++i) {
//    //        //mDebug() << "i=" << i << "ref=" << ref_verts1[i] << ", result=" << model1.vertices[i];
//    //        QVERIFY(compareVertices(ref_verts1[i], model1.vertices[i], 1e-6));
//    //    }
//    csgjs::saveToStl(model1, "xmplane.stl");
//    csgjs::saveToStl(lg::createCsgModel(Lg::fromString("2", convMap_), smap_, DIV, EXTENT), "xpplane.stl");
//    csgjs::saveToStl(lg::createCsgModel(Lg::fromString("3", convMap_), smap_, DIV, EXTENT), "ymplane.stl");
//    csgjs::saveToStl(lg::createCsgModel(Lg::fromString("4", convMap_), smap_, DIV, EXTENT), "ypplane.stl");
//    csgjs::saveToStl(lg::createCsgModel(Lg::fromString("5", convMap_), smap_, DIV, EXTENT), "zmplane.stl");
//    csgjs::saveToStl(lg::createCsgModel(Lg::fromString("6", convMap_), smap_, DIV, EXTENT), "zpplane.stl");
//}

//void createCsgModel::testTorus()
//{
//    const std::string str = "-10";
//    auto poly = Lg::fromString(str, convMap_);
//    csgjs::Model torusModel = lg::createCsgModel(poly, smap_, DIV, EXTENT);
//    saveToStl(torusModel, "torus.stl");

//    // トーラスが原点中心でz方向を向いているように変換する行列を用意
//    std::shared_ptr<const geom::Torus> torus =  std::dynamic_pointer_cast<const geom::Torus>(smap_.at("10"));
//    auto matrix = math::Matrix<4>::IDENTITY();
//    // 原点中心、z軸方向楕円トーラスに返還してからの陰関数が面上で0になることを確認する。
//    // affienTransformでは回転→並進の順に作用させられるが
//    // ここでは先に並進を作用させる必要が有るのでmatrixに並進は含めない。
//    matrix.setRotationMatrix(math::generateRotationMatrix1(Vec3{0,0,1}, torus->axis()));
//    double R = torus->majorRadius(), a = torus->minorVRadius(), b = torus->minorHRadius();
//    for(size_t i = 0; i < torusModel.vertices.size(); ++i) {
//        const csgjs::Vertex &v = torusModel.vertices.at(i);
//        Vec3 pos{v.pos.x, v.pos.y,v.pos.z};
//        pos = pos - torus->center();
//        math::affineTransform(&pos, matrix);
//        // 以下(原点中心、z軸方向)楕円トーラスの陰関数
//        double x = pos.x(), y = pos.y(), z = pos.z();
//        double root = std::sqrt((x*x)/(b*b) + (y*y)/(b*b));
//        double func = std::pow(root - R/b, 2) +(z*z)/(a*a) - 1;
//        QVERIFY(compareDouble(func, 0, 1e-6));
//    }
//}

//void createCsgModel::testCone()
//{
//    const std::string str = "-9";
//    auto poly = Lg::fromString(str, convMap_);
//    csgjs::Model coneModel = lg::createCsgModel(poly, smap_, DIV, EXTENT);
//    saveToStl(coneModel, "cone.stl");

//    Vec3 top = Vec3{CN_X, CN_Y, CN_Z}, dir = Vec3{CN_DX, CN_DY, CN_DZ}.normalized();
//    for(size_t i = 1; i < coneModel.vertices.size(); ++i) {
//        const auto v = coneModel.vertices.at(i);
//        // dist1 頂点からの径方向の距離、dist2:頂点からの軸方向の距離
//        double dist1 = math::dotProd(Vec3{v.pos.x, v.pos.y, v.pos.z} - top, dir);
//        double dist2 = (top - Vec3{v.pos.x, v.pos.y, v.pos.z}
//            - math::dotProd(top - Vec3{v.pos.x, v.pos.y, v.pos.z}, dir)*dir).abs();
//        //mDebug() << "dist1, dist2, dist2/dist1 ===" << dist1 << dist2 << dist2/dist1;
//        QVERIFY(compareDouble(std::abs(dist2/dist1), CN_R, 1e-6));
//    }

//    auto poly2 = Lg::fromString("-91", convMap_);
//    auto cone2 = smap_.at("-91");
//    mDebug() << "cone2===" << cone2->toString();
//    auto cone2Model = lg::createCsgModel(poly2, smap_, DIV, EXTENT);
//    saveToStl(cone2Model, "cone2.stl");
//}

//void createCsgModel::testCylinder()
//{
//    using Vec = math::Vector<3>;
//    const std::string str = "-8";
//    Lg poly = Lg::fromString(str, convMap_);
//    csgjs::Model cylinderModel = lg::createCsgModel(poly, smap_, DIV, EXTENT);

//    const math::Point refPt = math::Point{CY_X, CY_Y, CY_Z};
//    const math::Vector<3> refDir = math::Vector<3>{CY_DX, CY_DY, CY_DZ}.normalized();
//    // 頂点座標はrefPtを通る方向refDirの直線からの距離がCY_R
//    for(const auto& v: cylinderModel.vertices) {
//        double dist = (refPt - Vec{v.pos.x, v.pos.y, v.pos.z}
//                    - math::dotProd(refPt - Vec{v.pos.x, v.pos.y, v.pos.z}, refDir)*refDir).abs();
//        QVERIFY(compareDouble(CY_R, dist, 1e-6));
//    }
//    saveToStl(cylinderModel, "cylinderm8.stl");
//    mDebug() << "indices===" << cylinderModel.indices;


//    cylinderModel.flip();
//    mDebug() << "after flip: indices===" << cylinderModel.indices;
//    saveToStl(cylinderModel, "cylinderm8r.stl");

//    auto p8model = lg::createCsgModel(Lg::fromString("+8", convMap_), smap_, DIV, EXTENT);
//    saveToStl(p8model, "cylinderp8.stl");
//    p8model.flip();
//    saveToStl(p8model, "cylinderp8r.stl");
//}



//void createCsgModel::testSphere()
//{
//    const std::string str1 = "-7";
//    Lg poly = Lg::fromString(str1, convMap_);
//    csgjs::Model model1 = lg::createCsgModel(poly, smap_, DIV, EXTENT);
//    csgjs::saveToStl(model1, "sphere.stl");
//    for(size_t i = 0; i < model1.vertices.size(); ++i) {
//        auto vpos = model1.vertices.at(i).pos;
//        double dist = std::sqrt((vpos.x-SX)*(vpos.x-SX) + (vpos.y-SY)*(vpos.y-SY) + (vpos.z-SZ)*(vpos.z-SZ));
//        QVERIFY(compareDouble(SR, dist, 1e-5));
//    }
//}


//void createCsgModel::testCreateCubeModelFromPlanes()
//{
//    // 6面体内部定義
//    const std::string str = "1 -2 3 -4 5 -6";
//    lg::LogicalExpression<int> poly = lg::LogicalExpression<int>::fromString(str, convMap_);
//    csgjs::Model model = lg::createCsgModel(poly, smap_, DIV, EXTENT);
//    csgjs::saveToStl(model, "cubemodel.stl");

//    // このcubeは一辺がLENで、中心が(DX,DY,DZ)の立方体なので頂点はその範囲に収まるはず。(必要条件に過ぎないが)
//    for(const auto& vertex: model.vertices) {
//        QVERIFY(isInBox(DX-LEN, DX+LEN, DY-LEN, DY+LEN, DZ-LEN, DZ+LEN, vertex));
//    }
//    model.flip();
//    csgjs::saveToStl(model,"cubemodelr.stl");
//}

//void createCsgModel::testCubeAndSphere()
//{
//    /*
//     *パラメータによっては球と直方体は交差しないが、そのときでもunion演算の結果はemptyにならないはず。
//     */
//    csgjs::Model cube = lg::createCsgModel(Lg::fromString("1 2 3 4 5 6", convMap_), smap_, DIV, EXTENT);
//    csgjs::Model sphere = lg::createCsgModel(Lg::fromString("-7", convMap_), smap_, DIV, EXTENT);
//    csgjs::Model cubeOrSphere = csgjs::opUnion(cube, sphere);
//    csgjs::saveToStl(cubeOrSphere, "cubeorsphere.ref.stl");  // これはOk


//    const std::string str2 = "(1 2 3 4 5 6):(-7):-8";
//    lg::LogicalExpression<int> poly2 = lg::LogicalExpression<int>::fromString(str2, convMap_);
//    csgjs::Model model2 = lg::createCsgModel(poly2, smap_, DIV, EXTENT);
//    csgjs::saveToStl(model2, "cubeorsphere.stl");

//    const std::string str = "1 2 3 4 5 6 -7";
//    lg::LogicalExpression<int> poly = lg::LogicalExpression<int>::fromString(str, convMap_);
//    csgjs::Model model = lg::createCsgModel(poly, smap_, DIV, EXTENT);
//    csgjs::saveToStl(model, "cubeandsphere.stl");
//}

//void createCsgModel::testCubeAndTorus()
//{
//    csgjs::Model cubeortorus = lg::createCsgModel(Lg::fromString("1 2 3 4 5 6:-10", convMap_), smap_, DIV, EXTENT);
//    csgjs::saveToStl(cubeortorus, "cubeortorus.stl");
//    csgjs::Model cubeandtorus = lg::createCsgModel(Lg::fromString("1 2 3 4 5 6 -10", convMap_), smap_, DIV, EXTENT);
//    csgjs::saveToStl(cubeandtorus, "cubeandtorus.stl");
//}


QTEST_APPLESS_MAIN(createCsgModel)

#include "tst_createcsgmodel.moc"
