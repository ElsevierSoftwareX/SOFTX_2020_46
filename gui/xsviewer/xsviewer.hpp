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
#ifndef XSVIEWER_HPP
#define XSVIEWER_HPP

#include <memory>
#include <set>
#include <utility>
#include <unordered_map>
#include <vector>

#include <QString>
#include <QTreeWidgetItem>
#include <QWidget>

#include <vtkChartXY.h>
#include <vtkContextView.h>
#include <vtkTable.h>
#include <vtkSmartPointer.h>

#include "chartconfig.hpp"
#include "linestyle.hpp"
#include "../tabitem.hpp"
#include "../core/physics/physconstants.hpp"


class Simulation;
class NuclideTableWidget;
struct GuiConfig;
struct XsItemInfo;


namespace mat{
class Nuclide;
}

class vtkAxis;

namespace Ui {
class XsViewer;
}


class XsViewer : public TabItem
{
	Q_OBJECT
public:
	XsViewer(QWidget *parent = 0, const QString &tabText = "", const GuiConfig *gconf = nullptr);
	~XsViewer();
	virtual void retranslate() override;
	void exportToRasterGraphics() override final;
	void exportToVectorGraphics() override final;
	void exportToTextData();
	void clear();
	//void init() override;

public slots:
	void updateSimulationData(std::shared_ptr<const Simulation> sim);


private:
	using compare_type = bool(*)(const XsItemInfo &xs1, const XsItemInfo &xs2);
	Ui::XsViewer *ui;

    ChartConfig chartConfig_;
	std::shared_ptr<const Simulation> simulation_;
	//入射粒子別に ui->toolBoxNuclide に格納するテーブルクラスをvecで保持
	std::vector<NuclideTableWidget*> nuclideTables_;
	// 核種データのスマポ
	/*
	 * nuclides() 核種データは vector<vector<pair<sahred<nuclide>, ratio>>>で
	 * nuclides[ParticleType].at[index].first で核種データを
	 * nuclides[ParticleType].at[index].second でその核種の質量存在割合を
	 * 返す。 indexは材料構成核種のインデックス。1材料1核種ならindex=0のみ
	 */
	//std::map<phys::ParticleType, std::vector<std::shared_ptr<const mat::Nuclide>>> nuclidesMap_;

	// (int)particletype, zaid をキー/index にしてnuclideスマポが得られるコンテナが欲しい
	std::vector<std::unordered_map<std::string, std::shared_ptr<const mat::Nuclide>>> nuclidesMap2_;

	// XsItemInfoの比較はfactorこみか無しかでややこしくなるのでoperator<でなく明示的にコンストラクタで与える
	std::set<XsItemInfo, compare_type> addingTargets_;
	std::set<XsItemInfo, compare_type> removingTargets_;

	vtkSmartPointer<vtkContextView>contextView_;
	vtkSmartPointer<vtkChartXY> chart_;
	vtkSmartPointer<vtkBrush> backgroundBrush_;  // plot部分のバックグラウンド用ブラシ

	LineStyleCollection lineStyleCollection_;
	/*
	 *  plot用データテーブルコンテナは
	 * 1．(核種、ZAID, TargetInfoなどに因る)順序を保持でき、特定要素の挿入・削除可能であること
	 * 2．個別のtableのサイズは大きいが、コンテナの要素数は小さい
	 * 3．TargetInfoの情報を含み、インデックス/キー等での同一性比較ができること。
	 *
	 * よってTargetInfoをキーにしたstd::mapを使う。
	 * vtkTableはpure virtualなのでスマポを保持する。
	 */
	std::map<XsItemInfo, vtkSmartPointer<vtkTable>, compare_type> tables_;
    // tablesに格納されたデータをプロット
    void plotDataTables(const std::map<XsItemInfo, vtkSmartPointer<vtkTable>, compare_type> &tables);


private slots:
	void handleItemChanged(phys::ParticleType ptype, QTreeWidgetItem *item, int column);
	void updatePlot(bool autoScale = true);
	void clearAllCheckBoxes();
	void applyConfig(bool autoScale);
    void editConfig();
    void enableXlog(bool b);
    void enableYlog(bool b);
    void enableLegend(bool b);
	void applyRanges();
};

#endif // XSVIEWER_HPP
