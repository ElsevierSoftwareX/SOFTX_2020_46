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
#ifndef GEOMETRYVIEWER_HPP
#define GEOMETRYVIEWER_HPP

#include <memory>
#include <set>
#include <sstream>
#include <unordered_map>

#include <vtkCamera.h>
#include <QCheckBox>
#include <QProgressBar>
#include <QTableWidgetItem>
#include <QWidget>
#include <QVector>


#include <vtkSmartPointer.h>
#include <vtkAxesActor.h>
#include <vtkActor.h>
#include <vtkImplicitFunction.h>
#include <vtkTextActor.h>
#include <vtkScalarBarActor.h>
#include <vtkLookupTable.h>


#include "camerainfo.hpp"
#include "cellobject.hpp"
#include "cellpane.hpp"
#include "colorpane.hpp"
#include "geometryviewerconfig.hpp"
#include "customqvtkwidget.hpp"
#include "settingpane.hpp"
#include "../fileconverter.hpp"
#include "../tabitem.hpp"
#include "../../core/simulation.hpp"
#include "../../core/geometry/cell/boundingbox.hpp"


namespace geom {
class Cell;
}



namespace Ui {
class GeometryViewer;
}


struct GuiConfig;


class GeometryViewer : public TabItem
{
	Q_OBJECT
public:
    GeometryViewer(QWidget *parent = nullptr, const QString &tabText = "", const GuiConfig* gconf = nullptr);
    ~GeometryViewer() override;
	void exportTo(ff::FORMAT3D fformat);
	void init() override final;
	void retranslate() override final;
	void exportToRasterGraphics() override final;

public slots:
	void updateSimulationDataX(std::shared_ptr<const Simulation> sim);
	void updateViewColorOnly();  // ポリゴンは生成せずに表示中セルの色だけを変える。
signals:
	void updateGeomViewFinished();


protected:
	void mouseReleaseEvent(QMouseEvent *ev) override;
	void mousePressEvent(QMouseEvent *ev) override;


private:
	Ui::GeometryViewer *ui;
	CellPane *cellPane_;
	SettingPane *settingPane_;
	ColorPane *colorPane_;

	CustomQVTKWidget *qvtkWidget_;
	vtkSmartPointer<vtkRenderer> renderer_;
	std::shared_ptr<const Simulation> simulation_;

	// 左クリック押しっぱなしで選択中のセルのアクター
	vtkSmartPointer<vtkActor> selectedActor_;
    // セル情報表示テキストアクター
	vtkSmartPointer<vtkTextActor> textActor_;
    // 凡例アクター
    std::vector<vtkSmartPointer<vtkTextActor>> legendActors_;



	bool isFirstCall_;  //  QVTKOpenGLWidgetに最初に画像を表示する時の初期化が必要かを管理するフラグ
	std::set<std::string> removingCellNames_;  // 削除対象セル名
	std::set<std::string> addingCellNames_;  // 追加(orサンプリングレート変更)されたセル名
	std::set<std::string> displayedCellNames_;  // 表示中のセル名(ここに記載されているセルがvtkなどへexportされる)

	/*
	 * セル名、陰関数、 vtkActor、refサンプリング点数、 実サンプリング点数 をまとめたクラスを作ればそんなに何個も
	 * map作らなくても良い。
	 */
	// CellObjectのマップへ統合
	// BBと陰関数はsimulationクラスから受け取り次第このマップ内のcellObjectに保存する。
	std::unordered_map<std::string, CellObject> cellObjMap_;

	// 現在のカメラ情報保存変数。 updateViewボタンを押した時にsaveし離した時にloadする
	CameraInfo cameraInfo_;
	// 3D GeometryViewerの設定(サンプリングレート、描画領域、補助平面)を集めたクラス(構造体)
	GeometryViewerConfig geomConfig_;




	// メソッド
	void clear();
	geom::BoundingBox largestBB() const;
	// レンダラから全てのアクターを削除する。
	void removeAllActors();


	// pt可視領域の点かどうかチェックし、不可視領域内であればfalseを返し、次の交点をinterSectionに代入する。
	//bool isInVisibleRegion(const math::Point &pt, const math::Vector<3> &dir, std::unique_ptr<math::Point> &interSection);
	// QVTKWidgetをクリックした点の先にあるセルを返す。
	// 間に未定義領域があって追跡不可能な場合は例外が発生する。
	const geom::Cell *getPickedCell(const QPoint &pos);


private slots:
	void updateView(bool doNotChangeRegion);
	void handleUpdateView();  // qvtkGLRenderingを更新する。
	void saveCameraState();  // 現在のカメラの状態を保存する
	void setCameraInfo(const CameraInfo& cinfo);
	void setZPlusCamera();  // カメラをZ+方向から見た状態へセットする。
	void setZMinusCamera();
	void setYPlusCamera();
	void setYMinusCamera();
	void setXPlusCamera();
	void setXMinusCamera();
	void updateQVTKWidget(bool doResetCamera); // QVTKWidgetをアップデートする関数を呼ぶ
    void showMaterialLegend(bool checked);
    void updateMaterialLegendPosition();  // resizeイベント発生時などに凡例の位置を調整する。
	// 以下のイベントハンドラの実装は geometriviewer.vekeventhandler.cppへ移す。
	// QVTKWidget関連のイベント（mouseMoven等は）このクラスのハンドラ(のオーバーライド)では処理せず、
	// 手動で以下に宣言するハンドラ関数につなぐ。
	void handleQVTKMouseEvent(QMouseEvent *ev); // QVtkOpenGLWidgetのマウスイベントを処理する。

};



#endif // GEOMETRYVIEWER_HPP
