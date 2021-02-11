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
#include "xsviewer.hpp"
#include "ui_xsviewer.h"

#include <cmath>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <unordered_map>

#include <QDir>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QPalette>
#include <QString>
#include <QSurfaceFormat>

#include <vtkVersion.h>

// See http://vtk.1045678.n5.nabble.com/Error-no-override-found-for-vtkContextDevice2D-td5741533.html
#include <vtkAutoInit.h>

// VTK version dependency
#if VTK_MAJOR_VERSION < 9
    // VTK8
    VTK_MODULE_INIT(vtkIOExportOpenGL2)
#endif
VTK_MODULE_INIT(vtkRenderingContextOpenGL2)
VTK_MODULE_INIT(vtkRenderingOpenGL2)
VTK_MODULE_INIT(vtkRenderingGL2PSOpenGL2)
VTK_MODULE_INIT(vtkInteractionStyle)
VTK_MODULE_INIT(vtkRenderingFreeType)
#include <vtkAxis.h>
#include <vtkColor.h>
#include <vtkBrush.h>
#include <vtkChartLegend.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkDoubleArray.h>
#include <vtkGL2PSExporter.h>
#include <vtkFloatArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkMath.h>
#include <vtkSmartPointer.h>
#include <vtkPen.h>
#include <vtkPlot.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>

#include "chartconfigdialog.hpp"
#include "nuclidetablewidget.hpp"
#include "xsiteminfo.hpp"
#include "../fileconverter.hpp"
#include "../globals.hpp"
#include "../subdialog/messagebox.hpp"
#include "../../core/simulation.hpp"
#include "../../core/material/materials.hpp"
#include "../../core/material/nuclide.hpp"
#include "../../core/utils/message.hpp"
#include "../../component/libacexs/libsrc/mt.hpp"
#include "qvtkopenglwrapperwidget.hpp"

// 参考 https://www.vtk.org/gitweb?p=VTK.git;a=blob;f=Examples/Charts/Cxx/QChartTable.cxx
// 3Dは Charts/Core/Testing/Cxx/TestSurfacePlot.cxx .



/*
 * vtkChartをQVTKOpenGLWidgetに追加するには？
 *
 * cf. 3Dの場合
 * 1.QVTKOpenGLWitにRenderWindowを追加し、(qvtkWidget_->SetRenderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New());)
 * 2.そのRenderWindowにレンダラを追加し、	(qvtkWidget_->GetRenderWindow()->AddRenderer(renderer_);)
 * 3．そのレンダラにActorを追加する。(renderer_->AddActor(…);)
 *
 * vtkFloatArrayに値をセット(vtkFloatArray::SetName())
 * vtkTableの列に加える(vtkTable::AddColumn())デフォルトでは最初の列がx座標になる
 * vtkTable::SetNumberOfRows()で行数をセット
 * vtkTable::SetValue(row, col, 値)で値をセット
 * vtkChartXYを作成
 * contextviewのsceneにAddItemでチャートを追加
 * vtkPlotを作成してvtkChartに追加
 * vtkPlotにtableを追加
 *
 * データ→vtkFloatArray→vtkTable→vtkPlot→vtkChartXY→vtkContextScene→vtkContextView→vtkGenericOpenGLRenderWindow→QVTKOpenGLWidget
 * という流れになっている。
 *
 * 方針：
 * ・QVTKOpenGLWidget::GetRenderWindow,  vtkRenderWindow::GetInteractorなどを利用して、なるべく左辺値変数の生成を避ける。
 */

namespace {
const double MINIMUM_VALUE = 1e-30;

// 入力された文字列と軸から適切なrangeを設定する。値が不適切ならrange==falseとなる。
// minStr, maxStrに入力されている文字列と軸から適切なrangeを設定する。対数の扱いは後の軸にデータ・セットする時にチェック。
void getMinMaxRange(QWidget *parent, const QString &minStr, const QString &maxStr,
			  vtkAxis *ax, std::shared_ptr<std::pair<double, double>> &range)
{
	if(!minStr.isEmpty() || !maxStr.isEmpty()) {
		try {
			double minValue, maxValue;
			bool minOk = true, maxOk = true;
			minValue = (minStr.isEmpty()) ? ax->GetUnscaledMinimum() : minStr.toDouble(&minOk);
			maxValue = (maxStr.isEmpty()) ? ax->GetUnscaledMaximum() : maxStr.toDouble(&maxOk);
			std::stringstream ss;
			if(minValue > maxValue) {
				ss << "Axis min value=" << minValue << " is larger than max value=" << maxValue;
			} else if(!minOk) {
				ss << "Axis minimum input=" << minStr << " is not numeric";
			} else if(!maxOk) {
				ss << "Axis maximum input=" << maxStr << " is not numeric";
			}
			if(!ss.str().empty()) throw std::invalid_argument(ss.str());
			range.reset(new std::pair<double, double>(minValue, maxValue));
		} catch (std::invalid_argument &ia) {
            GMessageBox::warning(parent, "Invalid axis range inputs", ia.what(), true);
		}
		return;
	} else {
		// minmax両方空なので何もしない。
		//mDebug() << "minmax両方空なので何もしない";
		return;
	}
}

}

