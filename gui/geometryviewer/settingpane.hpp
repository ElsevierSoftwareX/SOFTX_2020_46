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
#ifndef SETTINGPANE_HPP
#define SETTINGPANE_HPP

#include <array>
#include <unordered_map>
#include <QWidget>
#include <vtkAxesActor.h>
#include <vtkCubeAxesActor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include "geometryviewerconfig.hpp"

//class AuxPlaneInfo;
class CellPane;


namespace geom{
class BoundingBox;
}

namespace Ui {
class SettingPane;
}


/* GeometryViewerの左側に表示するSettingペイン
 * ・描画領域の取得、推定結果の反映
 * ・補助平面の管理
 * ・AxesArrowの管理
 * ・AxesCubeの管理
 *
 */

class SettingPane : public QWidget
{
	Q_OBJECT
	friend class GeometryViewer;
public:
	static const int DEFAULT_BB_SIZE;
	SettingPane(QWidget *parent, vtkSmartPointer<vtkRenderer> renderer);
	~SettingPane();
	void retranslate();

    // maxbbを描画領域としてセットし、自動的に解像度を設定する。setRegionByBB, updateAxes,
	void updatePane(const geom::BoundingBox &maxbb);

	//! @brief 描画領域データを第二引数に代入する。取得失敗の場合はfalseを返す。第一引数がtrueなら失敗時警告する
	bool getRenderingRangeInput(bool warn, std::array<double, 6> *rangeArray) const;

	//! @brief 現在の(widgetに入力されている領域)設定でcubeAxes表示(非表示)する。矢印とグリッドは分離させた。
	void showCubeAxes(bool on);

	//! @brief 現在の設定でarrowを表示(非表示)する
	void showArrows(bool on);

    //! @brief 現在の設定で凡例を表示(非表示)する
    void showLegend(bool on);


	//! @brief 描画領域をlineEdit群にセット
	void setRenderingRegion(const std::array<double, 6> &arr);
//    //! @brief 分解能
//    void setAutomaticResolution();
	//! @brief BB領域をこのペインのRegion設定lineEditにセットする。
	void setRegionByBB(const geom::BoundingBox &maxbb);
	//! @brief 現在settingペインに入力されている設定を返す
	GeometryViewerConfig getGeomConfig();


	//! @brief スクロールバーが不要な最小サイズを返す
	int minimumNoScrollWidth() const;

	//! @brief 補助平面をアップデートする。カットを伴わない補助平面を可視/非可視化する。
	void updateAuxPlanes();

	void clear();
	vtkSmartPointer<vtkAxesActor> getArrowActor(){return arrowAxesActor_;}  //DEBUG

signals:
	void axesConfigChanged();
	void requestAutoCalcRegion();
	void requestDisableAutoResolution();
	void requestDisableAutoRegion();
	void requestUpdateQVTKWidget(); // GeometryViewerにQVTKWidgetをアップデートしてもうらうためのシグナル。

private:
	Ui::SettingPane *ui;
	vtkSmartPointer<vtkRenderer> renderer_;
	// 座標軸矢印のactor
	vtkSmartPointer<vtkAxesActor> arrowAxesActor_;
	// 座標ボックスのactor
	vtkSmartPointer<vtkCubeAxesActor> cubeAxesActor_;
	// 補助平面のactor
	std::array<vtkSmartPointer<vtkActor>, 3> auxPlaneActors_;

	std::array<double, 6> renderingRange() const;
	//! @brief 現在入力されているセルあたりのサンプリング点数を返す。
	size_t numberOfPoints();



private slots:
	void setEnableArrowConfig(bool state);  // arrow非表示の時はarrow設定widtgetは隠す
	void emitDisableAutoResolution(); // designerでsignal-signal接続できないのでprivate slotを噛ませる
	void emitDisableAutoRegion();
};

#endif // SETTINGPANE_HPP
