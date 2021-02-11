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
#include "config.hpp"

#include <fstream>
#include <list>
#include <map>

#include "core/utils/utils.hpp"
#include "core/utils/message.hpp"
#include "core/utils/system_utils.hpp"
#include "core/io/input/dataline.hpp"

#include "component/picojson/picojson.h"

namespace {
const std::string OPTSTART = "-";
}


#include <thread>

conf::Config::Config()
	: mode(McMode::AUTO),
      ipInteractive(false),
      quiet(false),
	  verbose(false),
	  warnPhitsIncompatible(false),
	  noXs(false),
	  timeoutBB(5000)
{
    numThread = static_cast<int>(std::thread::hardware_concurrency());
    if(numThread == 0) numThread = 1;

	optFuncMap_=std::map<std::string, funcpair_type>{
		// ここでオプション名キー、 オプション処理関数、 オプション説明関数を登録する。
		{"help", std::make_pair(
			[](conf::Config *conf, const std::string &optarg){
				(void) optarg;
				conf->PrintHelp();
			},
            []() {
				return  " :Show this help.";
			})
		},

		{"ip", std::make_pair(
			[](conf::Config *conf, const std::string &optarg){
				(void) optarg;
				conf->ipInteractive = true;
			},
            []() {
				return  " :Exec interactive geometry plot.";
			})
		},
		{"com", std::make_pair(
			[](conf::Config *conf, const std::string &optarg){
				conf->comfile_ = optarg;
			},
            []() {
				return  "=(filename) :Read initial plotting commands file.";
			})
		},
		{"v", std::make_pair(
			[](conf::Config *conf, const std::string &optarg){
				(void) optarg;
				conf->verbose = true;
			},
            []() {
                return  ": Enable verbose output.";
			})
		},
		{"t", std::make_pair(
			[](conf::Config *conf, const std::string &optarg){
				conf->numThread = utils::stringTo<int>(optarg);
			},
            []() {
				return  "=(num threads) :Set number of threads.";
			})
		},
		{"Wphits-compat", std::make_pair(
			[](conf::Config *conf, const std::string &optarg){
				(void) optarg;
				conf->warnPhitsIncompatible = true;
			},
			[]() {
				return  ":Check compatibility with phits code.";
			})
		},

		{"no-xs", std::make_pair(
			[](conf::Config *conf, const std::string &optarg){
				(void) optarg;
				conf->noXs = true;
			},
            []() {
				return  ":Do not read xs files.";
			})
		},
		{"xsdir", std::make_pair(
			[](conf::Config *conf, const std::string &optarg){
				conf->xsdir = optarg;
			},
            []() {
				return  "=(xsdir filepath) :Set XSDIR file.";
			})
        },
        {"quiet", std::make_pair(
                       [](conf::Config *conf, const std::string &optarg){
                           (void) optarg;
                           conf->quiet = true;
                           conf->verbose = false;
                       },
                       []() {
                           return  "Disable any output to stdout.";
                       })
        },
		{"color", std::make_pair(
			[](conf::Config *conf, const std::string &optarg){
				conf->colorFile = optarg;
			},
            []() {
				return  "=(color filepath) :Set phits-like color file.";
			})
		},
    };
}

std::string conf::Config::toString() const
{
    std::stringstream ss;
    ss << "mode = " << inp::getModeString(mode) << std::endl;
    ss << "ip_interactive = " << std::boolalpha << ipInteractive << std::endl;
    ss << "num threads = " << numThread << std::endl;
    ss << "verbose = " << std::boolalpha << verbose << std::endl;
    ss << "warn PHITS compat = " << std::boolalpha << warnPhitsIncompatible << std::endl;
    ss << "no xs = " << std::boolalpha << noXs << std::endl;
    ss << "xsdir = " << xsdir << std::endl;
	ss << "colorFile = " << colorFile;

    return ss.str();
}



