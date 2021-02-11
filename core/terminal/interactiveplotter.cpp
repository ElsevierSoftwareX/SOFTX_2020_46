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
#include "interactiveplotter.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <list>



#include "core/geometry/geometry.hpp"
#include "core/utils/utils.hpp"

#include "customterminal.hpp"


namespace {

const double DEFAULT_WIDTH_CM = 200;
const char DEFAULT_FILENAME[] = "plot.xpm";
const size_t DEFAULT_VRESOLUTION = 800;
const size_t DEFAULT_HRESOLUTION = DEFAULT_VRESOLUTION;
const int DEFAULT_LINEWIDTH = 1;
const int DEFAULT_POINTSIZE = static_cast<int>(DEFAULT_HRESOLUTION/200);
}



void term::InteractivePlotter::plotSection(const math::Vector<3> &offset,
											 math::Vector<3> dir1,
											 math::Vector<3> dir2,
											 const std::string fileName)
{
	if(!math::isOrthogonal(dir1, dir2)) {
		std::stringstream ss;
		ss << "Warning: Plot area direction is not orthogonal.\n"
			  << "dir1=" << dir1 << ", dir2=" << dir2 << "\nnot plotted.";
        outputMessage(OUTPUT_TYPE::MWARNING, ss.str());
		if(math::orthogonalize({&dir1, &dir2}, size_t(0)) == 2) {
			ss.clear();
			ss << "modified to " << dir1 << ", " << dir2;
            outputMessage(OUTPUT_TYPE::MWARNING, ss.str());
		} else {
			ss.clear();
			ss << "orthogonalization failed. Nothing done.";
            outputMessage(OUTPUT_TYPE::MWARNING, ss.str());
			return;
		}
	}
	auto bitmap = 	simulation_->plotSectionalImage(origin_ + offset -0.5*dir1 - 0.5*dir2,
													dir1, dir2,
													hResolution_, vResolution_,
													lineWidth_, pointSize_,
                                                    numThreads_, verbose_, quiet_
													);
#ifdef ENABLE_GUI
	(void)fileName;
	emit sectionPlotted(bitmap.exportToXpmString());
#else
	bitmap.exportToXpmFile(fileName);
#endif
}

void term::InteractivePlotter::outputMessage(OUTPUT_TYPE otype, const std::string &message){
#ifdef ENABLE_GUI
	emit sendMessage(QString::fromStdString(message));
#endif
	switch(otype) {
    case OUTPUT_TYPE::MFATAL:
	{
        mFatal{message};
		break;
	}
    case OUTPUT_TYPE::MWARNING:
	{
		mWarning() << message;
		break;
	}
    case OUTPUT_TYPE::MDEBUG:
	{
		mDebug() << message;
		break;
	}
//	default:
//		assert(!"unknown output type");
    }
}



/*
 * 複数コマンド実行は結構難しい
 *
 * MCNPで "pz 0 ex 100" とすると ex100 が実行されてからPZ0になる。
 * 後ろから解釈するのか？コマンドによって優先順位があるのか？
 * とりあえず;区切りで左側から複数コマンド実行する。
 */

term::InteractivePlotter::InteractivePlotter(std::shared_ptr<const Simulation> sim, int nt, bool verbose)
    : simulation_(sim), verbose_(verbose), quiet_(false),
	  exitFlag_(false), origin_(math::Point{0, 0, 0}),
	  hWidthCm_(DEFAULT_WIDTH_CM), vWidthCm_(DEFAULT_WIDTH_CM),
	  hResolution_(DEFAULT_HRESOLUTION), vResolution_(DEFAULT_VRESOLUTION),
	  lineWidth_(DEFAULT_LINEWIDTH), pointSize_(DEFAULT_POINTSIZE),
	  filename_(DEFAULT_FILENAME), numThreads_(nt),
	  terminal_("command > ")
{

	initCommandMap();
	if(simulation_ == nullptr) {
		throw std::invalid_argument("Simulation model is not defined.");
	}

#ifdef ENABLE_GUI
    emit plotterConfigChanged();
#endif
}


// コマンドライン文字列を解釈・実行
void term::InteractivePlotter::execCommandLineString(const std::string &commandLineStr)
{
	// 任意のコマンドが実行できるのはあまり良くないからデフォルトでは無効にする。
#ifdef ENABLE_SHELL_COMMAND
	// !で始まる場合はシェルに渡す。
	if(commandLineStr.substr(0, 1) == "!") {
		// sudoは禁止
		auto strVec = utils::splitString(" ;&\t", commandLineStr, true);
		for (auto & str: strVec) {
			if(str.find("sudo") != std::string::npos) {
				mWarning() << "Warning: sudo execution is forbiden.";
			}
		}
		std::system(commandLineStr.substr(1).c_str());
		return;
	}
#endif


	std::vector<std::string> commandList = utils::splitString(";", commandLineStr, true);
	// 読み取った文字列は空白で分解してパラメータに分ける。
	for(auto &commandLine: commandList) {
		std::vector<std::string> params, comlist = utils::splitString(" \t", commandLine, true);
		// comlistの先頭がコマンド名、以降がコマンドの引数パラメータ
		if(comlist.size() > 1)  {
			params.resize(comlist.size()-1);
			std::copy(comlist.begin()+1, comlist.end(), params.begin());
		}
		if(!comlist.empty()) {
			const std::string command = comlist.front();
			std::stringstream ss;
			if(comMap_.find(command) == comMap_.end()) {
				ss << "Invalid command \"" << command << "\".";
                outputMessage(OUTPUT_TYPE::MWARNING, ss.str());
			} else {
				try {
					comMap_.at(command)(params);
				} catch(std::out_of_range &e) {
					(void) e;
					ss << "In executing command  \"" << command << "\" , no such a command";
                    outputMessage(OUTPUT_TYPE::MWARNING, ss.str());
				} catch(std::invalid_argument &e) {
					ss << "Invalid arguments for command \"" << command << "\" params = "
							   << params << ", reason=" << e.what();
                    outputMessage(OUTPUT_TYPE::MWARNING, ss.str());
				}
			}
		}
	}
	// 面倒なのでとりあえずコマンド実行後は設定変更されたとしておく。
#ifdef ENABLE_GUI
	emit plotterConfigChanged();
#endif
}

void term::InteractivePlotter::start(const std::vector<std::string> &arguments)
{
	if(!arguments.empty()) {
		mDebug() << "initial plotting commands =" << arguments;
		for(auto com: arguments) {
			execCommandLineString(com);
		}
	}

	std::string buff;
	//CustomTerminal terminal (prompt);
	while(!exitFlag_) {
		terminal_.customGetline(std::cin, buff);
		execCommandLineString(buff);
	}
}

void term::InteractivePlotter::setNumThreads(int num)
{
	numThreads_ = num;
	mDebug() << "in SectionalViewer::setNumThreads, numThreads_ ===" << numThreads_;
}





