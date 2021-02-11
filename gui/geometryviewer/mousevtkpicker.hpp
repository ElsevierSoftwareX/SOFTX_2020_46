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

/*
 * こんな感じでレンダラを追加した後、インタラクターにセットする。
 * 	auto style = vtkSmartPointer<MouseInteractorHighLightActor>::New();
 * 	style->SetDefaultRenderer(renderer_);
 * 	qvtkWidget_->GetInteractor()->SetInteractorStyle(style);
 *
 *  なぜかピックしたactorのGetPropertyでsegfaultするので使用しない。
 */


#include <vtkPropPicker.h>
#include <vtkSmartPointer.h>
#include <vtkInteractorStyleTrackballCamera.h>
// Handle mouse events
class MouseInteractorHighLightActor : public vtkInteractorStyleTrackballCamera
{
public:
	static MouseInteractorHighLightActor* New();
	vtkTypeMacro(MouseInteractorHighLightActor, vtkInteractorStyleTrackballCamera)

	MouseInteractorHighLightActor()
	{
		LastPickedActor = NULL;
		LastPickedProperty = vtkProperty::New();
	}
	virtual ~MouseInteractorHighLightActor() {LastPickedProperty->Delete();}
	virtual void OnLeftButtonDown()
	{
        int* clickPos = this->GetGenericInteractor()->GetEventPosition();

		// Pick from this location.
		auto picker = vtkSmartPointer<vtkPropPicker>::New();
		picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

		// If we picked something before, reset its property
		//if (this->LastPickedActor) {
		if (this->LastPickedActor && LastPickedProperty) {
			// ピッキングしているとここで落ちる。
			this->LastPickedActor->GetProperty()->DeepCopy(this->LastPickedProperty);
		}
		this->LastPickedActor = picker->GetActor();
		if (this->LastPickedActor) {
			// Save the property of the picked actor so that we can
			// restore it next time
			this->LastPickedProperty->DeepCopy(this->LastPickedActor->GetProperty());
			// Highlight the picked actor by changing its properties
			this->LastPickedActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
			this->LastPickedActor->GetProperty()->SetDiffuse(1.0);
			this->LastPickedActor->GetProperty()->SetSpecular(0.0);
		}

		// Forward events
		vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
	}

private:
	vtkActor    *LastPickedActor;
	vtkProperty *LastPickedProperty;
};
vtkStandardNewMacro(MouseInteractorHighLightActor);





