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
#include "fileconverter.hpp"

#include <cstdlib>
#include <iostream>
#include <unordered_map>

#include <QFileDialog>

#include <vtkPLYWriter.h>
#include <vtkPolyData.h>
#include <vtkPolyDataWriter.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkReverseSense.h>
#include <vtkSmartPointer.h>
#include <vtkSTLWriter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkWindowToImageFilter.h>

#include "gui/subdialog/messagebox.hpp"



namespace {


}  // end anonymous namespace






bool ff::isXMLFormat(ff::FORMAT3D f)
{
	switch(f) {
	case FORMAT3D::VTP:
		return true;
		break;
	case FORMAT3D::VTK:
	case FORMAT3D::STL:
	case FORMAT3D::PLY:
		return false;
		break;
	default:
		std::cerr << __FILE__ << ":" << __LINE__
				  << " Invalid file format.This could not be happen." << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

void ff::exportFile(ff::FORMAT3D format, const std::string &fileBaseName, vtkPolyData *polyData, double factor, bool reverse)
{
	// polyDataはconst&としたいがvtkの関数各種がconstっぽいやつでも非constなので変更不可。
	reverse = true;
//	auto writingData = vtkSmartPointer<vtkPolyData>::New();
	auto writingData = vtkPolyData::New();
	writingData->DeepCopy(polyData);

	if(reverse) {
		auto reverseSense = vtkSmartPointer<vtkReverseSense>::New();
		reverseSense->SetInputData(writingData);
		reverseSense->ReverseNormalsOn();
		reverseSense->Update();
		//reverseSense->Print(std::cout);
		// vtkReverseSenseが破棄されると出力データも破棄されるのでdeepcopyする必要がある。
		writingData->DeepCopy(reverseSense->GetOutput());
	}


	// factorの適用
	// vtkShrinkPolyDataFilterはなぜかSetInputが機能しない。
	// また、shrinkPolydataFilterはpolygon中心に対してshrinkするのでこの用途には不向き
//	shrink->SetShrinkFactor(0.1);
//	shrink->SetInputData(writingData); // うまく行かない

	auto transform = vtkSmartPointer<vtkTransform>::New();
	transform->Identity();
	transform->Scale(factor, factor, factor);
	vtkSmartPointer<vtkTransformPolyDataFilter> shrink = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	shrink->SetTransform(transform);
	shrink->SetInputData(writingData);
	shrink->Update();
	//shrink->Print(std::cout);

	std::string fileName = fileBaseName + "." + getFormat3DSuffix(format);
	if(format == FORMAT3D::VTP) {
		auto writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
		writer->SetDataModeToAscii();
		writer->SetFileName(fileName.c_str());
		writer->SetInputData(shrink->GetOutput());
		writer->Write();
	} else if (format == FORMAT3D::VTK) {
		auto writer = vtkSmartPointer<vtkPolyDataWriter>::New();
		writer->SetFileName(fileName.c_str());
		writer->SetInputData(shrink->GetOutput()); // setInputDataは基底で宣言されていないから個別のWriterを使う必要がある
		writer->Write();
	} else if (format == FORMAT3D::STL) {
		auto writer = vtkSmartPointer<vtkSTLWriter>::New();
		writer->SetFileName(fileName.c_str());
		writer->SetInputData(shrink->GetOutput());
		writer->Write();
	} else if (format == FORMAT3D::PLY) {
		auto writer = vtkSmartPointer<vtkPLYWriter>::New();
		writer->SetFileName(fileName.c_str());
		writer->SetInputData(shrink->GetOutput()); // setInputDataは基底で線源されていないから個別のWriterを使う必要がある
		writer->Write();
	}
}



//std::string ff::get3DFileName(const std::string &orgName, FORMAT3D format)
//{
//#if defined(_WIN32) || defined(_WIN64)  // windoesの場合はなぜか拡張子が追加されないので手動追加
//	return orgName +  "."  + ff::getFormat3DSuffix(format);
//#else
//	(void) format;  // vtk on linuxでは拡張子は自動付与される
//	return orgName;
//#endif
//}




std::pair<QFileInfo, ff::FORMAT2D> ff::getFileAndFormat2D(std::vector<ff::FORMAT2D> excludingFormats)
{
	static const std::vector<ff::FORMAT2D> allFormats {
		ff::FORMAT2D::PNG, ff::FORMAT2D::JPG, ff::FORMAT2D::XPM, ff::FORMAT2D::BMP
	};

	QString supportedFormatsString;
	for(auto it = allFormats.cbegin(); it != allFormats.cend(); ++it) {
		if(std::find(excludingFormats.cbegin(), excludingFormats.cend(), *it) == excludingFormats.cend()) {
			supportedFormatsString += QString::fromStdString(ff::format2DToStr(*it));
			if(it != allFormats.cend() - 1) supportedFormatsString += "/";
		}
	}
	QString selectmessage = QObject::tr("Set exporting file name ") + "(" + supportedFormatsString + ")";
	QFileInfo info(QFileDialog::getSaveFileName(0, selectmessage));
	auto format = ff::strToFormat2D(info.suffix().toStdString());
	if(format == ff::FORMAT2D::NOT_DEFINED) {
		QString warningMessage = QObject::tr("Set file name (") + supportedFormatsString + ").";
        GMessageBox::warning(0, QObject::tr("Warning"), warningMessage, true);
		return std::make_pair(QFileInfo(), ff::FORMAT2D::NOT_DEFINED);
	}
	for(const auto& fm: excludingFormats) {
		if(fm == format) {
			QString message = QString::fromStdString(ff::format2DToStr(format));
			message += " " + QObject::tr("format is not supported currently in this tab.");
            GMessageBox::warning(0, QObject::tr("Warning"), message, true);
			return std::make_pair(QFileInfo(), ff::FORMAT2D::NOT_DEFINED);;
		}
	}
	return std::make_pair(info, format);
}

vtkSmartPointer<vtkImageWriter> ff::getVtk2DWriter(ff::FORMAT2D format)
{
	vtkSmartPointer<vtkImageWriter> writer;
	switch(format) {
	case ff::FORMAT2D::BMP:
		writer = vtkSmartPointer<vtkBMPWriter>::New();
		break;
	case ff::FORMAT2D::JPG:
		writer = vtkSmartPointer<vtkJPEGWriter>::New();
		break;
	case ff::FORMAT2D::PNG:
		writer = vtkSmartPointer<vtkPNGWriter>::New();
		break;
	default:
		std::cerr << "format = " << ff::format2DToStr(format) << " is not supported in vtkWriter" << std::endl;
		break;
	}
	return writer;
}

void ff::exportRenderWindowToRasterGraphics(vtkRenderWindow *rwindow) {
	// ファイル名、フォーマット、vtkライターの取得
	auto infoPair = ff::getFileAndFormat2D(std::vector<ff::FORMAT2D>{ff::FORMAT2D::XPM});
	auto writer = ff::getVtk2DWriter(infoPair.second);
	if(!writer) throw std::invalid_argument("format = " + ff::format2DToStr(infoPair.second) + " is not supported in vtkWriter");

	writer->SetFileName(infoPair.first.absoluteFilePath().toStdString().c_str());
	rwindow->Render();
	auto w2iFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
	w2iFilter->SetInput(rwindow);
	writer->SetInputData(w2iFilter->GetOutput());
	w2iFilter->Update();
	writer->Write();
}
