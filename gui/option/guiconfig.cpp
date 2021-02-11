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
#include "guiconfig.hpp"

#include <fstream>

#include <QLocale>
#include <QString>

#include "../../core/utils/message.hpp"
#include "../../core/utils/json_utils.hpp"
#include "guiconfigdialog.hpp"

namespace {

// rgb = 0.2 0.3 0.4
constexpr char DEFAULT_BGCOLOR3D[] = "#334d66";

}

GuiConfig GuiConfig::getGuiConfig(const GuiConfig &gconf,
                                  const std::map<std::string, img::MaterialColorData> &defaultColorMap)
{
    GuiConfigDialog gcd(Q_NULLPTR, gconf, defaultColorMap);
	auto result = gcd.exec();
	if(result == QDialog::Accepted) {
		auto retConf = gcd.getCurrentConfig();
		return retConf;
	} else {

		return gconf;
	}
}

GuiConfig::GuiConfig()
	:bgColor3D(img::Color(DEFAULT_BGCOLOR3D, 1.0))
{
	// 日本語デフォルトフォントはメイリオに設定。MS gothic uiはサイズによってはvtkで表示されないので
	QString systemLocaleName = QLocale::system().name();		// "ja_JP"
	systemLocaleName.truncate(systemLocaleName.lastIndexOf('_')); // "ja"
	if(systemLocaleName == "ja") {
#ifdef __APPLE__
		// 	uiFont = (QFont("ヒラギノ角ゴシック")); //
#else
		// linux ではmeiryo指定すればfontconfigが適当にvlgothicあたりにしてくれるので区別する必要なし。
		uiFont = QFont("Meiryo UI");
#endif
	}
	editorFont = uiFont;
}

std::string GuiConfig::toString() const
{
    std::stringstream ss;
    ss << jsonValue().serialize(true);
    return ss.str();
}






#define VARNAME(ARG) #ARG
picojson::value GuiConfig::jsonValue() const
{
	/*
	 *  メンバは
	 * 	conf::Config cuiConfig;
	 *  QFont uiFont;  // UI用フォント
	 *  QFont editorFont; // inputViewer用フォント
	 *
	 */
	picojson::object obj;
	obj.insert(std::make_pair(VARNAME(bgColor3D), bgColor3D.jsonValue()));
	obj.insert(std::make_pair(VARNAME(cuiConfig), cuiConfig.jsonValue()));
    obj.insert(std::make_pair(VARNAME(uiFont), utils::jsonValue(uiFont)));
    obj.insert(std::make_pair(VARNAME(editorFont), utils::jsonValue(editorFont)));
	return picojson::value(obj);
}

void GuiConfig::dumpJson(std::ostream &os) const
{
	os << this->jsonValue().serialize(true) << std::endl;
}



GuiConfig GuiConfig::fromJsonString(const std::string &jsonStr)
{
	using Pobj = picojson::object;
	picojson::value val;
	const std::string err = picojson::parse(val, jsonStr);
	if (!err.empty()) {
		mFatal("Parsing json file for guicunfig failed. err = ", err);
	}

	GuiConfig conf;
	auto obj = val.get<Pobj>();
	if(obj[VARNAME(bgColor3D)].is<Pobj>()) conf.bgColor3D = img::Color::fromJsonObject(obj[VARNAME(bgColor3D)].get<Pobj>());
	if(obj[VARNAME(uiFont)].is<Pobj>()) conf.uiFont = utils::fromJsonObject(obj[VARNAME(uiFont)].get<Pobj>());
	if(obj[VARNAME(editorFont)].is<Pobj>()) conf.editorFont = utils::fromJsonObject(obj[VARNAME(editorFont)].get<Pobj>());
	if(obj[VARNAME(cuiConfig)].is<Pobj>()) conf.cuiConfig = conf::Config::fromJsonObject(obj[VARNAME(cuiConfig)].get<Pobj>());
	return conf;
}


GuiConfig GuiConfig::fromJsonFile(const std::string &fileName)
{
	std::ifstream ifs(fileName.c_str());
	if(ifs.fail()) {
		mWarning() << "Configuration file" << fileName << "not found, using default config.";
		return GuiConfig();
	}
	return GuiConfig::fromJsonString(std::string((std::istreambuf_iterator<char>(ifs)),
												   std::istreambuf_iterator<char>()));
}


#undef VARNAME


