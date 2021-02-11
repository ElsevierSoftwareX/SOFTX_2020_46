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
#ifndef XPMARRAY_HPP
#define XPMARRAY_HPP


#include <string>


class XpmArray {
public:
	explicit XpmArray(std::string xpmStr);

	~XpmArray();

	char ** getArray() const {return xpm_;}

private:
	size_t numLines_;
	char **xpm_ = nullptr;  // QPixMapコンストラクタのシグネチャに合わせる
};

#endif // XPMARRAY_HPP