// current/adding/removingTargets_ではxsItemInfoのfactorメンバは区別しない。
// 同核種違Factorのプロットを出さないため。
XsViewer::XsViewer(QWidget *parent, const QString &tabText, const GuiConfig *gconf)
	:TabItem(parent, tabText, gconf), 	ui(new Ui::XsViewer),
	  addingTargets_(xsInfoLessWithoutFactor),
	  removingTargets_(xsInfoLessWithoutFactor),
	  contextView_(vtkSmartPointer<vtkContextView>::New()),
	  chart_(vtkSmartPointer<vtkChartXY>::New()),
	  backgroundBrush_(vtkSmartPointer<vtkBrush>::New()),
	  tables_(xsInfoLessWithoutFactor)
{
	ui->setupUi(this);
	ui->toolBoxNuclide->removeItem(0);  // designerで作成した1ページ目は削除

//	QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());
//    QSurfaceFormat::setDefaultFormat(QVTKOpenGLStereoWidget::defaultFormat());
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLWrapperWidget::defaultFormat());
    ui->xsVtkWidget->SetRenderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New());
    contextView_->SetRenderWindow(ui->xsVtkWidget->GetRenderWindow());
    contextView_->SetInteractor(ui->xsVtkWidget->GetRenderWindow()->GetInteractor());

	chart_->GetAxis(vtkAxis::BOTTOM)->SetTitle(chartConfig_.xtitle().toStdString());
	chart_->GetAxis(vtkAxis::LEFT)->SetTitle(chartConfig_.ytitle().toStdString());

	/*
	 * NOTE GetLabelFormat()を機能させるにはnotation と tick algorithmの設定が必要
	 * https://stackoverflow.com/questions/45692385/vtkaxis-remove-decimal-points-from-axis-notation
	 */
	chart_->GetAxis(vtkAxis::LEFT)->SetNotation(vtkAxis::PRINTF_NOTATION);
	chart_->GetAxis(vtkAxis::LEFT)->SetTickLabelAlgorithm(vtkAxis::TICK_SIMPLE);
	chart_->GetAxis(vtkAxis::LEFT)->SetLabelFormat("%4.0e");
	chart_->GetAxis(vtkAxis::BOTTOM)->SetNotation(vtkAxis::PRINTF_NOTATION);
	chart_->GetAxis(vtkAxis::BOTTOM)->SetTickLabelAlgorithm(vtkAxis::TICK_SIMPLE);
	chart_->GetAxis(vtkAxis::BOTTOM)->SetLabelFormat("%4.0e");
	// label, tickの数の最適化をしたいが、マウス操作で変化するのでQVTKWidgetを継承して
	// マウスイベントのハンドラ等を書かないとだめ。面倒なので限りなく後回し。
	// chart_->GetAxis(vtkAxis::LEFT)->SetLabelOffset(20); // これは軸とラベルの間隔(pixel)
	// chart_->GetAxis(vtkAxis::LEFT)->SetNumberOfTicks(5);  // 動作しない
	/*
	 * NOTE vtkAxis::SetNumberOfTicks()は実装されていない。
	 * ・https://www.vtk.org/pipermail/vtk-developers/2010-November/024454.html
	 * vtkAxis::vtkAxis::SetCustomTickPositionsを使うべし
	 *
	 * NOTE chartをdraggableにしたい。が,ver6以降ではSetDragEnabledは機能しない。代わりにSet*Alignment()を使う
	 * https://www.vtk.org/pipermail/vtkusers/2016-January/093867.html
	 * chart_->GetLegend()->SetDragEnabled(true);
	 */

    chartConfig_.applyToChart(chart_);
	contextView_->GetScene()->AddItem(chart_);


    // Signal/Slot
    connect(ui->pushButtonUpdatePlot, &QPushButton::clicked, this, &XsViewer::updatePlot);
    connect(ui->pushButtonSetRange, &QPushButton::pressed, this, &XsViewer::applyRanges);
    connect(ui->pushButtonClearCheckbox, &QPushButton::clicked, this, &XsViewer::clearAllCheckBoxes);
    connect(ui->pushButton, &QPushButton::pressed, this, &XsViewer::editConfig);
    connect(ui->checkBoxXlog, &QCheckBox::toggled, this, &XsViewer::enableXlog);
    connect(ui->checkBoxYlog, &QCheckBox::toggled, this, &XsViewer::enableYlog);
    connect(ui->checkBoxShowLegend, &QCheckBox::toggled, this, &XsViewer::enableLegend);

}


