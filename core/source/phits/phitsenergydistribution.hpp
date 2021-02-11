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
#ifndef PHITSENERGYDISTRIBUTION_HPP
#define PHITSENERGYDISTRIBUTION_HPP

#include <memory>
#include <string>
#include <list>

namespace inp{
class DataLine;
}

namespace src {
class Distribution;


namespace phits {

class EnergyDistributionCreator {
public:
    EnergyDistributionCreator(const std::string &etype, bool isLen);
    virtual ~EnergyDistributionCreator();
    virtual std::unique_ptr<src::Distribution> operator()(const std::list<inp::DataLine> &sourceInput,
														  std::list<inp::DataLine>::const_iterator &it) = 0;
protected:
	std::string etype_;
	bool isWaveLength_;

};


class EnergyMultiGroupCreator: public EnergyDistributionCreator {
public:
    EnergyMultiGroupCreator(const std::string &etype, bool isLen = false):EnergyDistributionCreator(etype, isLen){}
	std::unique_ptr<src::Distribution> operator() (const std::list<inp::DataLine> &sourceInput,
												   std::list<inp::DataLine>::const_iterator &it) override;
};

class EnergyDiscreteCreator: public EnergyDistributionCreator {
public:
    EnergyDiscreteCreator(const std::string &etype, bool isLen = false):EnergyDistributionCreator(etype, isLen) {}
	std::unique_ptr<src::Distribution> operator() (const std::list<inp::DataLine> &sourceInput,
												   std::list<inp::DataLine>::const_iterator &it) override;
};

class EnergyUserDefinedCreator: public EnergyDistributionCreator {
public:
    EnergyUserDefinedCreator(const std::string &etype, bool isLen = false):EnergyDistributionCreator(etype, isLen) {}
	std::unique_ptr<src::Distribution> operator() (const std::list<inp::DataLine> &sourceInput,
												   std::list<inp::DataLine>::const_iterator &it) override;
};

class EnergyMaxwellCreator: public EnergyDistributionCreator {
public:
    EnergyMaxwellCreator(const std::string &etype, bool isLen = false):EnergyDistributionCreator(etype, isLen) {}
	std::unique_ptr<src::Distribution> operator() (const std::list<inp::DataLine> &sourceInput,
												   std::list<inp::DataLine>::const_iterator &it) override;
};

class EnergyGaussianCreator: public EnergyDistributionCreator {
public:
    EnergyGaussianCreator(const std::string &etype, bool isLen = false):EnergyDistributionCreator(etype, isLen) {}
	std::unique_ptr<src::Distribution> operator() (const std::list<inp::DataLine> &sourceInput,
												   std::list<inp::DataLine>::const_iterator &it) override;
};

std::unique_ptr<src::Distribution> createPhitsEnergyDistribution(const std::string &e_type,
															 const std::list<inp::DataLine> &sourceInput,
															 std::list<inp::DataLine>::const_iterator &it);



}  // end namespace phits
}  // end namespace src
#endif // PHITSENERGYDISTRIBUTION_HPP
