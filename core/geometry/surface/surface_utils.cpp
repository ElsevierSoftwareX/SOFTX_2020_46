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
#include "surface_utils.hpp"

#include <functional>
#include "surface.hpp"
#include "cone.hpp"
#include "cylinder.hpp"
#include "plane.hpp"
#include "polyhedron.hpp"
#include "quadric.hpp"
#include "sphere.hpp"
#include "torus.hpp"
#include "triangle.hpp"

namespace {

typedef std::function<std::shared_ptr<geom::Surface>(
                                                    const std::string&,
                                                    const std::vector<double>&,
                                                    const std::map<std::string, std::string> &,
                                                    const math::Matrix<4>&,
                                                    bool
                                                    )>     function_type;
typedef std::unordered_map<std::string, function_type> function_map_type;


const function_map_type &getCreationFunctionMap()
{
    // 一般的なcreate関数引数にbindして個別の特殊タイプのsurface作成関数を作る。
    // _1がsurface名, _2が実数パラメータ vector<double>, _3がmatrixからmap<string, string>, _4がmatrix, _5がbool
    // 見づらいからstd::bindかららラムダ式に置き換えたほうが良いかも
    namespace sp = std::placeholders;
    using namespace geom;
    static function_map_type funcMap{
        {"s",   std::bind(geom::Sphere::createSphere, sp::_1, sp::_2, sp::_4, geom::Sphere::TYPE::S, sp::_5)},
        {"so",  std::bind(geom::Sphere::createSphere, sp::_1, sp::_2, sp::_4, geom::Sphere::TYPE::SO, sp::_5)},
        {"sx",  std::bind(geom::Sphere::createSphere, sp::_1, sp::_2, sp::_4, geom::Sphere::TYPE::SX, sp::_5)},
        {"sy",  std::bind(geom::Sphere::createSphere, sp::_1, sp::_2, sp::_4, geom::Sphere::TYPE::SY, sp::_5)},
        {"sz",  std::bind(geom::Sphere::createSphere, sp::_1, sp::_2, sp::_4, geom::Sphere::TYPE::SZ, sp::_5)},
        {"p",   std::bind(geom::Plane::createPlane, sp::_1, sp::_2, sp::_4, geom::Plane::TYPE::P, sp::_5)},
        {"px",  std::bind(geom::Plane::createPlane, sp::_1, sp::_2, sp::_4, geom::Plane::TYPE::PX, sp::_5)},
        {"py",  std::bind(geom::Plane::createPlane, sp::_1, sp::_2, sp::_4, geom::Plane::TYPE::PY, sp::_5)},
        {"pz",  std::bind(geom::Plane::createPlane, sp::_1, sp::_2, sp::_4, geom::Plane::TYPE::PZ, sp::_5)},
        {"c/x", std::bind(geom::Cylinder::createCylinder, sp::_1, sp::_2, sp::_4, geom::Cylinder::TYPE::CX, sp::_5)},
        {"c/y", std::bind(geom::Cylinder::createCylinder, sp::_1, sp::_2, sp::_4, geom::Cylinder::TYPE::CY, sp::_5)},
        {"c/z", std::bind(geom::Cylinder::createCylinder, sp::_1, sp::_2, sp::_4, geom::Cylinder::TYPE::CZ, sp::_5)},
        {"cx",  std::bind(geom::Cylinder::createCylinder, sp::_1, sp::_2, sp::_4, geom::Cylinder::TYPE::CXO, sp::_5)},
        {"cy",  std::bind(geom::Cylinder::createCylinder, sp::_1, sp::_2, sp::_4, geom::Cylinder::TYPE::CYO, sp::_5)},
        {"cz",  std::bind(geom::Cylinder::createCylinder, sp::_1, sp::_2, sp::_4, geom::Cylinder::TYPE::CZO, sp::_5)},
        {"ca",  std::bind(geom::Cylinder::createCylinder, sp::_1, sp::_2, sp::_4, geom::Cylinder::TYPE::CA, sp::_5)},
        {"k/x", std::bind(geom::Cone::createCone, sp::_1, sp::_2, sp::_4, geom::Cone::TYPE::KX, sp::_5)},
        {"k/y", std::bind(geom::Cone::createCone, sp::_1, sp::_2, sp::_4, geom::Cone::TYPE::KY, sp::_5)},
        {"k/z", std::bind(geom::Cone::createCone, sp::_1, sp::_2, sp::_4, geom::Cone::TYPE::KZ, sp::_5)},
        {"kx",  std::bind(geom::Cone::createCone, sp::_1, sp::_2, sp::_4, geom::Cone::TYPE::KXO, sp::_5)},
        {"ky",  std::bind(geom::Cone::createCone, sp::_1, sp::_2, sp::_4, geom::Cone::TYPE::KYO, sp::_5)},
        {"kz",  std::bind(geom::Cone::createCone, sp::_1, sp::_2, sp::_4, geom::Cone::TYPE::KZO, sp::_5)},
        {"ka",  std::bind(geom::Cone::createCone, sp::_1, sp::_2, sp::_4, geom::Cone::TYPE::KA, sp::_5)},
        {"gq",  std::bind(geom::Quadric::createQuadric, sp::_1, sp::_2, sp::_4, geom::Quadric::TYPE::GQ, sp::_5)},
        {"sq",  std::bind(geom::Quadric::createQuadric, sp::_1, sp::_2, sp::_4, geom::Quadric::TYPE::SQ, sp::_5)},
        {"tx",  std::bind(geom::Torus::createTorus, sp::_1, sp::_2, sp::_4, geom::Torus::TYPE::TX, sp::_5)},
        {"ty",  std::bind(geom::Torus::createTorus, sp::_1, sp::_2, sp::_4, geom::Torus::TYPE::TY, sp::_5)},
        {"tz",  std::bind(geom::Torus::createTorus, sp::_1, sp::_2, sp::_4, geom::Torus::TYPE::TZ, sp::_5)},
        {"ta",  std::bind(geom::Torus::createTorus, sp::_1, sp::_2, sp::_4, geom::Torus::TYPE::TA, sp::_5)},
        {"tri", std::bind(geom::Triangle::createTriangle, sp::_1, sp::_2, sp::_4, sp::_5)},
        {"poly", geom::PolyHedron::createPolyhedron},
    };
    return funcMap;
}

}  // end anonymous namespace



/*
 *  create関数は
 * ・返り値 std::shared_ptr<geom::Surface>
 * ・引数 const string& name, const vector<double>& params, const Matrix<4> &trMatrix
 *
 * 但し、それぞれの面create関数は4番目の引数に、面のタイプを取り、特殊形の面を作るようにする。
 */
std::shared_ptr<geom::Surface> geom::createSurface(const std::string &surfName,
                                                                   const std::string &surfSymbol,
                                                                   const std::vector<double> &surfParams,
                                                                   const std::map<std::string, std::string> &paramMap,
                                                                   const math::Matrix<4> &trMatrix,
                                                                   bool warnPhitsCompat)
{
    auto creationFunctionMap = getCreationFunctionMap();
    if(creationFunctionMap.find(surfSymbol) == creationFunctionMap.end()) {
        throw std::out_of_range(std::string("Symbol \"") + surfSymbol + "\" is not valid");
    }
    //mDebug() << "Creating surface, name===" << surfName << "tr ===" << trMatrix;
    return creationFunctionMap.at(surfSymbol)(surfName, surfParams, paramMap, trMatrix, warnPhitsCompat);
}