XsViewer::~XsViewer() {delete ui;}

void XsViewer::retranslate()
{
	ui->retranslateUi(this);
	// ui以外で生成したクラスは明示的にretranslateしないといけない。
	for(auto &nucTable: nuclideTables_) {
		nucTable->retranslate();
	}
}



void XsViewer::exportToRasterGraphics()
{
	try {
		ff::exportRenderWindowToRasterGraphics(ui->xsVtkWidget->GetRenderWindow());
	} catch (std::exception &e) {
		mWarning() << e.what();
	}
}


void XsViewer::clear()
{
	simulation_.reset();
	// nuclideTableWidgetのクリアはややこしい。
	// 1. NuclideTableWidget自体の中身のクリア
	// 2. NuclideTableWidgetを保持しているnuclideTables_のクリア
	// 3. toolBoxのクリア
	// 4. NuclideTableWidgetのインスタンスdelete
	// 特に4．が重要.parentをセットしているのでXsViewerがデストラとされるときには自動開放されるが、
	// 核種テーブルのみをクリアするときには個別にdeleteする必要がある。
	for(size_t i = 0; i < nuclideTables_.size(); ++i) {
		nuclideTables_.at(i)->clear();
		delete nuclideTables_.at(i);
	}

	nuclideTables_.clear();
	int numBox = ui->toolBoxNuclide->count();
	for(int i = 0; i < numBox; ++i) {
		ui->toolBoxNuclide->removeItem(0);  // 削除するたびにcountもindexもずれるのでindex=0固定で先頭から削除する。
	}
	addingTargets_.clear();
	removingTargets_.clear();
	tables_.clear();
//	// VTK関連のクリア
	chart_->ClearPlots();
}


// simulationデータが変更されたらここのスロットが実行される
void XsViewer::updateSimulationData(std::shared_ptr<const Simulation> sim)
{
	if(!sim) return;
	this->clear();
	simulation_ = sim;

	auto materials = simulation_->getMaterials();

	// まずparticle typeを知る必要がある。
	auto ptypes = materials->particleTypes();

	// ptypeをキーにした 核種一覧。vectorのNuclideは重複しないように、追加前にnuclide名を検索する必要がある。
	// このマップはmaterialsからmaterial一覧を受け取って、一覧をスキャンして作成する。
	// Tableクラスはvector部分を受け取って表を作成する。


	auto materialMap = materials->materialMapByName();  // 材料名をキーとしたMaterialスマポへのマップ

	// nuclidesMap2が全データ格納可能なようにリサイズする。
	int maxPtype = 0;
	for(auto &ptype: ptypes) {
		if(maxPtype < static_cast<int>(ptype)) maxPtype = static_cast<int>(ptype);
	}
	nuclidesMap2_.resize(static_cast<size_t>(maxPtype)+1);

	for(auto &ptype: ptypes) {
		//std::vector<std::shared_ptr<const mat::Nuclide>> nucVec;
		std::unordered_map<std::string, std::shared_ptr<const mat::Nuclide>> nucMap; // keyはzaid

		for(auto &matPair: materialMap) {
			/*
			 * nuclides() 核種データは vector<vector<pair<sahred<nuclide>, ratio>>>で
			 * nuclides[ParticleType].at[index].first で核種データを
			 * nuclides[ParticleType].at[index].second でその核種の質量存在割合を
			 * 返す。 indexは材料構成核種のインデックス。1材料1核種ならindex=0のみ
			 */
			auto nuclides = matPair.second->nuclides();  // matPair.secondがshared_ptr<Material>
			for(auto &nPair: nuclides.at(static_cast<size_t>(ptype))) {
				auto nuc = nPair.first;
				nucMap[nuc->zaid()] = nuc;
			}
		}

		NuclideTableWidget* table = new NuclideTableWidget(ptype, this);

		table->setNuclides(nucMap);
		ui->toolBoxNuclide->addItem(table, QString::fromStdString(table->particleName()));
		nuclideTables_.push_back(table);


		//nuclidesMap_.emplace(ptype, std::move(nucVec));
		nuclidesMap2_.at(static_cast<int>(ptype)) = nucMap;

		connect(table, &NuclideTableWidget::itemChanged, this, &XsViewer::handleItemChanged);
	}

	// サイズ調整
    int paneWidth = (nuclideTables_.empty() || !nuclideTables_.front())?
                ui->toolBoxNuclide->width() : nuclideTables_.front()->minRequiredWidth();
	ui->splitter->setSizes(QList<int>{paneWidth, this->size().width() - paneWidth});
}

