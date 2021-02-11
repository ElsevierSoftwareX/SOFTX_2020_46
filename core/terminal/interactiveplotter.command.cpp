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

#include "core/utils/utils.hpp"


// "com2"コマンドを"com1"と同等のコマンドとしてmapに登録する。
void RegisterCommandAlias(const std::string &com1, const std::string& com2,
								term::InteractivePlotter::map_type *map)
{
	try {
		map->operator [](com2) = map->at(com1);
	} catch (...) {
		std::cerr << "ProgramError: com1=\"" << com1 << "\" is not found in plotter command map." << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

// argument数が満たされているかチェック
void CheckNumberOfParms(const size_t sz, const std::vector<std::string> args, bool strict = false)
{
	if(strict) {
		if(args.size() != sz) {
			throw std::invalid_argument(std::string("Number of arguments should be ") + std::to_string(sz));
		}
	} else {
		if(args.size() < sz)  {
			throw std::invalid_argument("Too few argument");
		}
	}

	for(size_t i = 0; i < sz; ++i) {
		if(!utils::isArithmetic(args.at(i))) {
			std::stringstream ss;
			ss << "Argument at i=" << i << " is not a number, " << args.at(i);
			throw std::invalid_argument(ss.str());
		}
	}
}

std::string term::InteractivePlotter::showHelp() const
{
	std::stringstream ss;
	ss << "h|help:                 Show this help." << std::endl;
	ss << "q|quit                  Quit plotter" << std::endl;
    ss << "quiet                  Quiet plotter" << std::endl;
	ss << "conf                    Show current conf" << std::endl;
	ss << "ex (xwidth) (ywidth)    Set plotting xy range" << std::endl;
	ss << "o|origin (x) (y) (z)   Set origin to (x,y,z)" << std::endl;
	ss << "r|resolution (h) [v=h] Set horizontal and vertical resolutions in pixels" << std::endl;
#ifdef ENABLE_GUI
	ss << "fit                   adjust resolution to fit current window size" << std::endl;
#endif
	ss << "t (nt <256)            Set number of threads used in plot sections" << std::endl;
	ss << "Acceptable commands =" << std::endl;
	for(auto &funcPair: comMap_) {
		ss << funcPair.first << ", ";
	}
	ss << std::endl;
	return ss.str();
}


void term::InteractivePlotter::initCommandMap()
{
	typedef const std::vector<std::string> CVEC;
	comMap_["q"] = [&](CVEC& args) {(void)args; exitFlag_ = true;};
    comMap_["quiet"] = [&](CVEC& args) {(void)args; quiet_ = true;};
    comMap_["h"] = [&](CVEC& args) {(void)args; outputMessage(OUTPUT_TYPE::MWARNING, showHelp());};
	comMap_["history"] = [&](CVEC& args)
					{
						(void)args;
						// GUI環境ではterminal_は使われていないのでGUIへリクエストをemitする。
#ifdef ENABLE_GUI
						emit requestPrintGuiHistory();
#else
						auto hist = terminal_.history();
						std::stringstream ss;
						mDebug() << "history stack size=====" << terminal_.history().size();
						ss << "HISTORY : \n";
						for(auto it = hist.begin(); it != hist.end(); ++it) {
							ss << *it << std::endl;
						}
                        outputMessage(OUTPUT_TYPE::MDEBUG, ss.str());
#endif
					};
	comMap_["conf"] = [&](CVEC& args)
					{
						(void)args;
						std::stringstream ss;
						ss << "Origin = " << origin_ << "\n"
							<< "Plot region width (horizontal, vertical) = (" << hWidthCm_ << ", " << vWidthCm_ << ")\n"
							<< "Plot resolusion =" << hResolution_ << "x" <<vResolution_ << "\n"
							<< "Line width = " << lineWidth_ << "\n"
							<< "Output file = " << filename_ << "\n"
							<< "Number of thread when section tracing = " <<  numThreads_ << std::endl;
                        outputMessage(OUTPUT_TYPE::MDEBUG, ss.str());

					};
	comMap_["o"] = [&](CVEC& args)
					{
						CheckNumberOfParms(3, args, true);
						origin_ = math::Point{utils::stringTo<double>(args.at(0)),
										   utils::stringTo<double>(args.at(1)),
										   utils::stringTo<double>(args.at(2))};
					};
	/*
	 * p[xyz]はmcnpのプロッターでは実行すると、断面までoriginが動くようになっている。
	 * origin=(0,0,0)の状態でpz 100を実行すると origin=(0,0,100)となる。
	 * 面倒だがそっちの挙動に合わせてみる。
	 */
	comMap_["px"] = [&](CVEC& args)
					{
						CheckNumberOfParms(1, args, true);
						const double xpos = utils::stringTo<double>(args.at(0));
						math::Vector<3> dir1{0, hWidthCm_, 0}, dir2{0, 0, vWidthCm_};
						//math::Vector<3> offset{xpos, 0, 0};
						// this->plotSection(offset, dir1, dir2, filename_);
						origin_ = math::Point{xpos, origin_.y(), origin_.z()};
						this->plotSection(math::Vector<3>{0, 0, 0}, dir1, dir2, filename_);
						normal_ = math::Vector<3>{1, 0, 0};
						hDir_ = math::Vector<3>{0, 1, 0};
						vDir_ = math::Vector<3>{0, 0, 1};
					};
	comMap_["py"] = [&](CVEC& args)
					{
						CheckNumberOfParms(1, args, true);
						const double ypos = utils::stringTo<double>(args.at(0));
						math::Vector<3> dir1{hWidthCm_, 0, 0}, dir2{0, 0, vWidthCm_};
						//math::Vector<3> offset{0, ypos, 0};
						//this->plotSection(offset, dir1, dir2, filename_);
						origin_ = math::Point{origin_.x(), ypos, origin_.z()};
						this->plotSection(math::Vector<3>{0, 0, 0}, dir1, dir2, filename_);
						normal_ = math::Vector<3>{0, 1, 0};
						hDir_ = math::Vector<3>{1, 0, 0};
						vDir_ = math::Vector<3>{0, 0, 1};
					};
	comMap_["pz"] = [&](CVEC& args)
					{
						CheckNumberOfParms(1, args, true);
						const double zpos = utils::stringTo<double>(args.at(0));
						math::Vector<3> dir1{hWidthCm_, 0, 0}, dir2{0, vWidthCm_, 0};
						//math::Vector<3> offset{0, 0, zpos};
						//this->plotSection(offset, dir1, dir2, filename_);
						origin_ = math::Point{origin_.x(), origin_.y(), zpos};
						this->plotSection(math::Vector<3>{0, 0, 0}, dir1, dir2, filename_);
						normal_ = math::Vector<3>{0, 0, 1};
						hDir_ = math::Vector<3>{1, 0, 0};
						vDir_ = math::Vector<3>{0, 1, 0};
					};
	comMap_["ex"] = [&](CVEC& args)
					{
						CheckNumberOfParms(1, args);
						hWidthCm_ = utils::stringTo<double>(args.at(0))*2;
						if(args.size() >= 2) {
							vWidthCm_ = utils::stringTo<double>(args.at(1))*2;
						} else {
							vWidthCm_ = hWidthCm_;
						}
					};
	comMap_["r"] = [&](CVEC& args)
					{
#ifndef ENABLE_GUI
						CheckNumberOfParms(1, args);
#endif
						if(args.size() == 0) {
#ifdef ENABLE_GUI
							emit requestResize();
#endif
						} else {
							hResolution_ = utils::stringTo<size_t>(args.at(0));
							if(args.size() >= 2) {
								vResolution_ = utils::stringTo<size_t>(args.at(1));
							} else {
								vResolution_ = hResolution_;
							}
						}
					};
	comMap_["t"] = [&](CVEC& args)
					{
						CheckNumberOfParms(1, args, true);
						auto nt = utils::stringTo<int>(args.at(0));
						std::stringstream ss;
						if(nt <= 0) {
							ss << "Number of thread should be >0, ignored.";
                            outputMessage(OUTPUT_TYPE::MWARNING, ss.str());
						} else {
							if(numThreads_ != nt) {
								numThreads_ = nt;
#ifdef ENABLE_GUI
								// GUI環境下ではnumThreads_の変更をmainWindowに伝える。
								emit localNumThreadsChanged(numThreads_);
#endif
							}
						}
					};
	comMap_["lw"] =[&](CVEC& args)
					{
						CheckNumberOfParms(1, args);
                        lineWidth_ = utils::stringTo<int>(args.at(0));
					};
#ifdef ENABLE_GUI
	comMap_["fit"] =[&](CVEC &args)
					{
						CheckNumberOfParms(0, args);
						emit requestFittingResolutionToScreen();
					};
#endif

	/*
	 * p 原点オフセットvec,  hvec, vvec,  の9入力 hvecとvvecは直交していること！
	 */
	comMap_["p"] = [&](CVEC& args)
					{
						CheckNumberOfParms(9, args);
						std::vector<double> dargs = utils::stringVectorTo<double>(args);
						math::Vector<3> offset{dargs[0], dargs[1], dargs[2]};
						math::Vector<3> dir1{dargs[3], dargs[4], dargs[5]};
						math::Vector<3> dir2{dargs[6], dargs[7], dargs[8]};

						if(!math::isOrthogonal(dir1, dir2)) {
							std::stringstream ss;
							ss << "horizontal direction and vertical direction is not orthogonal";
                            outputMessage(OUTPUT_TYPE::MWARNING, ss.str());
							ss.clear();
							if(math::orthogonalize({&dir1, &dir2}, 0) == 2) {
								ss << "modified to" << dir1 << ", " << dir2;
                                outputMessage(OUTPUT_TYPE::MWARNING, ss.str());
							} else {
								ss << "orthogonalization failed. Nothing done.";
                                outputMessage(OUTPUT_TYPE::MWARNING, ss.str());
								return;
							}
						}
						this->plotSection(offset, dir1, dir2, filename_);
						normal_ = math::crossProd(dir1, dir2).normalized();
						hDir_ = dir1;
						vDir_ = dir2;
					};

	RegisterCommandAlias("h", "help", &comMap_);
	RegisterCommandAlias("q", "quit", &comMap_);
	RegisterCommandAlias("q", "end", &comMap_);
	RegisterCommandAlias("o", "origin", &comMap_);
	RegisterCommandAlias("r", "resolution", &comMap_);

}


