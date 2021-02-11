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
#include "customqvtkwidget.hpp"

#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>
#include "vtkRenderWindowInteractor.h"
#include "../guiutils.hpp"
#include "../subdialog/messagebox.hpp"

/*
 右クリックに関しては全て再emitだけでQVTKOPwnGLの処理関数は呼ばない。
 というかここでev->ignore()してもmousePressEventは握り潰せない。
 vtkRenderWindowInteractorの方でdisableする必要がある。
 そしてこれは結構面倒。
 結論	QVTKOpenGLのサブクラス化はやめ。shift+左クリックとかにする。
 としようと思ったが
 shft+左クリックもパンが割り当てられているし
 Ctrl+左クリックも回転が割り当てられているので、
 結局の所割り当てられていないクリック動作がない。
*/


CustomQVTKWidget::CustomQVTKWidget(QWidget *parent)
//    : QVTKOpenGLWidget(parent)
//    : QVTKOpenGLStereoWidget(parent)
: QVTKOpenGLWrapperWidget(parent)
{}



bool CustomQVTKWidget::event(QEvent *ev)
{
    auto tp = ev->type();
    //	qDebug() << "type ===" << tp;
	if(tp == QEvent::MouseButtonPress || tp == QEvent::MouseButtonRelease || tp == QEvent::MouseButtonDblClick
			|| tp == QEvent::MouseMove) {
		//qDebug() << "Mouse button or move event ignored.";
		if(static_cast<QMouseEvent*>(ev)->button() == Qt::RightButton) {
            this->GetGenericInteractor()->Disable();
		} else {
            this->GetGenericInteractor()->Enable();
		}
	}
    return QVTKOpenGLWrapperWidget::event(ev);
}

void CustomQVTKWidget::mousePressEvent(QMouseEvent *ev)
{
//	emit customMouseEvent(ev);

	if(ev->button() == Qt::RightButton) {
        this->GetGenericInteractor()->Disable();
	} else {
        this->GetGenericInteractor()->Enable();
	}
	emit customMouseEvent(ev);
}


void CustomQVTKWidget::mouseReleaseEvent(QMouseEvent *ev)
{
//	emit customMouseEvent(ev);
	if(ev->button() == Qt::RightButton) {
        this->GetGenericInteractor()->Disable();
	} else {
        this->GetGenericInteractor()->Enable();
	}
    emit customMouseEvent(ev);
}

void CustomQVTKWidget::resizeEvent(QResizeEvent *evt)
{
    emit resized();
    QVTKOpenGLWrapperWidget::resizeEvent(evt);
}

//void CustomQVTKWidget::mouseDoubleClickEvent(QMouseEvent *ev)
//{
//	if(ev->button() == Qt::RightButton) {
//		emit customMouseEvent(ev);
//	} else {
//		QVTKOpenGLWidget::mouseReleaseEvent(ev);
//	}
//}
