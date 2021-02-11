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
#include "camerainfo.hpp"

#include <array>

CameraInfo::CameraInfo(const std::array<double, 9> &arr)
{
    hasData = true;
    position[0] = arr.at(0);
    position[1] = arr[1];
    position[2] = arr[2];
    focalPoint[0] = arr[3];
    focalPoint[1] = arr[4];
    focalPoint[2] = arr[5];
    viewUp[0] = arr[6];
    viewUp[1] = arr[7];
	viewUp[2] = arr[8];
}

CameraInfo CameraInfo::fromCamera(vtkCamera * const camera) {
	CameraInfo cinfo;
	camera->GetPosition(cinfo.position);
	camera->GetFocalPoint(cinfo.focalPoint);
	camera->GetViewUp(cinfo.viewUp);
	cinfo.hasData = true;
	return cinfo;
}

void CameraInfo::setToCamera(const CameraInfo &cinfo, vtkCamera *camera) {
	camera->SetPosition(cinfo.position);
	camera->SetFocalPoint(cinfo.focalPoint);
	camera->SetViewUp(cinfo.viewUp);
}

std::string CameraInfo::toString() const {
	std::stringstream ss;
	ss << "pos = {" << position[0] << ", " << position[1] << ", " << position[2] << "}  "
	   << "focal = {" << focalPoint[0] << ", " << focalPoint[1] << ", " << focalPoint[2] << "}  "
	   << "viewup = {" << viewUp[0] << ", " << viewUp[1] << ", " << viewUp[2] << "}";
	return ss.str();
}

QString CameraInfo::toQString() const { return QString::fromStdString(toString());}

/*
 * arrはGeometryViewerConfig::region()であり、xmin,xmax,ymin,ymax,zmin,zmax
 * szはQVTKOpenGLWidgetのサイズ(縦×横ピクセル)
 */
CameraInfo CameraInfo::getAutoAdjustedCameraInfo(const std::array<double, 6> arr, const QSize &sz) {
	// regionに応じてカメラを近づけるとりあえずZ方向から見るとするので xy方向の広がりから距離をだす。
	double factor1 = 0.5*((arr[1] - arr[0]) + (arr[3] - arr[2]));
	double factor2 = 0.5*(sz.width() + sz.height());
	double dist = factor1/factor2*1e+2  + arr[5]*1.2;
	//mDebug() << "viewArea===" << factor2 << "rendaArea====" << factor1 << "dist=" << dist;
	/*
	 * CameraInfoの9引数は 位置xyz 焦点xyz 上方向xyz
	 *
	 */
	return CameraInfo(std::array<double, 9>{0.5*(arr[0]+arr[1]), 0.5*(arr[2]+arr[3]), dist*10, // カメラ位置
											0, 0, 0,  // 焦点位置
											0, 1, 0   // 画面上に対応するQVTK世界の中での方向ベクトル
											});
}

void CameraInfo::clear()
{
	hasData = false;
	for(size_t i = 0; i < 3; ++i) {
		position[i] = 0;
		focalPoint[i] = 0;
		viewUp[i] = 0;
	}
}

vtkSmartPointer<vtkCamera> CameraInfo::createCamera(const CameraInfo &cinfo)
{
	auto camera = vtkSmartPointer<vtkCamera>::New();
	camera->SetPosition(cinfo.position);
	camera->SetFocalPoint(cinfo.focalPoint);
	camera->SetViewUp(cinfo.viewUp);
	return camera;
}