void XsViewer::exportToVectorGraphics()
{
	// まず表示内容があるかをチェック
    if(chart_->GetNumberOfPlots() == 0){
        GMessageBox::warning(this, tr("Export chart"), tr("No cross section data is plotted."), true);
        return;
    }
	// 次にファイル名を取得
	QFileInfo finfo(QFileDialog::getSaveFileName(this, tr("Select exporting (vector) file name(eps, ps pdf, svg, tex)")));
	auto format = ff::strToFormat2DV(finfo.suffix().toStdString());


	// エクスポート時にあと枠からチャートがはみ出る。これvtkに手を入れないと多分無理。あるいはデータをカットしてしまうとか
	// tableを複製してはみ出る部分をカットし、再プロットする。そしてエクスポート終了後は元のtableでプロットし直す。
	// → データカット方式はx方向のカットは良いが、y方向カットはsin関数みたいにうねっている箇所が境界に来た時に対応不可
	//   (ショートカットして変なところで繋がってしまう。)
	// とりあえずxカット方式だけ実装してみる。
	auto xax = chart_->GetAxis(vtkAxis::BOTTOM), yax = chart_->GetAxis(vtkAxis::LEFT);
	std::pair<double, double> xrange(xax->GetUnscaledMinimum(), xax->GetUnscaledMaximum());
	std::pair<double, double> yrange(yax->GetUnscaledMinimum(), yax->GetUnscaledMaximum());
	chart_->ClearPlots();

	// ここからデータカット
	std::map<XsItemInfo, vtkSmartPointer<vtkTable>, compare_type> tmpTab(xsInfoLessWithoutFactor);
	for(auto it = tables_.cbegin(); it != tables_.cend(); ++it) {
		auto newTable = vtkSmartPointer<vtkTable>::New();
		const XsItemInfo info = it->first;
		const vtkSmartPointer<vtkTable> &table = it->second;
		// table初期化
		auto arrX = vtkSmartPointer<vtkFloatArray>::New();
		arrX->DeepCopy(table->GetColumn(0));
		newTable->AddColumn(arrX);
		auto arrY = vtkSmartPointer<vtkFloatArray>::New();
		arrY->DeepCopy(table->GetColumn(1));
		newTable->AddColumn(arrY);
		// ここでtableから指定のレンジ外成分をカットしてnewTableに格納する。
		// まずtableをスキャンして必要な点数を求める。
		// y方向カットはグラフが波打っている時に内容が崩れるので実装しない
		int numPoints = 0;
		for(int i = 0; i < table->GetNumberOfRows(); ++i) {
			double xval = table->GetValue(i, 0).ToDouble();
			if(xval >= xrange.first && xval <= xrange.second) ++numPoints;
		}
		newTable->SetNumberOfRows(numPoints);
		int n = 0;
		for(int i = 0; i < table->GetNumberOfRows(); ++i) {
			double xval = table->GetValue(i, 0).ToDouble();
			if(xval >= xrange.first && xval <= xrange.second) {
				newTable->SetValue(n, 0, table->GetValue(i, 0));
				newTable->SetValue(n, 1, table->GetValue(i, 1));
				++n;
			}
		}
		tmpTab.emplace(info, newTable);
	}
	plotDataTables(tmpTab);
	xax->SetUnscaledRange(xrange.first, xrange.second);
	yax->SetUnscaledRange(yrange.first, yrange.second);
	xax->SetBehavior(vtkAxis::FIXED);
	yax->SetBehavior(vtkAxis::FIXED);
	contextView_->GetInteractor()->Render();

	auto exporter = vtkSmartPointer<vtkGL2PSExporter>::New();
	exporter->SetRenderWindow(ui->xsVtkWidget->GetRenderWindow());

	switch(format) {
	case ff::FORMAT2DV::NOT_DEFINED:
        GMessageBox::warning(this, tr("Warning"), tr("Select eps/ps/pdf/svg/tex file name."), true);
		return;
		break;
	case ff::FORMAT2DV::EPS:
		exporter->SetFileFormatToEPS();
		break;
	case ff::FORMAT2DV::PS:
		exporter->SetFileFormatToPS();
		break;
	case ff::FORMAT2DV::PDF:
		exporter->SetFileFormatToPDF();
		break;
	case ff::FORMAT2DV::SVG:
		exporter->SetFileFormatToSVG();
		break;
	case ff::FORMAT2DV::TEX:
		exporter->SetFileFormatToTeX();
		break;
	}
	QString baseName = finfo.absolutePath() + "/" + finfo.completeBaseName();  // 実はWindowsで/セパレータは使える。DOSで使えないだけ
	exporter->SetFilePrefix(baseName.toStdString().c_str());
	exporter->Write();

	// plotの復元
	chart_->ClearPlots();
	plotDataTables(tables_);
	xax->SetUnscaledRange(xrange.first, xrange.second);
	yax->SetUnscaledRange(yrange.first, yrange.second);
	xax->SetBehavior(vtkAxis::FIXED);
	yax->SetBehavior(vtkAxis::FIXED);
}


