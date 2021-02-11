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
#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "math/nvector.hpp"


namespace img{
class BitmapImage;
class CellColorPalette;
}
namespace inp{
class InputData;
}
namespace mat{
class Materials;
}
namespace geom{
class Geometry;
}
namespace tal{
class PkTally;
}
namespace src{
class PhitsSource;
}
namespace conf{
struct Config;
}

namespace ff {
enum class FORMAT3D: int;
}



#ifdef ENABLE_GUI
#include <QObject>
class Simulation : public QObject
{
	Q_OBJECT
signals:
	void fileOpenSucceeded(std::pair<std::string, std::string> filePair);
#else
class Simulation
{
#endif
public:
	Simulation();
    ~Simulation();
	void init(const std::string &inputFileName, conf::Config config);
	// setter
	void setMaterials(const std::shared_ptr<const mat::Materials> &mats) {materials_ = mats;}
	void setGeometry(const std::shared_ptr<const geom::Geometry> &geom) {geometry_ = geom;}
	void setTallies(const std::vector<std::shared_ptr<tal::PkTally>> &tals);
    void setSources(const std::vector<std::shared_ptr<const src::PhitsSource> > &srcs);

        // getter
    const std::shared_ptr<const mat::Materials> &getMaterials() const {return materials_;}
	const std::shared_ptr<const geom::Geometry> &getGeometry() const {return geometry_;}
	const std::vector<std::shared_ptr<tal::PkTally>> &getTallies() const {return tallies_;}
	const std::vector<std::shared_ptr<const src::PhitsSource>> &getSources() const {return sources_;}

	void run(int numThread);
	void clear() noexcept;
	img::BitmapImage plotSectionalImage(const math::Vector<3> &origin,
						   const math::Vector<3> &hDir,
						   const math::Vector<3> &vDir,
						   size_t hReso, size_t vReso,
						   int lineWidth, int pointSize,
                           int numThread, bool verbose, bool quiet=false) const;
	std::string finalInputText() const;
    const std::unique_ptr<const img::CellColorPalette> &defaultPalette() const;


    // static
//	static std::shared_ptr<Simulation> createFromFile(const std::string &inputFileName, conf::Config config);


private:
    std::string inputFileName_;
	std::shared_ptr<const mat::Materials> materials_;
	std::shared_ptr<const geom::Geometry> geometry_;
	// tallyは計算によって変更されるのでconstではない。
	std::vector<std::shared_ptr<tal::PkTally>> tallies_;
	std::vector<std::shared_ptr<const src::PhitsSource>> sources_;
    std::unique_ptr<const img::CellColorPalette> defaultPalette_;
};

#endif // SIMULATION_HPP
