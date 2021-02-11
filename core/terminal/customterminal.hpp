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
#ifndef CUSTOMTERMINAL_HPP
#define CUSTOMTERMINAL_HPP

#include <iostream>
#include <list>
#include <string>


#if defined(__unix__) && !defined(ENABLE_GUI)
#include <termios.h>
#include <unistd.h>
#endif


namespace term{


class CustomTerminal
{
public:
    explicit CustomTerminal(const std::string &prompt);
#if defined(__unix__) && !defined(ENABLE_GUI)
	~CustomTerminal()
	{
		tcsetattr(STDIN_FILENO, TCSANOW, &save_);
	}
#endif
	std::istream & customGetline(std::istream &ist, std::string &buff);
	const std::list<std::string> &history() const {return history_;}

private:
	std::string prompt_;
#if defined(__unix__) && !defined(ENABLE_GUI)
	struct termios save_;
#endif
	std::list<std::string> history_;

};


}  // end namespace term
#endif // CUSTOMTERMINAL_HPP
