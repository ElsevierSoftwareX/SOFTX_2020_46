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
#ifndef QADINPUT_HPP
#define QADINPUT_HPP

#include <array>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "core/io/input/cg/cgbody.hpp"
#include "core/io/input/cg/cgzone.hpp"
#include "core/physics/particle/particle.hpp"
#include "core/math/nmatrix.hpp"
#include "core/io/input/mcmode.hpp"
#include "core/io/input/inputdata.hpp"


namespace conf{
struct Config;
}

namespace inp {

class DataLine;


namespace qad {




class QadInput: public InputData
{
public:
    QadInput(const conf::Config &conf):InputData(conf) {config_.mode = McMode::QAD;}
	virtual ~QadInput(){;}
	// 仮想関数
	void init(const std::string &inputFileName) override final;
	std::vector<phys::ParticleType> particleTypes() const override final;

private:
	// QAD固有の変数
	std::string title_;
	// コントロールパラメータ。意味はQADマニュアル参照
	int lso_;
	int mso_;
	int nso_;
	int mat_;
	int ncomp_;
	int nreg_;
	int nrgy_;
	int nbound_;
	int nsopt_;
	int nzso_;
	int isrc_;
	int ineut_;
	int ngpf_;
	int ngpl_;
	int ngpi_;
	int ngint_;

	// ソースパラメータ
	double aso_;
	std::array<std::array<double, 3>, 2> xiso_;
	std::vector<double> rso_, zso_, phiso_;
	std::vector<double> fl_, fm_, fn_;

	// ジオメトリパラメータ
	int ivopt_;
	int idbg_;
	std::string jty_;
	std::vector<CGBody> cgbs_;
	std::vector<CGZone> cgcs_;
	std::vector<int> mmiz_;

	// 材料パラメータ
	int nbld_;
	std::vector<int> matz_;
	std::string matgp_;
	std::string icgp_;
	double congy_;
	std::string matdos_;
	std::vector<std::vector<double>> comps_;

	// 以下は物理的に意味のないデータ

	// 正味データ行番号と実ファイル行番号の対応表
	std::map<int, int> lineTable_;
	// (コメント削除後の)正味データの行番号を実ファイルの行番号へ換算する。
	int toRealLineNumber(int lineNumber) const;
	void dumpQadVars(std::ostream &os) const;

	// CGBカード文字列からIALPを返す。
	static std::string kgetIALP(const std::string &str) {return str.substr(2, 3);}
};

}  // end namespace qad
}  // end namespace inp
#endif // QADINPUT_HPP