void conf::Config::procOptions(std::vector<std::string> *args)
{
	for(auto it = args->begin(); it != args->end(); it++) {
		// オプション
		if(it->size() > OPTSTART.size() && it->substr(0, OPTSTART.size()) == OPTSTART) {

			std::string opt = it->substr(OPTSTART.size());
			auto optelements = utils::splitString("=", opt);  // オプション全体文字列
			assert(!optelements.empty());
			// optstr：オプション名、optarg：オプション引数
			std::string optstr = optelements.at(0);
			std::string optarg = (optelements.size() >= 2)? optelements.at(1) : "";

			it = args->erase(it);  // 引数からオプションを削除
			--it;

			try {
				optFuncMap_.at(optstr).first(this, optarg);
			} catch (...) {
				std::cerr << "Error: Invalid option, \"" + opt << "\"" << std::endl;
				std::exit(EXIT_FAILURE);
			}
		}
	}

	if(!this->comfile_.empty()) {
		std::ifstream comifs(utils::utf8ToSystemEncoding(this->comfile_).c_str());
		if(comifs.fail()) {
			mWarning() << "Warning: Com file \"" << this->comfile_ << "\" not found, ignored.";
		} else {
			std::string buff;
			while(getline(comifs, buff)) {
				utils::trim(&buff);
				if(!buff.empty()) {
					if(buff.substr(0, 1) == "!") {
						std::cerr << "Warning: Shell execution from comfile is forbidden. Ignored." << std::endl;
					} else {
						this->initialCommands.emplace_back(buff);
					}
				}
			}
		}
	}

	// 色ファイル読み込みはここで
	if(!colorFile.empty()) {
		size_t lineNumber = 0;
		std::list<inp::DataLine> colorCards;
		std::ifstream ifs(utils::utf8ToSystemEncoding(colorFile).c_str());
		if(ifs.fail()) {
			mWarning() << "Color file \"" + colorFile + "\" not found. Default color table was used.";
		} else {
			std::string buff;
			while(getline(ifs, buff)) {
				utils::sanitizeCR(&buff);
				colorCards.emplace_back(inp::DataLine(colorFile, ++lineNumber, std::move(buff)));
			}
		}
		colorMap = img::MaterialColorData::fromCardsToMap(colorCards);
	}
}

void conf::Config::PrintHelp()
{
	std::cout << "auto cell devider" << std::endl;
	std::cout << "USAGE: acd  [option] inputfile" << std::endl;
	std::cout << "OPTION:" << std::endl;

	for(auto &funcPairs: optFuncMap_) {
		std::cout << OPTSTART << funcPairs.first << funcPairs.second.second() << std::endl;
	}
    std::exit(EXIT_FAILURE);
}

void conf::Config::dumpJson(std::ostream &os) const
{
	std::string jsonStr = this->jsonValue().serialize();
	// TODO できればここでネストを加えたい
	os << jsonStr << std::endl;
}


/*
 *  JSONで保存できるデータは
 * ・データ(bool, double, string)
 * ・配列(データのシーケンス。異なる型の混在可能)
 * ・オブジェクト(文字列とデータ/配列をペアにしたmap。キーは文字列のみ。値は異なる型が混在して良い)
 */
#define VARNAME(ARG) #ARG
picojson::value conf::Config::jsonValue() const
{
    namespace pj = picojson;
    /*
     * こんな感じ。ipInteractive, initialCommandsなどはコマンドラインからの指定以外使わないメンバ変数は復元対象にしない。
     * {
     *    "config":[
     *       "mode":"phits",
     *       "numThread":4,
     *       "verbose":true,
     *       "warnPhitsImcompatible":false,
     *       "noXs":true,
     *       "timeoutBB":2000,
     *       "xsdir":"/hom/code/mcnp/xs/xsdir.all",
	 *		 "colormap":[]
     *    ]
     * }
     */
    pj::object configData;
    configData.insert(std::make_pair(VARNAME(mode), inp::getModeString(mode)));
    configData.insert(std::make_pair(VARNAME(numThread), static_cast<double>(numThread)));
    configData.insert(std::make_pair(VARNAME(verbose), verbose));
    configData.insert(std::make_pair(VARNAME(warnPhitsIncompatible), warnPhitsIncompatible));
    configData.insert(std::make_pair(VARNAME(noXs), noXs));
    configData.insert(std::make_pair(VARNAME(timeoutBB), static_cast<double>(timeoutBB)));
    configData.insert(std::make_pair(VARNAME(xsdir), xsdir));
	configData.insert(std::make_pair(VARNAME(colorMap), colorMapJsonValue()));

	return pj::value(configData);
}



