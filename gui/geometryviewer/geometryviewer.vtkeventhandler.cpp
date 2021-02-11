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
#include "geometryviewer.hpp"
#include "ui_geometryviewer.h"

#include <QMenu>
#include <QMouseEvent>
#include <QScreen>


#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkRenderWindow.h>


#include "../../core/physics/particleexception.hpp"
#include "../../core/geometry/geometry.hpp"
#include "../../core/geometry/cell/cell.hpp"

namespace {

const int OVERLAY_FONT_PT = 19;
// refPoint_との距離でlessするファンクタ
class PtPtrLess {
	typedef std::unique_ptr<math::Point> PPtr;
public:
	PtPtrLess(const math::Point &ptr): refPoint_(ptr){;}
	bool operator()(const PPtr &p1, const PPtr &p2) {
		return (refPoint_ - *p1.get()).abs() < (refPoint_ - *p2.get()).abs();
	}

private:
	math::Point refPoint_;
};

/*
 * 有効化されている補助平面との交点の内、最も遠いものを返す
 * これは補助平面で領域の画面手前がカットされている場合に
 * ray-tracingを継続する場合につかう。
 */
std::unique_ptr<math::Point>
	getFarestIntersectionWithAuxPlanes(const math::Point &pt,
									   const std::array<PlaneInfo, 3> &planes,
									   const math::Vector<3> &dir)
{
	//mDebug() << "Enter isInVisibleRegion, pt===" << pt.toString() << "dir===" << dir.toString();
	// 可視化領域内かつ補助平面カットされていないことを確認する。
	std::vector<std::unique_ptr<math::Point>> pts; // 補助平面との交点
	for(size_t i = 0; i < planes.size(); ++i) {
		if(!planes.at(i).visible) continue;  // 有効化されていない補助平面は無視。

		double d = math::dotProd((pt - (planes.at(i).pos*planes.at(i).normal)), planes.at(i).normal);
		// d > 0ならptはnormal側なのでd*cuttingが>0なら、点はカット済みの不可視領域にある。
		if(d*planes.at(i).cutting > 0) {
			// ptが不可視領域でかつplaneとpt,dirが交点を持つ場合
			double dn = math::dotProd(dir, planes.at(i).normal);
			if(std::abs(dn) > math::EPS) {
				double t = (planes.at(i).pos - math::dotProd(pt, planes.at(i).normal))/dn;
				pts.emplace_back(std::make_unique<math::Point>(pt + (t+math::Vector<3>::delta())*dir));
			}
		}
	}

	if(!pts.empty()) {
		// 最も遠い点を選択
		std::sort(pts.begin(), pts.end(), PtPtrLess(pt));
		return std::move(pts.back());
	} else {
		return std::unique_ptr<math::Point>();
	}
}

/*
 * 点ptが補助平面の可視領域内にあればtrueを返す。
 * そうでない場合はtrueを返し、 dir方向の次の交点をinterSectionに代入する。
 *
 * TODO 描画領域との内外判定は実施していない。
 */
bool isVisiblePoint(const math::Point &pt, const std::array<PlaneInfo, 3> &planes)
{
	//mDebug() << "Enter isInVisibleRegion, pt===" << pt.toString();
	// 可視化領域内かつ補助平面カットされていないことを確認する。
	for(const auto& plane: planes) {
		if(!plane.visible) continue;  // 有効化されていない補助平面は無視。
		double d = math::dotProd((pt - (plane.pos*plane.normal)), plane.normal);
		// d > 0ならptはnormal側なのでd*cuttingが>0なら、点はカット済みの不可視領域にある。
		if(d*plane.cutting > 0) return false;
	}
	return true;
}

}  // end anonymous namespace




