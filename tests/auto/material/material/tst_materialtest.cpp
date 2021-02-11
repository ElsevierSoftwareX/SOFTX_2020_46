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
#include <QString>
#include <QtTest>
#include <QMetaObject>

#include <cmath>
#include <cstdlib>
#include <list>
#include <string>

#include "core/io/input/dataline.hpp"
#include "core/material/material.hpp"
#include "core/material/materials.hpp"
#include "component/libacexs/libsrc/xsdir.hpp"
#include "core/utils/utils.hpp"



using namespace mat;
using namespace inp;

static size_t mid = 0;

Q_DECLARE_METATYPE(std::string)

class MaterialTest : public QObject
{
	Q_OBJECT

public:
	MaterialTest();
private:
    std::string xs_dir_path_;
    std::string datapath_;
    std::shared_ptr<const ace::XsDir> xsdir_;

private Q_SLOTS:
    void testPhotoatomicNuclideTotal_data();
    void testPhotoatomicNuclideTotal();
    void testMaterialP();
    void testMaterialNP();
    void testMaterialNPMulti();
    void testMap();
};

MaterialTest::MaterialTest()
{
    mDebug() << "begin ctor";
    // TEST_XSDATA_DIR is defined in material.pro
    // $$PROJECT/component/libacexs/example/tests
    const std::string xsdata_dir = TEST_XSDATA_DIR;
    mDebug() << "xsdata_dir===" << xsdata_dir;
//	std::cerr << "testdor=" << xsdata_dir << std::endl;
//	mDebug() << "testdor="<< xsdata_dir;  何故かテスト失敗扱いになる。
    xs_dir_path_ = xsdata_dir + "/xs";
    auto fileName = xs_dir_path_ + "/xsdir.all";
    xsdir_ = std::make_shared<const ace::XsDir>(ace::XsDir(fileName));
    datapath_ = xsdir_->datapath();
}

void MaterialTest::testPhotoatomicNuclideTotal_data()
{
    QTest::addColumn<std::string>("aceFileName");
    QTest::addColumn<double>("awr");
    QTest::addColumn<std::string>("zaidx");
    QTest::addColumn<int>("startline");
    QTest::addColumn<std::vector<double>>("energies");
    QTest::addColumn<std::vector<double>>("expected");

    std::vector<double> energies{0.1, 0.13, 0.6, 1.0, 1.115, 2, 8.9, 9.9999};
    std::vector<double> xcomH{4.928E-01, 4.616E-01, 2.676E-01, 2.114E-01, 2.004E-01, 1.468E-01, 5.862E-02, 5.447E-02};
    std::vector<double> xcomFe{3.447E+01, 2.208E+01, 7.144E+00, 5.559E+00, 5.259E+00, 3.955E+00, 2.770E+00, 2.777E+00,};
    QTest::newRow("H-endf")   << xs_dir_path_ + "/1000.84p.ace"   << 1.0    << std::string("1000.84p")  << 0 << energies << xcomH;
    QTest::newRow("Fe-endf")  << xs_dir_path_ + "/26000.84p.ace"  << 55.367 << std::string("26000.84p") << 0 << energies << xcomFe;
    QTest::newRow("Fe-jendl") << xs_dir_path_ + "/Fe000.j40p.ace" << 55.367 << std::string("26000.50p") << 0 << energies << xcomFe;
    ace::XsInfo info = xsdir_->getNuclideInfo("26000.84p", ace::NTY::CONTIUNOUS_PHOTOATOMIC);
    QTest::newRow("Fe-endf/mono") << datapath_ + "/" + info.filename << info.awr << info.tableID << info.address << energies << xcomFe;
}