conf::Config conf::Config::fromJsonFile(const std::string &fileName)
{
    namespace pj = picojson;
    std::ifstream ifs(fileName.c_str());
    if (ifs.fail()) {
        throw std::invalid_argument(std::string("Config file = ") + fileName + " cannot be opened.");
    }
    const std::string jsonStr((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	ifs.close();
	return fromJsonString(jsonStr);
}

conf::Config conf::Config::fromJsonString(const std::string &jsonStr)
{
	namespace pj = picojson;
	// JSONデータを解析する。
	pj::value v;
	const std::string err = pj::parse(v, jsonStr);
	if (!err.empty()) {
        throw std::invalid_argument(std::string("Parsing json file for cunfig failed. err = ") + err);
	}
	pj::object& obj = v.get<pj::object>();
	return fromJsonObject(obj);
}

conf::Config conf::Config::fromJsonObject(picojson::object obj)
{
	McMode mode = inp::stringToMcMode(obj[VARNAME(mode)].get<std::string>());
	int numThread =  static_cast<int>(obj[VARNAME(numThread)].get<double>());
	bool verbose = obj[VARNAME(verbose)].get<bool>();
	bool warnPhitsIncompatible = obj[VARNAME(warnPhitsIncompatible)].get<bool>();
	bool noXs = obj[VARNAME(noXs)].get<bool>();
	int timeoutBB = static_cast<int>(obj[VARNAME(timeoutBB)].get<double>());
	std::string xsdir = obj[VARNAME(xsdir)].get<std::string>();
	auto colMap = colorMapFromJsonObject(obj[VARNAME(colorMap)].get<picojson::object>());

	Config conf;
	conf.mode =  mode;
	conf.numThread = numThread;
	conf.verbose = verbose;
	conf.warnPhitsIncompatible = warnPhitsIncompatible;
	conf.noXs = noXs;
	conf.timeoutBB = timeoutBB;
	conf.xsdir = xsdir;
	conf.colorMap = colMap;
	return conf;
}




picojson::value conf::Config::colorMapJsonValue() const
{
	/* colormap <string, MatNameColor>のjson化
	 *  MatNameColorは
	 *  string matName_;
	 * 	string printName_;
	 * 	double printSize_;
	 * 	std::shared_ptr<Color> color_;
	 * で構成されている。
	 *
	 * よってcolorMapのpj::valueは
	 *
	 *		"1":[
	 *			"matname":"1",
	 *			"printname":"Fe",
	 *			"printsize":1.5,
	 *			"color":[255, 0, 255, 1]
	 *		],
	 *
	 * 		"2":[
	 *			"matname":"2",
	 *			"printname":"H2O",
	 *			"printsize":1.5,
	 *			"color":[0, 0, 255, 1]
	 *		]
	 *
	 *
	 * なのでMatNameColor::JsonValue();を実装すべし。
	 */
	namespace pj = picojson;
	pj::object colorMapObj;
	for(auto it = colorMap.cbegin(); it != colorMap.cend(); ++it) {
		// it->first: 材料名
		// it->second MatNameColorデータ
		colorMapObj.insert(std::make_pair(it->first, it->second.jsonValue()));
	}
	return pj::value(colorMapObj);
}



std::map<std::string, img::MaterialColorData> conf::Config::colorMapFromJsonObject(picojson::object obj)
{
	// キーはmaterial名なので事前にexplicitにはわからない。
	// なので全キースキャンする。pj::objはstd::map<string, pj::value)のtypedef.
	// これをstd::map<string, img::MatNameColor>に変換することが目的
	std::map<std::string, img::MaterialColorData> colMap;
	for(auto it = obj.cbegin(); it != obj.cend(); ++it) {
		// ここでit->firstはキー名、
		//it->secondはpicojson::value
		// {
		//	"color_": {"a": 255,"b": 118,"g": 14,"r": 255},
		//	"matName_": "4",
		//	"printName_": "4",
		//	"printSize_": 1
		// }
		// こんな感じ
		colMap.emplace(it->first, img::MaterialColorData::fromJsonObject(it->second.get<picojson::object>()));
	}
	return colMap;
}

#undef VARNAME