// クリックなどで選択された点を返す。
// 以下のGeometryViewerメンバ変数を使用
// renderer_
// qvtkWidget_
// simulation_
// displayedCellNames_
// geomConfig_.xyzPlaneInfo()
const geom::Cell* GeometryViewer::getPickedCell(const QPoint &pos)
{
	using Vec = math::Vector<3>;
	/*
	 * ev->pos()でQVTKWidget上での相対座標(左上が0,0)を取得可能
	 * 画面の中心位置はCameraInfoなどカメラの位置。
	 * 画面の上側と対応するベクトルもCameraInfoからわかるので
	 * カメラの存在する平面上での座標は取得可能。
	 * これをQVTKWidget上の座標に変換し、クリック点→視点直線とセルの交点を調べれば良い。
	 */

	/*
	 * 方針
	 * 1．マウスクリック点から視錘台のnear面上での座標を求める。
	 * 2. camera-near面上点の延長船上のセル交点を調べる。
	 */
	auto camera = renderer_->GetActiveCamera();
	Vec cameraPos, viewUp, focalPos, normal;
	camera->GetPosition(cameraPos.array().data());
	camera->GetViewUp(viewUp.array().data());
	camera->GetFocalPoint(focalPos.array().data());
	camera->GetViewPlaneNormal(normal.array().data());

	double angleR = math::toRadians(0.5*camera->GetViewAngle()); //, dist = camera->GetDistance();
	double dnear, dfar;
	camera->GetClippingRange(dnear, dfar);
	//		mDebug() << "camera===" << cameraPos << ", viewUp===" << viewUp << ", Focal===" << focalPos << "planeNormal===" << normal;
	//		mDebug() << "angle(deg)===" << math::toDegrees(angleR) << "dnear, dfocal, dfar ==="<< dnear << dist << dfar;

	auto widgetSize = qvtkWidget_->size();
	double widgetAspect = static_cast<double>(widgetSize.width())/widgetSize.height();

	// hvMousePosをVTK中心を原点とするhv座標系へ移す。ややこしいのでv方向をviewupに合わせる。
	int hpos = pos.x() - 0.5*widgetSize.width();
	int vpos = -1*(pos.y() - 0.5*widgetSize.height());
	//		mDebug() << "mouse clicked widget-centered-hvpos===" << hpos << ", " << vpos;

	// near面への投影 normalは手前を向いているのでマイナスする。
	Vec nearCenter = cameraPos - dnear*normal;


	// 画面水平、垂直に対応したベクトル
	Vec vvec = viewUp.normalized(), hvec = math::crossProd(-normal, viewUp).normalized();
	//		mDebug() << "vvec===" << vvec << "hvec===" << hvec;

	double nearHWidthCm = 2.0*dnear*std::tan(angleR);  // dnear*tan(angle)で中心から画面上までの幅
	//		// near面での画面中央上端位置ベクトルと画面中央右端位置ベクトルはこう表す。
	//		Vec nearTopCenter = nearCenter + nearHWidthCm*vvec, nearCenterRight = nearCenter + 0.5*widgetAspect*nearHWidthCm*hvec;
	//		mDebug() << "nearCenter===" << nearCenter << "nearTopCenter===" << nearTopCenter << "nearCenterRight===" << nearCenterRight;

	math::Point p = nearCenter
			+ hpos*widgetAspect*nearHWidthCm/widgetSize.width()*hvec
			+ vpos*nearHWidthCm/widgetSize.height()*vvec;
	//		mDebug() << "hpos===" << hpos << "aspect===" << widgetAspect << "Hf===" << dnear*std::tan(angleR) << "width===" << widgetSize.width();
	//		mDebug() << "vpos===" << vpos << ", HF===" << dnear*std::tan(angleR) << ", wh===" << widgetSize.height()
	//				 << "y=" << static_cast<double>(vpos)/widgetSize.height()*dnear*std::tan(angleR);
	Vec dir = (p - cameraPos).normalized();
	//mDebug() << "point on the near plane===" << p << "dir===" << dir;



	// 2. 交点を求める
	// 光線上に未定義セルがあると例外発生だからエラーハンドリングをする。
	// 全セル総当たりで交差判定すれば復帰できないこともないがそこまでしなくても
	//mDebug() << "Tracing start at p ===" << p.toString() << "dir===" << dir;
	const geom::Cell* cellP = nullptr;
	while(true) {
		// セルを求める
		// ptからdir方向に進んだ時に次にぶつかるセルのスマポを返す。副作用でptは交点+deltaでセル内に入った点まで進む。
		cellP = simulation_->getGeometry()->getNextCell(cellP, dir, &p);
		if(cellP == nullptr) {
			// 交点なしならそれ以上調べても無駄なのでbreak
			break;
		} else if(displayedCellNames_.find(cellP->cellName()) == displayedCellNames_.end()) {
			// 表示中のセルリストに無い(==非表示セル）セルとの交点ならcontinueで続行。その先を調べる。
			continue;
		}


		// ここまでで点Pは可視セルとの交点となっている。あとはそれが可視領域に入っているならOKなのでbreak
		if(isVisiblePoint(p, geomConfig_.xyzPlaneInfo())) break;


		// 可視領域外だった場合にinterSection(補助平面との交点)が求まっている場合、その点の向こう側クリッピング交点候補
		std::unique_ptr<math::Point> interSection = getFarestIntersectionWithAuxPlanes(p, geomConfig_.xyzPlaneInfo(), dir);

		if(interSection) {
			//mDebug() << "補助平面との交点は===" << interSection->toString();
			auto nextCell = geom::Cell::guessCell(simulation_->getGeometry()->cells(), *interSection.get(), false, false);
			if(!nextCell->isUndefined() && displayedCellNames_.find(nextCell->cellName()) != displayedCellNames_.end()) {
				cellP = nextCell.get();
				break;
			}
		}
	}
	return cellP;
}






