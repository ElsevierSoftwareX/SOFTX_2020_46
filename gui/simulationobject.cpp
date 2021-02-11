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
#include "simulationobject.hpp"

#include <cstdlib>
#include <string>

#include <QMetaObject>

#include "../core/io/input/mcmode.hpp"
#include "../core/geometry/geometry.hpp"
#include "../core/source/phits/phitssource.hpp"
#include "../core/tally/pktally.hpp"
#include "../core/utils/time_utils.hpp"

SimulationObject::SimulationObject(QObject *parent)
    : QObject(parent)
{
	qRegisterMetaType<std::shared_ptr<const Simulation>>("std::shared_ptr<const Simulation>");
	//qRegisterMetaType<std::pair<std::string, std::string>>("std::pair<std::string, std::string>");  // guimainクラスで登録済み
}

#include "../core/material/material.hpp"
void SimulationObject::readFile(const std::string &inputFileName, GuiConfig *gconf)
{
	guiConfig_ = gconf;
	if(simulation_){
		disconnect(simulation_.get(), &Simulation::fileOpenSucceeded, this, &SimulationObject::fileOpenSucceeded);
		simulation_.reset();
	}
	if(inputFileName.empty()) return;

	utils::SimpleTimer timer;
	timer.start();
	simulation_ = std::make_shared<Simulation>();
	connect(simulation_.get(), &Simulation::fileOpenSucceeded, this, &SimulationObject::fileOpenSucceeded);
	simulation_->init(inputFileName, guiConfig_->cuiConfig);
    // ここでパレットもconfigへロードしておく。ただし変更不可のシステム予約色は削除する。
    auto currentColorMap = simulation_->defaultPalette()->colorMap();
    for(auto it = currentColorMap.begin(); it != currentColorMap.end();) {
        if(!img::MaterialColorData::isUserDefinedColor(it->second)) {
            it = currentColorMap.erase(it);
        } else {
            ++it;
        }
    }
    guiConfig_->cuiConfig.colorMap = currentColorMap;
	timer.stop();
	mDebug() << "Construction of simulation class done in " << timer.msec() << "(ms)";
	emit simulationChanged(simulation_);
}

void SimulationObject::clear() noexcept
{
	if(simulation_) {
		simulation_->clear();  // MaterialDBをクリアするには明示的にclearを呼ばないとだめ。
		simulation_.reset();
        emit simulationChanged(simulation_);
	}
}

void SimulationObject::handleGuiConfigChanged()
{
    if(!simulation_) return;
    // guiConfig変更をhandleする。とりあえずいまのところはcolorMapのみ

    // geometry は粒子追跡時のデータ競合を防ぐためにshared_ptr<"const" Geometry>としているが
    // ここでは色を変更したいのでconstでは不都合。粒子追跡時のconst性を放棄するよりは
    // ここで一時的にconst除去キャストする。
    // 本来はgeometryに色をもたせるのが良くないという気がする。Simulationクラスで保持すべきであろう。
    const std::shared_ptr<geom::Geometry> geom = std::const_pointer_cast<geom::Geometry>(simulation_->getGeometry());

    geom->clearUserDefinedPalette();
	if(!guiConfig_->cuiConfig.colorMap.empty()) {
		geom->createModifiedPalette(guiConfig_->cuiConfig.colorMap);
    } else {
        geom->setDefaultPalette();
    }
    // 本当はここでパレット変更を検知してupdateするか否かを判定したい。
    emit requestUpdateCellColor();

}


