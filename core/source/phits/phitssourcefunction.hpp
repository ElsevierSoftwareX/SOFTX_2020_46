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
#ifndef PHITSSOURCEFUNCTION_HPP
#define PHITSSOURCEFUNCTION_HPP

#include <array>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace geom {
class Cell;
}
namespace inp{
class DataLine;
class MeshData;
}

namespace src{
class Distribution;

namespace phits {

void readPhitsExtendedParameters(const std::list<inp::DataLine> &extendedInput,
								const std::vector<std::string> &acceptableExtendedKeywords,
								const std::vector<std::string> &essentialExtendedKeywords,
								const std::shared_ptr<src::Distribution> energyDistribution,
								const std::array<std::shared_ptr<src::Distribution>, 3> spatialDistributions,
								const std::array<std::string, 3> &spatialChars,
								inp::MeshData *energyMesh,
								std::array<inp::MeshData, 3> *spatialMeshes);


std::unordered_map<std::string, std::string>
	readPhitsParameters(const std::list<inp::DataLine> & sourceInput,
						 const std::unordered_map<std::string, std::string> &defaultMap,
						 const std::vector<std::string> &acceptables,
						 const std::vector<std::string> &essentials,
						 const std::vector<std::pair<std::string, std::string>> exclusives,
						 const std::unordered_map<std::string, std::shared_ptr<const geom::Cell> > &cellMap, double *factor,
						 std::shared_ptr<src::Distribution> *energyDistribution,
						 std::shared_ptr<src::Distribution> *angularDistribution,
						 std::vector<std::shared_ptr<const geom::Cell>> *acceptableCells,
						 bool hasEnergyDistribution);


}
}



#endif // PHITSSOURCEFUNCTION_HPP