/*
 * Eventの処理について
 *
 * QVTKWidget上でのマウスイベント(クリック)などは明示的に呼ばなければ
 * 親クラス(GeometryViewer)には伝達されない。
 * はずであるが、QVTKOpenGLWidgetのイベントはvtkRenderWindowInteractorなどで処理されているので、
 * mouseEventを無効化したい場合はvtkRenderWindowInteractorをdisableする、サブクラス化して使う
 * 等の処理が必要になる。
 *
 */


#include <QDebug>
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
void GeometryViewer::handleQVTKMouseEvent(QMouseEvent *ev)
{
	if(!simulation_ || !simulation_->getGeometry()) return; // ジオメトリが読み込まれていないならレイトレーシングしてはならない。

    // FIXME ポリゴン構築キャンセル後に左クリックすると、レイトレーシングではセルがあるのに
    // 該当するアクターが無いためエラーになる。getproperty()を呼んだときに落ちる。

//	if(ev->type() != QEvent::MouseMove) {
//		qDebug() << "MouseEvent !!! type ===" << ev->type() << "button===" << ev->button();
//	}

	if(ev->type() == QEvent::MouseButtonPress
			&& ev->button() == Qt::RightButton
			&& ui->pushButtonEnableCellPicker->isChecked()) {
		const geom::Cell* cellP = nullptr;
		try {
			cellP = getPickedCell(ev->pos());
		} catch (...) {
			return;
		}

		// クリック線上にセルなしなら何もせずリターンする。
		if(cellP == nullptr) return;
		const std::string cellName = cellP->cellName();

		// 選択セルは右クリックでもワイアフレーム化する。
//		selectedActor_ = actorMap_.at(cellP->cellName()).first;
		selectedActor_ = cellObjMap_.at(cellP->cellName()).actor_;
		selectedActor_->GetProperty()->SetRepresentationToWireframe();
		//qvtkWidget_->update();
        qvtkWidget_->GetGenericRenderWindow()->Render();

		QMenu *menu = new QMenu(this->qvtkWidget_);
//		QMenu *menu = new QMenu(this);
		QString menuStr = tr("Hide cell : ");
		menuStr += QString::fromStdString(cellName);
		QAction *act = new QAction(menuStr);
		connect(act, &QAction::triggered,
				this, [=](bool){
						// セルペインのチェックを外し、
						cellPane_->setChecked(cellName, false);
						// 領域を自動変更せずにupdateViewする。
						this->updateView(true);
					 });
		menu->addAction(act);
		menu->exec(ev->globalPos());

		delete act;
		delete menu;
		if(selectedActor_){
			// selectedActor_が更かし状態でもpropertyの変更には問題はない。
			selectedActor_->GetProperty()->SetRepresentationToSurface();  // ワイアフレーム表示終了
			//qvtkWidget_->update(); // いらない。
            qvtkWidget_->GetGenericRenderWindow()->Render();

			// セル消去時にはinteractorを止めないとQVTKOpenGLWidgetの右クリック操作が始まってしまう。
            qvtkWidget_->GetGenericInteractor()->Disable();
		}

	// 左ボタンプレス
	} else if(ev->type() == QEvent::MouseButtonPress
			  && ev->button() == Qt::LeftButton
			  && ui->pushButtonEnableCellPicker->isChecked()) {

		const geom::Cell* cellP = nullptr;
		bool isUndefinedCell = false;
		std::string text;  // QVTKWidget内に表示する情報
		try{
			cellP = getPickedCell(ev->pos());
		} catch (phys::InvalidSource &is) {
			text = "Clicked position is in the undefined region.\n";
			text += "position = " + is.position().toString();
			text += " direction = " + is.direction().toString();
			mWarning() << text;
			isUndefinedCell = true;
		} catch (phys::ParticleException &pe) {
			// messageboxを出すとmouseReleaseイベントがmessageboxへ行ってしまって、
			// qvtkwidget上でのmouseReleaseイベントが発生しなくなり、回転モードを解除できなくなる。よって採用しない。
			text = "Undefined region exists between clicked position and the cells\n";
			text += "position = " + pe.position().toString();
			text += " direction = " + pe.direction().toString();
			mWarning() << text;
			isUndefinedCell = true;
			// 未定義セルに遭遇した場合その情報を表示するので
			// ここではまだリターンしない。
		}
		if(isUndefinedCell && textActor_) if(textActor_) renderer_->RemoveActor2D(textActor_);

		/*
		 *
		 * ここまででcellPの値は
		 * 1．交差セルなし: cellP==nullptr && isUndefinedCell == false
		 * 2  未定義セル cellP == nullptr && isUndefinedCell == true
		 * 3．それ以外: cellP != nullptr && isUndefinedCell == false
		 *
		 * 1と2の区別をするためにisUndefinedCellを利用している。
		 *
		 */

		// 交差しているセルがない場合はそのままリターン
		if(cellP == nullptr && !isUndefinedCell) {
			if(textActor_) renderer_->RemoveActor2D(textActor_);
			return;
		}

		// ここからはセル情報の表示
		//mDebug() << "Cell:" << cellP->toString();
		// 正常にセルが選択された場合。
		if(!isUndefinedCell) {
			std::string polyStr = cellP->polynomialString();
			// 1pt = 1.0/72 inch
			static const double dpi = QApplication::screens().at(0)->logicalDotsPerInch();
			int charWidth = static_cast<double>(OVERLAY_FONT_PT)/72*dpi;
			size_t numCharPerLine = 2*qvtkWidget_->size().width()/charWidth;  // proportionalなら1.5倍位行けるはず。
			// numCharPerLineごとに\nを挿入する。どうせなら少し賢く空白で改行する
			size_t numLines = polyStr.size()/numCharPerLine;
			for(size_t i = 0; i < numLines; ++i) {
				std::string::size_type pos = (i+1)*numCharPerLine + i;
				while(pos < polyStr.size() && polyStr.at(pos) != ' ') {
					//if(pos >= polyStr.size()) break;
					++pos;
				}
				if(pos < polyStr.size()) polyStr.insert(pos, "\n");
			}
			text =  + "Name: " + cellP->cellName() + "\n"
					+ "Material: " + cellP->cellMaterialName() + ", density(g/cc) = " + std::to_string(cellP->density()) + "\n"
					+ polyStr;
			// 選択セルはワイアフレーム化する。
			selectedActor_ = cellObjMap_.at(cellP->cellName()).actor_;
			selectedActor_->GetProperty()->SetRepresentationToWireframe();
		}

		// セル情報をqvtkOpenGLwidgetにオーバーレイして表示する。
		if(textActor_) renderer_->RemoveActor2D(textActor_);
		textActor_ = vtkSmartPointer<vtkTextActor>::New();
		textActor_->SetInput(text.c_str());
		textActor_->SetPosition2 (20,40);
		textActor_->GetTextProperty()->SetFontSize(OVERLAY_FONT_PT);
		if(isUndefinedCell) {
			textActor_->GetTextProperty()->SetColor(0.9, 0, 0);
		} else {
			textActor_->GetTextProperty()->SetColor(0.9, 0.9, 0.9);
		}

		renderer_->AddActor2D (textActor_);
		//qvtkWidget_->update();
        qvtkWidget_->GetGenericRenderWindow()->Render();

	// 左ボタンリリース
	} else 	if(ev->type() == QEvent::MouseButtonRelease && ev->button() == Qt::LeftButton) {
		// vtkSmartPointerが未割り当てのチェックはこれで正しいのか → referenceには書いていないがまあ大丈夫みたい。
		if(selectedActor_){
			selectedActor_->GetProperty()->SetRepresentationToSurface();  // ワイアフレーム表示終了
            qvtkWidget_->GetGenericRenderWindow()->Render();// ここで必要ないのは不思議な気がする。
		}
	}
}





