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
#ifndef DATACARD_HPP
#define DATACARD_HPP

#include <regex>
#include <string>

namespace inp {

namespace comm {

class DataCard {
public:
	DataCard(){;}
	DataCard(const std::string &str, const std::regex &reg);
	const std::regex &regex() const {return regex_;}
	const std::string &modifier() const {return modifier_;}
	const std::string &name() const {return name_;}
	const std::string &argument() const {return argument_;}
	virtual std::string toString() const;
	const std::string &id() const {return id_;}
	int idNumber() const {return std::stoi(id_);}

protected:
	const std::regex regex_;
	// *TR10 2 3 4 を例に取ると
	std::string modifier_;    // "*"
	std::string name_;        // "TR"
	std::string id_;          // "10"
	std::string argument_; // "2 3 4"
};



}  // end namespace comm
}  // end namespace inp


#endif // DATACARD_HPP
