#ifndef UTILS_CONV_HPP_
#define UTILS_CONV_HPP_

#include <string>
#include <vector>


// できれば使いたくないがwindowsとの互換性を取るための関数。
namespace compat {

// mingwではg++ version 4.8でもC++11の変換関数(std::stod、stoi等)が使えないのでそれを回避する。

double stod(const std::string& str);
int stoi(const std::string& str);
long int stoli(const std::string& str);
std::string to_string(const double& val);
std::string to_string(const int& val);

template <class T>
T Sto(const std::string& str) {
	// ACEファイルの有効桁数はやたらめったら長いせいか
	// 直接int変換すると結果がおかしくなる ex.3.0000000000+05が3になったり。
	// なので一旦doubleに変換してからintへキャストする。
	return static_cast<T>(compat::stod(str));
}

template <class T>
inline std::vector<T> GetPartialVec(const std::vector<std::string>& sourceVec,
							 const std::size_t startIndex, const std::size_t numElements) {
	std::vector<T> retVec;
	for(std::size_t i = startIndex; i < startIndex + numElements; i++) {
		retVec.emplace_back(Sto<T>(sourceVec.at(i)));
	}
	return retVec;
}


}  // end namespace compat


#endif