// チェックのついているMTのみcsvにエクスポートする。
void XsViewer::exportToTextData()
{
	/*
	 * XsViewr::nuclidesMap2_はvector<map<string, shared_ptr<Nuclide>>> Nuclideクラスを保持。
	 *  nuclidesMap2_.at(particle).at(ZAID)でNuclideデータが得られる。
	 * XsViewr::tables_はmap<XsItemInfo, vtkSmartPointer<vtkTable>>で
	 *  XsItemInfoをキーにしてデータにアクセスする。
	 *
	 * ・(identifierを含む)フルのZAIDには入射粒子情報が含まれているので、particle情報は重複する。
	 * ・particle情報を別キーにした方がわかりやすい場合とわかりにくい場合がある。
	 *
	 *
	 * NuclideTableWidgetのTreeWidgetItemは(topItemを除き)zaidを持っていないので、
	 * parentに問い合わせる必要がある。
	 *
	 *
	 */
	const QDir dir = QFileDialog::getExistingDirectory(this, tr("select an exporting directory"), ".");
	if(dir.isEmpty()) return;
	for(const auto& nuclideTable: nuclideTables_) {
		auto particleType = nuclideTable->ptype();
		auto checkedItems = nuclideTable->checkedItems();
		for(const auto& item: checkedItems) {
			std::string zaidStr = item->parent()->text(NuclideTableWidget::COLUMN_ZAID).toStdString();
			std::string mtStr = item->text(NuclideTableWidget::COLUMN_MT).toStdString();
			auto nuclide = nuclidesMap2_.at(static_cast<int>(particleType)).at(zaidStr);
			ace::CrossSection xs = nuclide->xsMap().at(ace::mt::toReaction(mtStr));
			std::string fileName = dir.absolutePath().toStdString() + "/" + zaidStr + "." + mtStr + ".dat";
			std::ofstream ost(fileName.c_str());
			xs.dump(ost);
		}
	}
}

void XsViewer::plotDataTables(const std::map<XsItemInfo, vtkSmartPointer<vtkTable>, compare_type> &tables)
{
	for(auto it = tables.cbegin(); it != tables.cend(); ++it) {
		vtkPlot *line = chart_->AddPlot(vtkChart::LINE);
		line->SetInputData(it->second, 0, 1);  // col=0列がx座標, col=1列がy座標
		chartConfig_.applyLineStyle(std::distance(tables.cbegin(), it), line);
	}
}

