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
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>




// 直接依存ヘッダ
#include "option/config.hpp"
#include "simulation.hpp"
#include "terminal/interactiveplotter.hpp"
#include "utils/message.hpp"

int main(int argc, char *argv[])
{
    namespace ip = inp::phits;

	std::vector<std::string> arguments;
    for(int i = 1; i < argc; i++) arguments.push_back(*(argv+i));


	/*
	 * セクションを処理する。優先度は本来以下のようになる。
	 * 優先度1(他のセクションに依存しない)
	 * ・transform  : surface, source処理に必要
	 * ・parameters : material処理に必要(xsdirが必要だから)
	 * 優先度2(優先度1のセクションにのみ依存)
	 * ・ source(regなし) : transformに依存、material処理に必要(粒子種が必要)
	 * ・ surface    : transformに依存、cell処理に必要
	 * 優先度3(同2に依存)
	 * ・ material   : parameters, sourceに依存、cell処理に必要
	 * 優先度4
	 * ・cell        : transform, surface, materialに依存
	 * 優先度5
	 * ・source(regあり)      : transform, cellに依存
	 * ・tally       : cell、surface、materialに依存
	 *
	 * ※sourceセクションはreg=でセルを指定している場合sourceはcellに依存してしまい、
	 * 依存性が循環してしまう。
	 *
	 * 対策方法1：
	 * sourceの処理は優先度5にして、particleは入力をスキャンしてproj=によって、事前にに取得してしまう。
	 * 対策方法2：
	 * sourceセルは暫定的にセル名だけ取得しておいて、simulation開始前にセルのインスタンスを得る
	 *
	 * → 性能的には2だろうが、1の方が整然とする。よって1を採用
	 */

	/*
	 * 先頭空白継続行が認められるセクション
	 * ・cell
	 * ・surface
	 * ・material
	 * ・transform
	 * 認められないセクション
	 * ・parameter
	 * ・source
	 * ・tally
	 *
	 * というように分かれている。
	 * 行連結の前に実施できるメタカード処理はファイルインクルードのみとしているので
	 * sectionに分ける前に実施できる作業はinclude処理のみとなる。
	 * よってgeometry,やsourceのconstruct時にはメタカード処理をそれぞれ実施しなければならない。
	 *
	 * 但し行連結はセクション構造に影響を与えないと考えられるのでセクション分けの後に
	 * meta処理をすれば良い
	 */

	conf::Config config;
	config.procOptions(&arguments);

	if(arguments.empty()) {
		config.PrintHelp();
		std::exit(EXIT_FAILURE);
	}

	std::string inputFileName = arguments.at(0);
	std::shared_ptr<Simulation> simulation = std::make_shared<Simulation>();
	simulation->init(inputFileName, config);

    if(config.verbose) mDebug() << simulation->finalInputText();

	if(config.ipInteractive) {
		// staticにしておかないと数行下でexitした時デストラクタが呼ばれない
		// デストラクタが呼ばれないとコンソールがカノニカルモードに戻らないので注意。
		static term::InteractivePlotter plotter(simulation, config.numThread, config.verbose);
		plotter.start(config.initialCommands);
		std::exit(EXIT_SUCCESS);
	}


//	// シミュレーション実行
//	simulation->run(config.numThread);
//	// 実行後にタリー結果を取得
//	for(auto &tally: simulation->getTallies()) {
//		tally->dump(std::cout);
//	}

	return 0;
}
