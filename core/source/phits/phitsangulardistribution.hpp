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
#ifndef PHITSANGULARDISTRIBUTION_HPP
#define PHITSANGULARDISTRIBUTION_HPP

#include <memory>
#include <string>
#include <list>


namespace inp{
class DataLine;
}


namespace src{
class Distribution;

namespace phits{

class AngularDistributionCreator {
public:
    AngularDistributionCreator(const std::string &atype, bool isDeg);
    virtual ~AngularDistributionCreator();
	virtual std::unique_ptr<src::Distribution> operator()(const std::list<inp::DataLine> &sourceInput,
														  std::list<inp::DataLine>::const_iterator &it) = 0;
protected:
	std::string atype_;
	bool isDegree_;
};


class AngularMultiGroupCreator: public AngularDistributionCreator {
public:
    AngularMultiGroupCreator(const std::string &atype, bool isDeg):AngularDistributionCreator(atype, isDeg){}
	std::unique_ptr<src::Distribution> operator() (const std::list<inp::DataLine> &sourceInput,
												   std::list<inp::DataLine>::const_iterator &it) override;
};
class AngularUserDefinedCreator: public AngularDistributionCreator {
public:
    AngularUserDefinedCreator(const std::string &atype, bool isDeg):AngularDistributionCreator(atype, isDeg){}
	std::unique_ptr<src::Distribution> operator() (const std::list<inp::DataLine> &sourceInput,
												   std::list<inp::DataLine>::const_iterator &it) override;
};



std::unique_ptr<src::Distribution>
	createPhitsAngularDistribution(const std::string &a_type,
									const std::list<inp::DataLine> &sourceInput,
									std::list<inp::DataLine>::const_iterator &it);

}  // end namespace phits
}  // end namespace src

#endif // PHITSANGULARDISTRIBUTION_HPP
