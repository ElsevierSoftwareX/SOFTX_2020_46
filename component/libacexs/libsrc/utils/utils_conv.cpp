#include "utils_conv.hpp"

#include <sstream>


double compat::stod(const std::string& str) {
	std::stringstream ss;
	ss << str;
	double retval;
	ss >> retval;
	return retval;
}


int compat::stoi(const std::string& str) {
	std::stringstream ss;
	ss << str;
	int retval;
	ss >> retval;
	return retval;
}

long int compat::stoli(const std::string &str) {
	std::stringstream ss;
	ss << str;
	long int retval;
	ss >> retval;
	return retval;
}

std::string compat::to_string(const double& val) {
	std::stringstream ss;
	ss << val;
	return ss.str();
}
std::string compat::to_string(const int& val) {
	std::stringstream ss;
	ss << val;
	return ss.str();
}
