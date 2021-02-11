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
#ifndef GLOBALS_HPP
#define GLOBALS_HPP

//#include <string>

namespace global {

constexpr char VERSION_STR[] = "1.3";
#ifdef DEFINE_GLOBAL_
    bool isDarkTheme;

#else
	// darkかどうかは適当なクラスのパレットを見ればわかるのでdeprecated
	// と思ったが結構css長く、regexとかも使うので、頻繁に判定するより、globalに持っておいたほうが良い。
    extern bool isDarkTheme;
#endif
#undef DEFINE_GLOBAL_

}  // end namespace global
#endif // GLOBALS_HPP