void MaterialTest::testPhotoatomicNuclideTotal()
{
    /*
     *  核種データは
     *  Nuclide(const string &zaidx, double awr, const ace::AceFile::XSmap_type &xsmap)か、
     *  createNuclide(const string &filename, double awr, const string &zaidx, size_t startline);
     *  で生成。後者はshared_ptr<const>
     */
    QFETCH(std::string, aceFileName);
    QFETCH(double, awr);
    QFETCH(std::string, zaidx);
    QFETCH(int, startline);
    QFETCH(std::vector<double>, energies);
    QFETCH(std::vector<double>, expected);

    std::shared_ptr<const Nuclide> nuclide = Nuclide::createNuclide(aceFileName, awr, zaidx, startline);
    //for(size_t i = 0; i < energies.size(); ++i) {
    for(size_t i = 0; i < energies.size(); ++i) {
        auto result = nuclide->getTotalXs(energies.at(i));
//		mDebug() << "ENERGY=" << energies.at(i) << "result=" << result
//				 << "expect=" << expected.at(i)  << "diff=" << result - expected.at(i);
        QVERIFY(std::abs(expected.at(i) - result) < 5e-2);
    }
}

void MaterialTest::testMaterialP()
{
    // voidテスト
    auto voidMatPTr = Material::void_material_ptr();
    QCOMPARE(voidMatPTr->id(), 0);

    Material mat0;
    QCOMPARE(mat0.id(), 0);

    /*
     *  普通のmaterialを作る。
     * 第一引数はid名
     * 第二引数は
     * unordered_multimap<ParticleType, pair<std::shared_ptr<const Nuclide>, double>>
     * で、Nuclide存在割合のペアをParticleTypeをキーにしたmultimap
     *
     * あるいはMaterial::createMaterial(idStr, xsdir, ptypes, nuclideParams)で作成。
     * nuclideParamsはMaterialカードの核種インプット
     */
    const std::unordered_map<std::string, std::string> &opts{std::make_pair("pli", "84p")};
    std::shared_ptr<const Material>
             hydorogen = mat::Material::createMaterial("mat1",
                                                       ++mid,
                                                        xsdir_,
                                                        std::vector<phys::ParticleType>{phys::ParticleType::PHOTON},
                                                        std::vector<std::string>{"1000.84p", "1.0"},
                                                        opts,
                                                        "");
    std::shared_ptr<const Material>
             water = mat::Material::createMaterial("water",
                                                   ++mid,
                                                   xsdir_,
                                                   std::vector<phys::ParticleType>{phys::ParticleType::PHOTON},
                                                   std::vector<std::string>{"1000", "2.0", "8000", "1.0"},
                                                   opts,
                                                   "");
    std::vector<double> energies{2.560E-03, 1.000E-01, 6.000E-01, 1.000E+00,
                                  1.100E+00,  2.500E+00, 8.800E+00,	9.000E+00, 1.000E+01};

    std::vector<double> xcomWaterTot{3.053E+02, 1.707E-01, 8.956E-02, 7.072E-02,
                                      6.746E-02, 4.376E-02, 2.334E-02, 2.313E-02, 2.219E-02};
      // xcomWaterTotは値はcm2/gなので巨視的断面積に等しい(ρ=1g/ccなので)

     for(size_t i = 0; i < energies.size(); ++i) {
         double result = water->macroTotalXs(phys::ParticleType::PHOTON, 1.0, energies.at(i));
//		 mDebug() << "ENERGY=" << energies.at(i) << "result=" << result
//				  << "expect=" << xcomWaterTot.at(i)  << "diff=" << result - xcomWaterTot.at(i);
         QVERIFY(std::abs(1-xcomWaterTot.at(i)/result) < 5e-2);
     }
}