// mt番号はやたら多いので断面積表をスキャンしてプロットする/しないを選択するのは無駄が多い。
// よってitem状態が変わったものだけを選択する（あるいはチェックが外れていれば選択しない）
void XsViewer::handleItemChanged(phys::ParticleType ptype, QTreeWidgetItem *item, int column)
{
	if(column == NuclideTableWidget::COLUMN_ZAID) return;
	std::string mtStr = item->text(NuclideTableWidget::COLUMN_MT).toStdString();
	XsItemInfo info(ptype,
					item->parent()->text(NuclideTableWidget::COLUMN_ZAID),
					ace::mt::toNumber(mtStr),
					item->text(NuclideTableWidget::COLUMN_FACTOR).toDouble());

	//mDebug() << "Changed item info, ZAID===" << info.zaid << "MT=" << info.mt << ", factor===" << info.factor;


	if(item->checkState(NuclideTableWidget::COLUMN_PLOT) == Qt::Checked) {
		// 変更されたアイテムにチェックがついている場合、removingTargets_に同データがあれば削除し、その後にaddingTargets_に加える。
		for(auto it = removingTargets_.begin(); it != removingTargets_.end(); ++it) {
			if(isSameInfoWithoutFactor(*it, info)) {
				removingTargets_.erase(it);
				break;
			}
		}
		// 後から変更されたアイテムinfoの方を追加しないとダメなので、検索して見つかれば上書き。そうでなければemplace
		auto addingIt = addingTargets_.find(info);
		if(addingIt != addingTargets_.end()) addingTargets_.erase(addingIt);
		addingTargets_.emplace(std::move(info));
		//mDebug() << "Info added to addingTargets_";

	} else {
		// 変更されたアイテムにチェックが無い場合、addingTargets_に同データがあれば削除し、その後にremovingTargets_に加える。
		for(auto it = addingTargets_.begin(); it != addingTargets_.end(); ++it) {
			if(isSameInfoWithoutFactor(*it, info)) {
				addingTargets_.erase(it);
				break;
			}
		}
		// removingTargets_に加えるが,factor以外が同じ要素があれば上書きする必要があるので、検索・削除してから追加。
		auto remIt = removingTargets_.find(info);
		if(remIt != removingTargets_.end()) removingTargets_.erase(remIt);
		removingTargets_.emplace(std::move(info));
		//mDebug() << "Info added to removingTargets_";
	}
}

/*
 * addingTargets_はこれから追加するアイテムを
 * removingTargets_はこれから削除するアイテムを
 * 保持している。
 */
void XsViewer::updatePlot(bool autoScale)
{
	if(removingTargets_.empty() && addingTargets_.empty() && chartConfig_.haveApplied()) return;

	chart_->ClearPlots();
	auto xax = chart_->GetAxis(vtkAxis::BOTTOM), yax = chart_->GetAxis(vtkAxis::LEFT);
	xax->SetTitle(chartConfig_.xtitle().toStdString());
	yax->SetTitle(chartConfig_.ytitle().toStdString());
	xax->SetLogScale(ui->checkBoxXlog->isChecked());
	yax->SetLogScale(ui->checkBoxYlog->isChecked());
	chart_->SetShowLegend(ui->checkBoxShowLegend->isChecked());

	//  ここで現在のターゲットからremovingTargetを除く
	for(auto &rinfo: removingTargets_) {
		auto it = tables_.find(rinfo);
		// tables_のinfoキー比較ではfactorは考慮しない
		if(it != tables_.end()) tables_.erase(it);
	}
	removingTargets_.clear();

    // autoscale している場合、新たな丸め下限値で理スケールされてしまうので意味なし。
    auto min_value = MINIMUM_VALUE;
	// ここで現在の表示ターゲットにaddingTargets_を加える。
	for(auto &info: addingTargets_) {
		auto table = vtkSmartPointer<vtkTable>::New();
		auto arrX = vtkSmartPointer<vtkFloatArray>::New();
		arrX->SetName("Energy");
		table->AddColumn(arrX);
		auto arrY = vtkSmartPointer<vtkFloatArray>::New();
		std::string title = info.zaid.toStdString() + ": " +  ace::mt::description(info.mt);
        if(std::abs(info.factor - 1) > 1e-6) {
			// factorは有効4桁で文字列化
			title += "  x" + QString::number(info.factor, 'g', 4).toStdString();
        }
        arrY->SetName(title.c_str());
		table->AddColumn(arrY);

		std::shared_ptr<const mat::Nuclide> nuclide = nuclidesMap2_.at(static_cast<int>(info.ptype)).at(info.zaid.toStdString());
		// 念の為MT番号に対応した断面積データがあるかチェックする。
		/*
		 * NOTE IRDF2002-ACEのMT番号は独自拡張されている。
		 * http://www.oecd-nea.org/tools/abstract/detail/iaea0867
		 * (D) ZZ-IRDF-2002-ACE
		 * A library in ACE-dosimetry format for the MCNP family of codes derived from from the pointwise IRDF-2002 data.
		 * The ACE reaction MT* numbers are related to the ENDF MT numbers as MT* = MT +1000*(10+LFS)
		 *  where LFS is the metastable state designator of the reaction product.
		 */
		auto key = ace::mt::toReaction(ace::mt::toMtString(info.mt));
		if(nuclide->xsMap().find(key) == nuclide->xsMap().end()) {
			std::string message = std::string("no value found for key = ") + ace::mt::toMtString(info.mt) + ". valid={";
			for(auto &p: nuclide->xsMap()) {
				message += " " + ace::mt::toMtString(p.first);
			}
			message += "}";
			throw std::out_of_range(message);
		}

		const ace::CrossSection &xs = nuclide->xsMap().at(key);
		table->SetNumberOfRows(xs.epoints.size());
		assert(xs.epoints.size() == xs.xs_value.size());
		for (size_t i = 0; i < xs.epoints.size(); ++i) {
			table->SetValue(i, 0, xs.epoints.at(i));
			// データに負の値が入ると対数軸が適用されなくなるので0は最小値に丸める。
			// table->SetValue(i, 1, xs.xs_value.at(i));
            if(xs.xs_value.at(i) <= min_value) {
                table->SetValue(i, 1, min_value);
			} else {
				table->SetValue(i, 1, xs.xs_value.at(i)*info.factor);
			}
			//mDebug() << "i=" << i << ", E=" << xs.epoints.at(i) << ", value=" << table->GetValue(i, 1);
		}
		// tables_はinfoのfactorを考慮せず同値判定するのでoperator[]を使って上書きする。
		tables_[info] = std::move(table);
//		tables_.emplace(std::move(info), std::move(table));
	}
	addingTargets_.clear();


	// ここからtables_の中身のデータをプロットしていく。
	plotDataTables(tables_);

	// !autoScale →　xy両方自動設定
	// min/max少なくともどちらかに入力がある→その軸は手動設定
	std::shared_ptr<std::pair<double, double>> xrange, yrange;
	if(!autoScale || !ui->lineEditXmin->text().isEmpty() || !ui->lineEditXmax->text().isEmpty()) {
		getMinMaxRange(this, ui->lineEditXmin->text(), ui->lineEditXmax->text(), xax, xrange);
	}
	if(!autoScale || !ui->lineEditYmin->text().isEmpty() || !ui->lineEditYmax->text().isEmpty()) {
		getMinMaxRange(this, ui->lineEditYmin->text(), ui->lineEditYmax->text(), yax, yrange);
	}
    if(xrange) {
        xax->SetUnscaledRange(xrange->first, xrange->second);
		xax->SetBehavior(vtkAxis::FIXED);
	} else {
		xax->SetBehavior(vtkAxis::AUTO);
	}
    if(yrange) {
        double ytmp = (!ui->checkBoxYlog->isChecked() || yrange->first > 0) ? yrange->first : min_value;
        yax->SetUnscaledRange(ytmp, yrange->second);
		yax->SetBehavior(vtkAxis::FIXED);
	} else {
		yax->SetBehavior(vtkAxis::AUTO);
	}

	// Render()を明示的に呼ばないとマウスオーバーなどのイベントが発生するまで(最初の一回は)グラフが更新されない。
	contextView_->GetInteractor()->Render();
    chartConfig_.setHaveApplied(true);
}

