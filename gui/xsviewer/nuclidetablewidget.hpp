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
#ifndef NUCLIDETABLEWIDGET_HPP
#define NUCLIDETABLEWIDGET_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <QWidget>
#include <QTreeWidgetItem>

#include "../../core/physics/physconstants.hpp"

namespace mat {
class Nuclide;
}


namespace Ui {
class NuclideTableWidget;
}

class NuclideTableWidget : public QWidget
{
	Q_OBJECT

public:
	NuclideTableWidget(phys::ParticleType pt, QWidget *parent = 0);
	~NuclideTableWidget();
	const std::string particleName() const {return phys::particleTypeTostr(ptype_);}
	phys::ParticleType ptype() const {return ptype_;}
	void setNuclides(const std::unordered_map<std::string, std::shared_ptr<const mat::Nuclide> > &nucMap);
	void clear();
	void clearTableCheckBox();
	void retranslate();
	// 最低限表示したい幅(チェックボックスが表示される最小限の幅)
	int minRequiredWidth()const;
	// チェックされているitemのポインタのvectorを返す。
	std::vector<QTreeWidgetItem *> checkedItems() const;

signals:
	void itemChanged(phys::ParticleType ptype, QTreeWidgetItem *item, int column);

private:
	Ui::NuclideTableWidget *ui;
	phys::ParticleType ptype_;  // この粒子に対する断面積であることを保存

private slots:
	void handleDoubleClick(QTreeWidgetItem *item, int column);
	void relayItemChanged(QTreeWidgetItem *item, int column);

// static

public:
	static const int COLUMN_ZAID;
	static const int COLUMN_MT;
	static const int COLUMN_PLOT;
	static const int COLUMN_FACTOR;
	static const int COLUMN_DESC;
};

#endif // NUCLIDETABLEWIDGET_HPP
