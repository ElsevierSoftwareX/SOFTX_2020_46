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
#ifndef CAMERAINFO_HPP
#define CAMERAINFO_HPP

#include <array>
#include <sstream>
#include <QSize>
#include <QString>
#include <vtkCamera.h>


struct CameraInfo {
	bool hasData;
	double position[3];
	double focalPoint[3];  // 焦点
	double viewUp[3];      // 画面上側方向を示すベクトル

	CameraInfo():hasData(false){;}
    CameraInfo(const std::array<double, 9>& arr);

	std::string toString() const;
	QString toQString() const;
	void clear();

	static vtkSmartPointer<vtkCamera> createCamera(const CameraInfo &cinfo);
	static CameraInfo fromCamera(vtkCamera *const camera);
	static void setToCamera(const CameraInfo &cinfo, vtkCamera* camera);
	// 視野と物体から適当なサイズの初期カメラを生成する。
	static CameraInfo getAutoAdjustedCameraInfo(const std::array<double, 6> arr, const QSize &sz);


};


#endif // CAMERAINFO_HPP
