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
#include "axisarrows.hpp"


#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include "../../core/math/nmatrix.hpp"


AxisArrows::AxisArrows()
	: scalingFactors_(std::array<double, 3>{1, 1, 1}), origin_(math::Point{0, 0, 0}),
	  tipLength_(0.1*2), tipRadius_(0.35*2), shaftRadius_(0.03*2)
{
	for(size_t xyz = 0; xyz < 3; ++xyz) {
		arrows_[xyz] = vtkSmartPointer<vtkArrowSource>::New();
		actors_[xyz] = vtkSmartPointer<vtkActor>::New();
		transforms_[xyz] = vtkSmartPointer<vtkTransform>::New();
		transformPDs_[xyz] = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
		mappers_[xyz] = vtkSmartPointer<vtkPolyDataMapper>::New();

		transformPDs_[xyz]->SetTransform(transforms_[xyz]);
		transformPDs_[xyz]->SetInputConnection(arrows_[xyz]->GetOutputPort());
		mappers_[xyz]->SetInputConnection(arrows_[xyz]->GetOutputPort());
		mappers_[xyz]->SetInputConnection(transformPDs_[xyz]->GetOutputPort());
		actors_[xyz]->SetMapper(mappers_[xyz]);
	}

    /*
     * vtkArrowSourceは(0, 0, 0) → (1, 0, 0)の矢印。
     * 向きや長さを変えるには行列で変換する
     */

    math::Vector<3> toOriginVec{-0.5, 0, 0};
    for(size_t i = 0; i < matrices_.size(); ++i) {
        matrices_[i] = math::Matrix<4>::IDENTITY();
    }

	auto xToY3 = math::generateRotationMatrix1(math::Vector<3>{0, 1, 0}, math::Vector<3>{1, 0, 0});
    auto xToZ3 = math::generateRotationMatrix1(math::Vector<3>{0, 0, 1}, math::Vector<3>{1, 0, 0});
    matrices_[1].setRotationMatrix(xToY3);
    matrices_[2].setRotationMatrix(xToZ3);
	// ここまででmatrices_[xyz]はxyz軸のarrowを(0,0,0)→(1,0,0)からそれぞれの軸へ変換するようにセッティングされている。
    // TODO ↑みたいに並進と回転セットした場合、どっちが先に適用されるかチェック
}


void AxisArrows::setRange(const std::array<double, 6> ranges)
{
	// 拡大ファクターと原点をセット
	scalingFactors_  = std::array<double, 3> {ranges[1]-ranges[0], ranges[3]-ranges[2], ranges[5]-ranges[4]};
	origin_ = math::Point{ranges[0], ranges[2], ranges[4]};
	if(!origin_.isValid()) {
		mWarning() << "Origin of Arrows is invalid, {0, 0, 0} is used instead.";
		origin_ = math::Point{0, 0, 0};
	}
	mDebug() << "range[024]=" << ranges[0] << ranges[2] << ranges[4];
	mDebug() << "scals=" << scalingFactors_;
	mDebug() << "origin=" << origin_;
}

void AxisArrows::clear() {
	//*this = AxisArrows();
}


std::array<vtkSmartPointer<vtkActor>, 3> AxisArrows::actors()
{
	using Arr4 = std::array<double, 4>;
	const std::array<std::array<double, 4>, 4> unitArray{Arr4{1,0,0,0}, Arr4{0,1,0,0}, Arr4{0,0,1,0}, Arr4{0,0,0,1}};

	for(size_t xyz = 0; xyz < 3; ++xyz) {
		// 拡大行列を作成
		auto tmpArray = unitArray;
		for(size_t i = 0; i < 3; ++i) {
			tmpArray[i][i] = scalingFactors_[xyz];
		}
		math::Matrix<4> expandingMat{tmpArray};

		// 並進行列を作成
		math::Matrix<4> translationMat = math::Matrix<4>::IDENTITY();
		translationMat.setTranslationVector(origin_);

		// 合成
		math::Matrix<4> matrix = matrices_[xyz]*expandingMat*translationMat;

		//まずvtkMatrixの作成。転置が必要
		auto vtkMatrix =vtkSmartPointer<vtkMatrix4x4>::New();
		for(size_t i = 0; i < 4; ++i) {
			for(size_t j = 0; j < 4; ++j) {
				vtkMatrix->SetElement(i, j, matrix(j, i));
			}
		}
		// ここまでは毎回実施が必要。


		// ここでShaftやtipの調整を行う
		arrows_[xyz]->SetTipLength(tipLength_); // tiplengthは行列による拡大がないのでscaling因子で割らない。
		arrows_[xyz]->SetTipRadius(tipRadius_/scalingFactors_[xyz]);
		arrows_[xyz]->SetShaftRadius(shaftRadius_/scalingFactors_[xyz]);

		/*
		 * データを更新する時に作り変える必要があるところはどこか？
		 * vtkActor::setMapper： 必要なし
		 *   mapperとの連結関係をセットするから
		 *   mapperインスタンス自体がnewされていなければ毎回呼ぶ必要はない。
		 *
		 * 要するにSetInputConnectionは連結関係を更新しなければ必要はない。
		 */
		// Apply the transforms
		transforms_[xyz]->Identity();
		transforms_[xyz]->Concatenate(vtkMatrix);

		transforms_[xyz]->Update();
		transformPDs_[xyz]->Update();
		mappers_[xyz]->Update();
    }
	actors_[0]->GetProperty()->SetColor(1, 0, 0);
	actors_[1]->GetProperty()->SetColor(0, 1, 0);
	actors_[2]->GetProperty()->SetColor(0, 0, 1);
	return actors_;
}