void XsViewer::clearAllCheckBoxes()
{
	for(auto &tab: nuclideTables_) {
		tab->clearTableCheckBox();
    }
}

void XsViewer::applyConfig(bool autoScale)
{
    chartConfig_.applyToChart(chart_);
    // 非グラフ領域の設定はvtkChartには含まれないのでここで設定する。
    //BG vtkViewportの色指定は0.0-1.0のdouble vtkBrushは0.0-255.0
    const QColor& bgc = chartConfig_.bgColor();
    ui->xsVtkWidget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()
            ->SetBackground(bgc.redF(), bgc.greenF(), bgc.blueF());
	// applyConfigでスケール変更されると見づらいのでレンジは更新しない。
	updatePlot(autoScale);
}

void XsViewer::editConfig()
{
	static int editingLineNumber = 0;
	ChartConfigDialog *ccd = new ChartConfigDialog(this, chartConfig_, editingLineNumber);
    auto ret = ccd->exec();
    if(ret == QDialog::Accepted) {
        chartConfig_ = ccd->getChartConfig();
		chartConfig_.setHaveApplied(false);
		editingLineNumber = ccd->getEditingLineNumber();
		/* applyConfigの引数はxyのスケールを変更する場合true。
		 * なのでchartConfig_でx/ymin/maxが設定されていなければfalseを与える。
		 * 考えられるのは
		 * 1．完全自動で適当にプロット → 引数 true
		 * 2．完全に変化なし → 引数 false
		 * 3．charConfig_でxmin/maxなどの何れかが設定され、新しい値に手動更新→ 引数はfalseでxmin等のポインタをチェックする。
		 */

		applyConfig(false);
    }
}

