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
#ifndef INTERACTIVEPLOTTER_HPP
#define INTERACTIVEPLOTTER_HPP

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#ifdef ENABLE_GUI
#include <QObject>
#include <QString>
#endif

#include "core/math/nvector.hpp"
#include "core/simulation.hpp"
#include "customterminal.hpp"

// 先行宣言
enum class OUTPUT_TYPE;
namespace geom {
class Geometry;
}



namespace term {

class CustomTerminal;

#ifdef ENABLE_GUI
class InteractivePlotter: public QObject
{
	Q_OBJECT

signals:
	// SectionalViewerへmessageを送るsignal
	void sendMessage(const QString message);
	// xpmファイルをプロットしたらsignalでSectionalViewerへ送る。
	void sectionPlotted(const std::string &xpmStr);
    void plotterConfigChanged();
	void requestResize();
	void requestFittingResolutionToScreen();
	void requestPrintGuiHistory();
	void localNumThreadsChanged(int num);
#else
class InteractivePlotter
{
#endif
public:
	// コマンド処理関数は返り値void引数がvector<string>
	typedef std::function<void(const std::vector<std::string>&)> com_func_type;
	typedef std::map<std::string, com_func_type> map_type;

	InteractivePlotter(std::shared_ptr<const Simulation> sim, int nt, bool verbose);
	// interactiveではないコマンド実行関数。
	void execCommandLineString(const std::string &commandLineStr);
	// startするとwhile(true)中でcin読み取り→コマンド実行を続ける
	void start(const std::vector<std::string> &arguments = std::vector<std::string>());
	void setNumThreads(int num);

	// 現在設定のgetter
    const math::Point &origin() const {return origin_;}
	std::pair<size_t, size_t> resolution() const {return std::make_pair(hResolution_, vResolution_);}
	int lineWidth() const {return lineWidth_;}
	int numThreads() const {return numThreads_;}
    std::pair<size_t, size_t> width() const {return std::make_pair(static_cast<size_t>(hWidthCm_), static_cast<size_t>(vWidthCm_));}
	// 最後に表示した断面の法線,画面水平方向、画面縦方向ベクトル
	const math::Vector<3> &normal() const {return normal_;}
	const math::Vector<3> &hDir() const {return hDir_;}
	const math::Vector<3> &vDir() const {return vDir_;}

private:
	std::shared_ptr<const Simulation> simulation_;
    bool verbose_;
    bool quiet_;
	map_type comMap_;
	bool exitFlag_;
    math::Point origin_;
	double hWidthCm_;  // 出力するx方向の幅(単位:シミュレーションモデル内のcm)
	double vWidthCm_;
	size_t hResolution_;
	size_t vResolution_;
	int lineWidth_;
	int pointSize_;
	std::string filename_;
	int numThreads_;
	math::Vector<3> normal_;
	math::Vector<3> hDir_;
	math::Vector<3> vDir_;
	CustomTerminal terminal_;

	std::string showHelp() const;
	void initCommandMap();

	void plotSection(const math::Vector<3> &org,
					  math::Vector<3> dir1,
					  math::Vector<3> dir2,
					  const std::string fileName);
	// ENABLE_GUI状態ではSectionalViewerへメッセージを飛ばしたいので
	// このクラスでは直接mWarningやmDebug()は使わない
	// そうしないと一々出力毎に#ifdef ENABLE_GUIみたいなディレクティブが必要になるから。
	void outputMessage(OUTPUT_TYPE otype, const std::string& message);
};

}  // end namespace term

#endif // INTERACTIVEPLOTTER_HPP
