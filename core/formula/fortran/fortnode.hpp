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
#ifndef FORTNODE_HPP
#define FORTNODE_HPP

#include <memory>
#include <regex>
#include <string>

namespace fort {

class Node {
public:
	Node(const std::string &str);
	double calculate() const;
	double calculate(const std::string &argStr, double value) const;
	std::string expression() const {return expression_;}

private:
	std::string expression_;  // nodeの持つ式or値
	std::unique_ptr<Node> left_;              // 二分木左側node
	std::unique_ptr<Node> right_;             // 右側node
};


double eq(std::string str);


}  // end namespace fort
#endif // FORTNODE_HPP
