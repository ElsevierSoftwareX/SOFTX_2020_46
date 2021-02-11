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
#ifndef MATERIALS_HPP
#define MATERIALS_HPP

#include <atomic>
#include <future>
#include <list>
#include <memory>
#include <string>

#include "material.hpp"

namespace inp {
class DataLine;
}

namespace ace {
class XsDir;
}

namespace mat {
/*
 * Simulationで使われるMaterialクラスの集合体。
 *
 * xsdir,Aceファイル共に結構なサイズなので、
 * 必要になるまでは実データを読まずにfutureで処理すること。
 */
class Materials {
public:
	/*
	 * 読み込みデータはMaterialカードのリストとしたい。mcnpと互換性が取れるので
	 * ※ xsdirファイル名はMaterialカードには含まれないので注意。
	 */
	Materials(std::list<inp::DataLine> matCards,
			  const std::string &xsdirFileName,
			  const std::vector<phys::ParticleType> &ptypes,
              std::atomic_size_t &counter,
        bool verbose);



	/*
	 *  Publicメソッドで必要とされているのは...
	 * xsdirファイル
	 */

	const std::shared_ptr<const ace::XsDir> &xsdir() const;
	std::unordered_map<std::string, std::shared_ptr<const Material>> materialMapByName() const {return materialMapByName_;}
	bool empty() const {return materialMapByName_.empty();}
	void clear();
	const std::vector<phys::ParticleType> &particleTypes() const {return particleTypes_;}

private:
	//int numMaterials_;  // 今までに生成したインスタンスの数
	std::vector<phys::ParticleType> particleTypes_;
	std::shared_ptr<const ace::XsDir> xsdir_;
	std::unordered_map<std::string, std::shared_ptr<const Material>>  materialMapByName_;// 材料名-Materialのマップ
    std::unordered_map<std::string, int> nameIdMap_;                   // 材料名-材料IDのマップ TODO これいらないんじゃ？
	std::unordered_map<int, std::shared_ptr<Material>> materialMap_;  // 材料ID-Materialのマップ

	// nuclideMapも必要


	const std::unordered_map<std::string, int> &nameIdMap() {return nameIdMap_;}  // TODO これはいらないのでは？
	int nameToId(const std::string & name);
	std::string idToName(const int id);



};



} // end namespace mat
#endif // MATERIALS_HPP
