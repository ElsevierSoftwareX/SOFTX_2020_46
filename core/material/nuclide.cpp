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
#include "nuclide.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "component/libacexs/libsrc/mt.hpp"
#include "core/utils/string_utils.hpp"
#include "core/utils/system_utils.hpp"
#include "core/utils/message.hpp"

std::unordered_map<std::string, std::shared_ptr<const mat::Nuclide>> mat::Nuclide::nuclidePool;
std::mutex mat::Nuclide::poolMtx;

#ifndef NO_ACEXS
mat::Nuclide::Nuclide(const std::string &zaidx, double awr, const ace::AceFile::XSmap_type &xsmap)
	:zaidx_(zaidx), awr_(awr), xsMap_(xsmap)
{
	//mDebug() << "creating nuclide, zaid===" << zaidx << "awr===" << awr;
	if(!xsmap.empty()) {
		ace::NTY nty = ace::getNtyFromZaidx(zaidx_);
		ace::Reaction	totalReaction;
		if(nty == ace::NTY::CONTINUOUS_NEUTRON || nty == ace::NTY::DISCRETE_NEUTRON || nty == ace::NTY::MULTIGROUP_NEUTRON) {
			totalReaction = ace::Reaction::TOTAL;
		} else if (nty == ace::NTY::CONTIUNOUS_PHOTOATOMIC || nty == ace::NTY::PHOTONUCLEAR) {
			totalReaction = ace::Reaction::TOTAL_PHOTON_INTERACTION;
		} else if (nty == ace::NTY::DOSIMETRY) {
			// ドシメトリファイルには全断面積が存在しないのでとりあえず存在するReactionを適当に割り当てる。
			totalReaction = xsMap_.begin()->first;
		} else {
			std::cerr << "ProgramError: Only photon and neutron XSs are implimented." << std::endl;
			std::exit(EXIT_FAILURE);
		}
		totalXs_ = xsMap_.at(totalReaction);
	}
}
#endif

std::string mat::Nuclide::toString() const
{
	std::stringstream ss;
	ss << zaid();
	return ss.str();
}

#ifndef NO_ACEXS
double mat::Nuclide::getTotalXs(double energy) const
{
	// ace::CrossSection totalXs_が（粒子種によって適切にMTナンバーを選択して構築した）全断面積なのでそれを返す。
	return totalXs_.getValue(energy);
}
#endif

// ACEファイルの読み取りはここ
std::shared_ptr<const mat::Nuclide> mat::Nuclide::createNuclide(const std::string &filename, double awr,
														  const std::string &zaidx, std::size_t startline)
{
#ifndef NO_ACEXS
    std::lock_guard<std::mutex>lg(poolMtx);
	if(nuclidePool.find(zaidx) == nuclidePool.end()) {
		std::ifstream ifs(utils::utf8ToSystemEncoding(filename).c_str());
		if(ifs.fail()) {
            // NOTE xsdirファイルの文字コードはUTF8でなければならない。
			throw std::invalid_argument(std::string("No such a nuclide file = ") + filename + " for ZAIDX = " + zaidx);
		} else if(!ace::isSZAX(zaidx) && !ace::isZAIDX(zaidx)) {
			throw std::invalid_argument(std::string("Invalid zaidx/szax, = ") + zaidx);
		}

		try {
			std::unique_ptr<ace::AceFile> aceFile = ace::AceFile::createAceFile(filename, zaidx, startline);
//			nuclidePool.emplace(zaidx, std::shared_ptr<Nuclide>(new Nuclide(zaidx, awr, aceFile->getXsMap())));
			auto nuc = std::make_shared<Nuclide>(zaidx, awr, aceFile->getXsMap());
			nuclidePool.emplace(zaidx, std::move(nuc));
		} catch (std::exception &e) {
            std::stringstream ss;
            ss << "While reading ace file = " << filename << ", reason = " << e.what();
            throw std::invalid_argument(ss.str());
		}
	}
    return nuclidePool.at(zaidx);
#else
    (void) filename;
    (void) awr;
    (void) zaidx;
    (void) startline;
    return std::shared_ptr<const mat::Nuclide>();
#endif
}

bool mat::Nuclide::ZaidLess(const std::string &zaid1, const std::string &zaid2)
{
	int za1, za2;
	auto pos1 = zaid1.find("."), pos2 = zaid2.find(".");
	std::string id1, id2;
	if(pos1 != std::string::npos) {
		za1 = utils::stringTo<int>(zaid1.substr(0, pos1));
		id1 = zaid1.substr(pos1);
	} else {
		za1 = utils::stringTo<int>(zaid1);
	}
	if(pos2 != std::string::npos) {
		za2 = utils::stringTo<int>(zaid2.substr(0, pos2));
		id2 = zaid2.substr(pos2);
	} else {
		za2 = utils::stringTo<int>(zaid2);
	}

	if(za1 == za2) {
		return id1 < id2;
	} else {
		return za1 < za2;
	}
}

bool mat::Nuclide::NuclideLess(const mat::Nuclide &nuc1, const mat::Nuclide &nuc2)
{
	// zaidはZ、Aを整数比較してIDは適当に文字列比較する。
	std::string zaid1 = nuc1.zaid(), zaid2 = nuc2.zaid();
	return ZaidLess(zaid1, zaid2);
}

bool mat::Nuclide::NuclidePLess(const std::shared_ptr<const mat::Nuclide> &nuc1, const std::shared_ptr<const mat::Nuclide> &nuc2)
{
	return NuclideLess(*nuc1.get(), *nuc2.get());
}

std::ostream &mat::operator <<(std::ostream &os, const mat::Nuclide &nuc)
{
	os << nuc.toString();
	return os;
}
std::ostream &mat::operator <<(std::ostream &os, const std::pair<std::shared_ptr<const mat::Nuclide>, double> &nucPair)
{
	os << "{" << *(nucPair.first.get()) << ", " << nucPair.second <<"}";
	return os;
}


