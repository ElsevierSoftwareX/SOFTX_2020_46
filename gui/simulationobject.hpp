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
#ifndef SIMULATIONOBJECT_HPP
#define SIMULATIONOBJECT_HPP

#include <QObject>
#include <QString>

#include "../core/simulation.hpp"
#include "../core/option/config.hpp"
#include "option/guiconfig.hpp"

//! Simulationクラスのラッパー。I/OやQtのsignal/slotを扱えるようにしてある。
class SimulationObject : public QObject
{
	Q_OBJECT
public:
	explicit SimulationObject(QObject *parent = 0);

	void readFile(const std::string &inputFile, GuiConfig *gconf);
	void clear() noexcept;
	const std::shared_ptr<Simulation>  &simulation() const {return simulation_;}

public slots:
	void handleGuiConfigChanged();

signals:
	void requestUpdateCellColor();
	void fileOpenSucceeded(std::pair<std::string, std::string> filePair);  // firstが親ファイル secondが読もうとするファイル
	void simulationChanged(std::shared_ptr<const Simulation>);
	void geometryChanged(std::shared_ptr<const geom::Geometry>);
//	void tallyChanged(std::vector<std::shared_ptr<tal::PkTally>>);
//	void sourceChanged(std::vector<std::shared_ptr<const src::PhitsSource>>);



private:
	std::shared_ptr<Simulation> simulation_;
	GuiConfig *guiConfig_;
};

#endif // SIMULATIONOBJECT_HPP
