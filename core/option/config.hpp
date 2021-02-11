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
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "core/io/input/mcmode.hpp"
#include "core/image/matnamecolor.hpp"

namespace picojson {
class value;
}


namespace conf {


struct Config {


	Config();
	McMode mode;
	bool ipInteractive;
	int numThread;
    bool quiet;
	bool verbose;
	bool warnPhitsIncompatible; // phits互換性警告
	bool noXs;  // 断面積ファイルを読まないフラグ
//	bool useIntegerName;  // 面やセルの名前を整数として扱う
	// BB計算タイムアウト(ms)
	int timeoutBB;
	std::string xsdir; // コマンドラインからxsdirを設定する場合ここにパスを格納。
	// 色情報
	std::map<std::string, img::MaterialColorData> colorMap;

    // アプリケーション起動後に実行したい(ipモードでの)コマンドを保存する
	std::vector<std::string> initialCommands;

	// 確認のためのstring化ルーチン
    std::string toString() const;
    void procOptions(std::vector<std::string> *args);
	void PrintHelp();

	// json関係 configはGuiConfigから参照される場合とそれ単体で使われる場合がある。
	// ゆえにインターフェイスにはfromFileとfromStringを整備する。
	// toJsonはstreamを引数に採ることでfile,string共用にした。
	picojson::value jsonValue() const;
	void dumpJson(std::ostream &os) const;
	static Config fromJsonFile(const std::string &fileName);
	static Config fromJsonString(const std::string &jsonStr);
	static Config fromJsonObject(picojson::object obj);


private:

	// TODO comfile_はinitialcommandsと、colorFileはcolorMapと重複するので削除可能なはず。
	//       前者が確定したら直後に後者を生成し、procOptionsまで待たないようにすれば良い。

	// privateメンバは外部から参照しないこと。（privateだからできないけど）
	std::string comfile_;
	using funcpair_type = std::pair<std::function<void(conf::Config*, const std::string&)>, std::function<std::string(void)>>;
	// オプション設定関数(返:void,引:opt引数stringとConfig*)・オプション説明関数(返：説明文string,引：void)のペアを格納するマップ
	std::map<std::string, funcpair_type> optFuncMap_;
	std::string colorFile; // 色指定ファイル(あれば)

	picojson::value colorMapJsonValue()const;
//	static std::map<std::string, img::MatNameColor> colorMapFromJsonString(const std::string &jsonStr);
	static std::map<std::string, img::MaterialColorData> colorMapFromJsonObject(picojson::object obj);

};


}  // end namespace conf
#endif // CONFIG_HPP
