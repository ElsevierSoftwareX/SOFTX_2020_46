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
#ifndef COLORPANE_HPP
#define COLORPANE_HPP

#include <memory>
#include <QString>
#include <QWidget>

#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkLookupTable.h>
#include <vtkSmartPointer.h>

namespace fd{
struct FieldColorData;
class XyzMeshTallyData;
}


namespace Ui {
class ColorPane;
}


class ColorPane : public QWidget
{
	Q_OBJECT
public:
	explicit ColorPane(QWidget *parent = 0);
	~ColorPane();
    const std::shared_ptr<const fd::XyzMeshTallyData> &meshData() {return meshData_;}
    QString scalarBarTitle() const;
    int numTics() const;
    bool useFieldDataChecked() const;
    bool isScalarBarEnabled() const;
	bool isChanged() const {return changed_;}
	// FieldColorData::validはなくす。存在する限りvalid。存在しなければスマポが未割り当て
	std::shared_ptr<const fd::FieldColorData>  fieldColorData() const;
	void updateChangedState(bool state);
    vtkSmartPointer<vtkScalarBarActor> scalarBarActor() {scalarBar_->SetLookupTable(lookupTable_);return scalarBar_;}
    vtkSmartPointer<vtkLookupTable> lookupTable() {return lookupTable_;}
	bool updateScalarBar(vtkSmartPointer<vtkRenderer> renderer);
	void retranslate();
public slots:
	void clear();

signals:
	void requestShowLegend(bool);

private:
	Ui::ColorPane *ui;
	// セルの再描画をすべきかどうかの判定に色設定が変更されているかを用いる。
	// とりあえず何か変更したらtrueにする。(すぐに戻しても。)
	bool changed_;
    std::shared_ptr<const fd::XyzMeshTallyData> meshData_;
    QString currentDataAbsFilePath_;

    // フラックス色表示のときの凡例
    vtkSmartPointer<vtkScalarBarActor> scalarBar_;
    vtkSmartPointer<vtkLookupTable> lookupTable_;

    void calcXyzMeshData();
	void enableWidgets(bool flag);
private slots:
	void selectFile();
	void setChanged();

};

#endif // COLORPANE_HPP