// 複数粒子種の場合。
void MaterialTest::testMaterialNP()
{
    /*
     * ZAID/SZAXでidentifier, classが無い場合の扱い
     *
     * 例1： 酸素で8000を入力した場合
     * 光子：8000.84pがヒットするので読み込み
     * 中性子：xsdirに酸素-natは無いのでエラー
     *
     * 例2：酸素で8016を入力した場合
     * 光子：8000.84pがヒット
     * 中性子：8016.80cがヒット
     *
     * → 光子ライブラリに関しては原子番号を無視(000に置き換えて)検索する。
     */
    std::shared_ptr<const Material> water = mat::Material::createMaterial("water2", ++mid, xsdir_,
                                               std::vector<phys::ParticleType>{phys::ParticleType::PHOTON, phys::ParticleType::NEUTRON},
                                               std::vector<std::string>{"1001", "2.0", "8016", "1.0"},
                                               std::unordered_map<std::string, std::string>{std::make_pair("plib", "84p")}, "");
    std::vector<double> energies{2.560E-03, 1.000E-01, 6.000E-01, 1.000E+00,
                                 1.100E+00,  2.500E+00, 8.800E+00,	9.000E+00, 1.000E+01};

    std::vector<double> xcomWaterTot{3.053E+02, 1.707E-01, 8.956E-02, 7.072E-02,
                                     6.746E-02, 4.376E-02, 2.334E-02, 2.313E-02, 2.219E-02};
     // xcomWaterTotは値はcm2/gなので巨視的断面積に等しい(ρ=1g/ccなので)

    for(size_t i = 0; i < energies.size(); ++i) {
        double result = water->macroTotalXs(phys::ParticleType::PHOTON, 1.0, energies.at(i));
//		mDebug() << "ENERGY=" << energies.at(i) << "result=" << result
//				 << "expect=" << xcomWaterTot.at(i)  << "diff=" << result - xcomWaterTot.at(i);
        QVERIFY(std::abs(1-xcomWaterTot.at(i)/result) < 5e-2);
    }
}

void MaterialTest::testMaterialNPMulti()
{
    std::shared_ptr<const Material> ironOxide = mat::Material::createMaterial("FeO", ++mid, xsdir_,
                                               std::vector<phys::ParticleType>{phys::ParticleType::PHOTON, phys::ParticleType::NEUTRON},
                                               std::vector<std::string>{"26054", "0.058", "26056", "0.9172", "26057", "0.022", "26058", "0.0028", "8016", "1.0" },
                                               std::unordered_map<std::string, std::string>{std::make_pair("plib", "84p")},  "");
    std::vector<double> energies{2.560E-03, 1.000E-01, 6.000E-01, 1.000E+00,
                                 1.100E+00,  2.500E+00, 8.800E+00,	9.000E+00, 1.000E+01};
    std::vector<double> xcomTot{7.352E+02,3.235E-01,7.785E-02,6.079E-02,5.792E-02,3.896E-02,2.808E-02,2.804E-02,2.793E-02};
    double density = 7.8;
    for(auto &val: xcomTot) {
        val *=density;
    }
     // xcomWaterTotは値はcm2/gなのでρ=1g/ccを掛けた時の巨視的断面積に等しい

    for(size_t i = 0; i < energies.size(); ++i) {
        double result = ironOxide->macroTotalXs(phys::ParticleType::PHOTON, density, energies.at(i));
//		mDebug() << "ENERGY=" << energies.at(i) << "result=" << result
//				 << "expect=" << xcomTot.at(i)  << "diff=" << result - xcomTot.at(i);
        QVERIFY(std::abs(1-xcomTot.at(i)/result) < 5e-2);
    }
}


