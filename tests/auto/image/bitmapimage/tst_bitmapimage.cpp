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

#include <string>
#include <vector>


#include "core/utils/utils.hpp"
#include "core/image/bitmapimage.hpp"
#include "core/image/tracingraydata.hpp"



using namespace img;

const char undefName[] = "*C_u";
const char undefBdName[] = "*C_ub";
const char bdName[] = "*C_b";
const char ddefName[] = "*C_d";


class Image2dTest : public QObject
{
	Q_OBJECT

public:
	Image2dTest();

private:
	std::vector<std::pair<std::string, std::string>> cellMatPairs;
	std::vector<double> trackLengths;
	std::vector<std::string> cellNames;
	size_t RESO1, RESO2;
	img::CellColorPalette palette;


private Q_SLOTS:
    void testSimpleCase1();
    void testHorizontalRendering();
    void testVerticalRendering();
    void testBidirectionalRendering();

};

Image2dTest::Image2dTest()
{
    img::BitmapImage a;
    trackLengths = std::vector<double>{10, 10, 30, 20, 30};
    cellNames = std::vector<std::string>{"C1", undefName, "C2", "C3", undefName};
    RESO1 = 100;
    RESO2 = 50;
    cellMatPairs = decltype(cellMatPairs){
            std::make_pair("C1", "m1"),
            std::make_pair(undefName, "void"),
            std::make_pair("C2", "m2"),
            std::make_pair("C3", "m3"),
            std::make_pair("C99", "m99")
    };
    int num = 0;
    for(auto cmpair: cellMatPairs) {
        palette.registerColor(cmpair.first, cmpair.second, img::Color::getDefaultColor(num++));
    }
}


// TracingDataからBitmapImageの作成
void Image2dTest::testSimpleCase1()
{
    const double WIDTH = 100;
    const size_t HSIZE = 100, VSIZE = 100;
    std::vector<std::string> cells{"C99", "C1", "*u", "C99"};
    std::vector<double> tracks0{22.5045, 54.4955, 0.495454, 22.5045};

    std::vector<TracingRayData> hRay{TracingRayData(math::Point{0,0,0},0, cells, tracks0, undefName, undefBdName, bdName), };

    BitmapImage image(DIR::H, HSIZE, VSIZE, WIDTH, WIDTH, hRay, palette);
    mDebug() << "cells=" << cells;
    mDebug() << "tracks0=" << tracks0;
    image.exportToXpmFile("line.xpm");
}

// TracingRayDataからBitmapImageの作成(水平方向走査)
void Image2dTest::testHorizontalRendering()
{
    using namespace img;
    using namespace std;
    //Image2D image(25, 1);
    vector<TracingRayData> rays;
    for(size_t i = 0; i < RESO2; i++) {
        rays.push_back(TracingRayData(
                                    math::Point{0,0,0},
                                    i,
                                    vector<string>{"C1", undefName, "C2", "C3", undefName},
                                    vector<double>{10, 10, 30, 20, 30},
                           undefName, undefBdName, bdName));
    }
    BitmapImage image(DIR::H, RESO1, RESO2, 100, 100, rays, palette);
    image.exportToXpmFile("horizontal.xpm");
}


// TracingRayDataからBitmapImageの作成(垂直方向走査)
void Image2dTest::testVerticalRendering()
{
    std::vector<img::TracingRayData> rays;
    for(size_t i = 0; i < RESO1; i++) {
        rays.push_back(TracingRayData(math::Point{0,0,0}, i, cellNames, trackLengths, undefName, undefBdName, bdName));
    }

    BitmapImage image(DIR::V,RESO1, RESO2, 100, 100, rays, palette);
    image.exportToXpmFile("vertical.xpm");
}


// TracingRayDataからBitmapImageの作成(両方向走査)
void Image2dTest::testBidirectionalRendering()
{
    std::vector<img::TracingRayData> hrays, vrays;
    for(size_t i = 0; i < RESO1; i++) {
        hrays.push_back(TracingRayData(math::Point{0,0,0},i, cellNames, trackLengths, undefName, undefBdName, bdName));
        vrays.push_back(TracingRayData(math::Point{0,0,0},i, cellNames, trackLengths, undefName, undefBdName, bdName));
    }

    BitmapImage vImage(DIR::V, RESO1, RESO1, 100, 100, vrays, palette);
    BitmapImage hImage(DIR::H, RESO1, RESO1, 100, 100, hrays, palette);
//	std::vector<PixelArray::pixel_type> priorIndexes{
//		vImage.palette().getIndexByCellName("*C_ub"),
//		vImage.palette().getIndexByCellName("*C_b")
//	};
//	PixelArray::pixel_type conflictedIndex = vImage.palette().getIndexByCellName("*C_d");



    BitmapImage image = BitmapImage::merge(vImage, hImage, std::vector<std::string>{undefBdName, bdName}, ddefName);
    image.exportToXpmFile("both.xpm");
}




QTEST_APPLESS_MAIN(Image2dTest)

#include "tst_bitmapimage.moc"
