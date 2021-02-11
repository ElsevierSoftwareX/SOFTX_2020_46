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
#ifndef CELL3DEXPORTERC_HPP
#define CELL3DEXPORTERC_HPP

#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <QDir>

#include <vtkSmartPointer.h>

#include "fileconverter.hpp"
#include "cellobject.hpp"

class vtkActor;
struct OperationInfo;



class Cell3DExporter
{
public:
	using result_type = std::vector<std::string>;
	static OperationInfo info();
	static result_type collect(std::vector<result_type> *resultVec);
	Cell3DExporter(const ff::FORMAT3D &format, const QDir &dir,
				   const std::unordered_map<std::string, CellObject> &cellObjMap,
//				   const std::unordered_map<std::string, std::pair<vtkSmartPointer<vtkActor>, int>> &actorMap,
				   const std::vector<std::string> &cellNames, double factor, bool unify);


	void operator()(std::atomic_size_t *counter, std::atomic_bool *stopFlag,
					size_t threadNumber, size_t startIndex, size_t endIndex,
					result_type *thisThreadResult,
                    std::exception_ptr *ep, bool quiet);

private:
	const ff::FORMAT3D format3D_;
	const QDir &dir_;  // 保存するフォルダ
//	const std::unordered_map<std::string, std::pair<vtkSmartPointer<vtkActor>, int>> &actorMap_;
	const std::unordered_map<std::string, CellObject> &cellObjMap_;

	const std::vector<std::string> targetCellNames_;  // 表示中のセル名(ここに記載されているセルがvtkなどへexportされる)
	double exFactor_; //拡大縮小因子 1で拡大縮小なし
	bool unify_;

};

#endif // CELL3DEXPORTERC_HPP