void XsViewer::enableXlog(bool b)
{
	/*
	 * NOTE vtkChartの即時update問題。結局vtkChartXY::ClearPlotsするくらいしか回避策は見つからない。
	 * ・http://vtk.1045678.n5.nabble.com/How-to-update-a-2d-plot-vtkChartXY-with-new-data-online-using-C-td5725677.html
	 * QVTKWidgetでも同様の問題。
	 * ・https://stackoverflow.com/questions/49025203/how-to-force-re-rendering-after-adding-point-to-a-vtkplot-in-a-qvtkwidget
	 *
	 * 実際にmouseoverするまで正しい描画がなされない。のでclearplotsを呼ぶ。
	 * chart_->GetPlot(i)でvtkPlot*を保存しておいてもclearPlotsを呼ぶとvtkPlot*の参照先はdeleteされる。
	 * vtkPlotは代入演算子もDeepCopyメソッドも存在しないので内容をコピーすることは不可。
	 *
	 * 結論：chartクリア後に再構築するにはどこかにvtkTableを保存しておく必要がある。
	 */

	// axisの保存と固定をしておかないと再描画時に自動リスケールされる。
	auto xax = chart_->GetAxis(vtkAxis::BOTTOM);
	auto yax = chart_->GetAxis(vtkAxis::LEFT);
	std::pair<double, double> xrange(xax->GetUnscaledMinimum(), xax->GetUnscaledMaximum());
	std::pair<double, double> yrange(yax->GetUnscaledMinimum(), yax->GetUnscaledMaximum());

	chart_->ClearPlots();
	xax->SetLogScale(b);
	plotDataTables(tables_);

	xax->SetUnscaledRange(xrange.first, xrange.second);
	xax->SetBehavior(vtkAxis::FIXED);
	yax->SetUnscaledRange(yrange.first, yrange.second);
	yax->SetBehavior(vtkAxis::FIXED);
	contextView_->GetInteractor()->Render();  // これは多分必須
}

void XsViewer::enableYlog(bool b)
{
	auto xax = chart_->GetAxis(vtkAxis::BOTTOM);
	auto yax = chart_->GetAxis(vtkAxis::LEFT);
	std::pair<double, double> xrange(xax->GetUnscaledMinimum(), xax->GetUnscaledMaximum());
	std::pair<double, double> yrange(yax->GetUnscaledMinimum(), yax->GetUnscaledMaximum());

	chart_->ClearPlots();
	yax->SetLogScale(b);
	plotDataTables(tables_);

	xax->SetUnscaledRange(xrange.first, xrange.second);
	xax->SetBehavior(vtkAxis::FIXED);
	yax->SetUnscaledRange(yrange.first, yrange.second);
	yax->SetBehavior(vtkAxis::FIXED);
	contextView_->GetInteractor()->Render();
}

void XsViewer::enableLegend(bool b)
{
	// legendの表示/非表示はaxisを触らないのでプロット自体の変更はなく、再プロットは不要
    chart_->SetShowLegend(b);
	contextView_->GetInteractor()->Render();
}

void XsViewer::applyRanges()
{
	auto xax = chart_->GetAxis(vtkAxis::BOTTOM);
	auto yax = chart_->GetAxis(vtkAxis::LEFT);
	std::shared_ptr<std::pair<double, double>> xrange, yrange;
	if(!ui->lineEditXmin->text().isEmpty() || !ui->lineEditXmax->text().isEmpty()) {
		getMinMaxRange(this, ui->lineEditXmin->text(), ui->lineEditXmax->text(), xax, xrange);
	}
	if(!ui->lineEditYmin->text().isEmpty() || !ui->lineEditYmax->text().isEmpty()) {
		getMinMaxRange(this, ui->lineEditYmin->text(), ui->lineEditYmax->text(), yax, yrange);
	}
	if(xrange) {
		mDebug() << "xmin, xmax=" <<  xrange->first << ", " << xrange->second;
		xax->SetUnscaledRange(xrange->first, xrange->second);
		xax->SetBehavior(vtkAxis::FIXED);
	} else {
		xax->SetBehavior(vtkAxis::AUTO);
	}
	if(yrange) {
		mDebug() << "ymin, ymax=" <<  yrange->first << ", " << yrange->second;
		double ytmp = (!ui->checkBoxYlog->isChecked() || yrange->first > 0) ? yrange->first : MINIMUM_VALUE;
		yax->SetUnscaledRange(ytmp, yrange->second);
		yax->SetBehavior(vtkAxis::FIXED);
	} else {
		yax->SetBehavior(vtkAxis::AUTO);
	}
	contextView_->GetInteractor()->Render();
}

