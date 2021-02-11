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
#ifndef CUSTOMQVTKWIDGET_HPP
#define CUSTOMQVTKWIDGET_HPP

#include <QWidget>
#include <QVTKInteractor.h>
#include <vtkRenderWindow.h>
#include "../qvtkopenglwrapperwidget.hpp"

class QMouseEvent;
class QEvent;
class QRenderWindow;

// QVTKOpenGLWidgetの右クリックデフォルト動作をカスタムするために
// 新クラスを作った。
class CustomQVTKWidget : public QVTKOpenGLWrapperWidget
{
	Q_OBJECT
public:
    CustomQVTKWidget(QWidget *parent);
    void disableInteractor(bool flag);
    vtkRenderWindow* GetGenericRenderWindow() const
    {
#if VTK_MAJOR_VERSION >= 9
        return this->renderWindow();
#else
        return this->GetRenderWindow();
#endif

    }
    QVTKInteractor* GetGenericInteractor() const
    {
#if VTK_MAJOR_VERSION >= 9
        return this->interactor();
#else
        return this->GetInteractor();
#endif
    }

protected:
	bool event(QEvent *ev) override;
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent* ev) override;

    void resizeEvent(QResizeEvent *evt) override;

signals:
	void customMouseEvent(QMouseEvent* ev);
    void resized();


};


#endif // CUSTOMQVTKWIDGET_HPP
