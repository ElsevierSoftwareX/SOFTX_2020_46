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
#ifndef INPUTDATA_HPP
#define INPUTDATA_HPP

#include <list>
#include <limits>
#include <fstream>
#include <vector>
#include "dataline.hpp"
#include "mcmode.hpp"
#include "phits/phits_metacards.hpp"
#include "common/trcard.hpp"
#include "core/physics/physconstants.hpp"
#include "core/option/config.hpp"

enum class McMode;

namespace inp {


/*
 * MCNP, Phitsの入力データ共通処理
 * 1.ファイル読み込み、
 * 2.インクルード処理
 * 3.コメント削除
 * 4.IJMR展開
 * を行う。
 *
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


// std::list<DataLine>をラップ
#ifdef ENABLE_GUI
#include <QObject>
class InputData: public QObject
{
	Q_OBJECT
signals:
	void fileOpenSucceeded(std::pair<std::string, std::string> filePair) const;
#else
class InputData
{
#endif

// static
public:
	static void removeComment(McMode mode, std::list<DataLine> *inputData);
	static void concatLine(std::list<DataLine> *inputData, bool enablePreConcat, bool warnPhitsCompat);
	static void replaceSemicolon(std::list<DataLine> *inputData, bool warnPhitsCompat);  // phitsでは;で行内複数入力できるので;は改行に置き換える
	static void expandIjmr(std::list<DataLine> *inputData, bool warnPhitsCompat);
    static void replaceConstants(std::list<DataLine> *inputData);
    static std::string getAlternativeXsdirFileName(bool quiet);
	static void toLowerDataLines(std::list<inp::DataLine> *lines);

public:
	InputData(const conf::Config &config);
	virtual ~InputData(){;}

    McMode mode() const {return config_.mode;}  // deprecated
	std::list<DataLine>* dataLines() {return &dataLines_;}
	const std::string &path() const {return path_;}
	const std::string &inputFile() const {return inputFile_;}
	// xsdirファイルの実在を確認する。
	bool confirmXsdir() const;

	// インターフェイス
	virtual void init(const std::string& inputFileName);
	virtual std::vector<phys::ParticleType> particleTypes() const = 0;

    const std::string &xsdirFilePath() const {return config_.xsdir;}
	const std::list<DataLine> &cellCards() const {return cellCards_;}
	const std::list<DataLine> &surfaceCards() const {return surfaceCards_;}
	const std::list<DataLine> &materialCards() const {return materialCards_;}
	const std::list<DataLine> &transformCards() const {return transformCards_;}
	// 色指定があるのはphitsだけである。
	virtual std::list<DataLine> colorCards() const {return std::list<DataLine>();}

	// データダンプ
	void dump(std::ostream &os)  const;
	void dumpData(std::ostream &os)  const;
	static void dump(std::ostream& os, const std::list<DataLine>& dataList);
	static void dumpData(std::ostream& os, const std::list<DataLine>& dataList);

protected:
    conf::Config config_;
//  McMode mode_;
//	bool warnPhitsCompat_;
//  std::string xsdirFileName_;  // コマンドラインから与えられたxsdirファイル名
	std::string path_; // 入力ファイルの存在するデイレクトリパス
	std::string inputFile_;

	std::list<DataLine> dataLines_;

	std::list<DataLine> cellCards_;
	std::list<DataLine> surfaceCards_;
	std::list<DataLine> materialCards_;
	std::list<DataLine> transformCards_;

	std::list<DataLine> readFile(const std::string &parentFileName, std::string inputFileName, bool echo = true,
								 std::size_t startLine = 0 , std::size_t endLine = phits::MAX_LINE_NUMBER, bool warnPhitsCompat = false);
};


}  // end namespace inp

#endif // INPUTDATA_HPP