void MaterialTest::testMap()
{
    std::list<DataLine> materialInput;
    materialInput.emplace_back(DataLine("testfile", 1, "MHydrogen 1001 1.0"));
    materialInput.emplace_back(DataLine("testfile", 2, "MAT[water3] 8016 1 1001 2"));
    materialInput.emplace_back(DataLine("testfile", 3, "MAT[1] 26054 0.058 26056 0.9172 26057 0.022 26058 0.0028"));
    materialInput.emplace_back(DataLine("testfile", 4, "MAT[4] 8016 1 1001 2"));

    std::atomic_size_t count;
    // mapにはmaterial名をキーにしてMaterialのスマポが入っている。
    auto materials = std::make_shared<const mat::Materials>(materialInput,
                                                            xsdir_->filename(),  // FIXME ここは本来xsdirの絶対パス
                                                            std::vector<phys::ParticleType>{phys::ParticleType::PHOTON, phys::ParticleType::NEUTRON},
                                                            count, false);
    auto matMap = materials->materialMapByName();
    for(auto &p:matMap) {
        mDebug() << "matmap name===" << p.first;
    }
    // 名前-材料 マップのテスト
    for(auto &material: matMap) {
        // firstが入力ファイル中の名前
        mDebug() << "materian iput-name=" << material.first
                 << ", material id=" << material.second->id();
        // nucsはvector<vector<pair<shared_ptr<const Nuclide>, double>>>
        auto nucs = material.second->nuclides();
        for(size_t i = 1; i < nucs.size(); ++i) {
            auto ptype = static_cast<phys::ParticleType>(i);
            mDebug() <<"	" << phys::particleTypeTostr(ptype) << "lib";
            for(auto &nucPair: nucs.at(i)) {
                mDebug() << "\t\t" << nucPair.first->zaid() << nucPair.second;
            }
        }
    }
    //mDebug() << "name-ID map=\n" << Material::nameIdMap();

    QCOMPARE(matMap.size(), size_t(4));
    QVERIFY_EXCEPTION_THROWN(matMap.at("5"), std::out_of_range);

    std::vector<phys::ParticleType> nptypes {phys::ParticleType::PHOTON, phys::ParticleType::NEUTRON};

    auto  refHydrogen = Material::createMaterial("refHydrogen", ++mid, xsdir_,nptypes,
                                                 std::vector<std::string>{"1001", "1"},
                                                 std::unordered_map<std::string, std::string>{std::make_pair("plib", "84p")},"");
    auto  refWater = Material::createMaterial("refWater", ++mid, xsdir_, nptypes,
                                                 std::vector<std::string>{"1001", "2", "8016", "1.0"},
                                              std::unordered_map<std::string, std::string>{std::make_pair("plib", "84p")}, "");
    auto refIron =  mat::Material::createMaterial("refIron", ++mid, xsdir_,
                                                  nptypes,
                                                  std::vector<std::string>{"26054", "0.058", "26056", "0.9172", "26057", "0.022", "26058", "0.0028"},
                                                  std::unordered_map<std::string, std::string>{std::make_pair("plib", "84p")},
                                                  "");
    std::vector<double> energies{0.011, 0.4, 0.5, 0.9, 1.1, 5, 5.5, 9};
    double dens = 7.8;

    for(auto ptype: nptypes) {
        for(auto ene: energies) {
            QVERIFY(std::abs(refHydrogen->macroTotalXs(ptype, dens, ene) - matMap["hydrogen"]->macroTotalXs(ptype, dens, ene))
                    < 5e-2/refHydrogen->macroTotalXs(ptype, dens, ene));
            QVERIFY(std::abs(refWater->macroTotalXs(ptype, dens, ene) - matMap["water3"]->macroTotalXs(ptype, dens, ene))
                    < 5e-2/refWater->macroTotalXs(ptype, dens, ene));
            QVERIFY(std::abs(refWater->macroTotalXs(ptype, dens, ene) - matMap["4"]->macroTotalXs(ptype, dens, ene))
                    < 5e-2/refWater->macroTotalXs(ptype, dens, ene));
            QVERIFY(std::abs(refIron->macroTotalXs(ptype, dens, ene) - matMap["1"]->macroTotalXs(ptype, dens, ene))
                    < 5e-2/refIron->macroTotalXs(ptype, dens, ene));
        }
    }
}

/*
 * ostream << vec<T>は定義済み(ostream << Tで出力)なので
 * std::ostream << pair<shared_ptr<Nuclide>, double>が必要。
 *
 * 1．os << Nuclide を定義し、
 * 2．os << pari<shared<Nuc>, double>  を  os << *(first.get()) << second で定義
 */

QTEST_APPLESS_MAIN(MaterialTest)

#include "tst_materialtest.moc"
