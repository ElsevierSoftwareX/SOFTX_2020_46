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
#ifndef GUICONFIG_HPP
#define GUICONFIG_HPP

#include <QFont>
#include <QStringList>

// GUI設定クラス。CUI設定を内包する。
#include "../core/option/config.hpp"
#include "../core/io/input/mcmode.hpp"
#include "../component/picojson/picojson.h"

/*
 * リスト
 * TODO 文字列化
 * TODO 文字列からの構築
 * 当然これらは対称に。fromString(toString)で元通りになるように。
 * 当然このまえにcuiConfigの文字列化が必要となる。
 */
struct GuiConfig
{
public:
	// GUIでguiConfigを設定・取得。引数はwidgetにセットする値
    static GuiConfig getGuiConfig(const GuiConfig &gconf,
                                  const std::map<std::string, img::MaterialColorData> &defaultColorMap);


	GuiConfig();
	// メンバ
	// 3Dビューの背景色(これはCUIでは使わないのでこちらで保持する。)
	img::Color bgColor3D;
	conf::Config cuiConfig;
	// メモリ使用量裕度
    //double memorySafetyFactor;
	// font
    QFont uiFont;  // UI用フォント
    QFont editorFont; // inputViewer用フォント


    McMode mcMode() const {return cuiConfig.mode;}
	std::string toString() const;

	// json関係
	picojson::value jsonValue() const;
    void dumpJson(std::ostream &os) const;
	static GuiConfig fromJsonFile(const std::string &fileName);
	static GuiConfig fromJsonString(const std::string &jsonStr);

private:

};

#endif // GUICONFIG_HPP
