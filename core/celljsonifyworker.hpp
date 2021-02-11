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
#ifndef CELLJSONIFYWORKER_HPP
#define CELLJSONIFYWORKER_HPP

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "core/utils/progress_utils.hpp"
#include "core/utils/workerinterface.hpp"
#include "core/geometry/cell/cell.hpp"
#include "component/picojson/picojson.h"

template<> struct WorkerTypeTraits<class CellJsonifyWorker> {
    // 返り値はタプルで
    // 1番目の要素がjsonString,
    // 2番めの要素が親セルの名前とjsonのunordered_map
    //tuple<vector<string>, unordered_map<string, pj::object>>
    typedef std::tuple<
        std::vector<std::string>,
        std::unordered_map<std::string, picojson::object>
    > result_type;
};


class CellJsonifyWorker: public WorkerInterface<CellJsonifyWorker>
{
public:
    typedef WorkerTypeTraits<CellJsonifyWorker>::result_type result_type;
    static OperationInfo info();
    static result_type collect(std::vector<result_type> *results);
    static constexpr char PARENT_KEY[7] = "parent";
    static constexpr bool prettify = true;
    CellJsonifyWorker(
        const std::vector<std::shared_ptr<const geom::Cell>> &targetCells,
        const std::unordered_map<std::string, int> & surfaceNameIndexMap,
        int divLevel, int extent
        )
        :targetCells_(targetCells), surfaceNameIndexMap_(surfaceNameIndexMap),
        divLevel_(divLevel), extent_(extent)
    {;}
    // i番目の処理
    void impl_operation(size_t i, int threadNumber, result_type* results)
    {
        (void) threadNumber;
        namespace pj = picojson;

        std::unordered_map<std::string, picojson::object> &parentObjs = std::get<1>(*results);
        const std::string &cellName = targetCells_.at(i)->cellName();
        pj::object obj = targetCells_.at(i)->jsonObject(surfaceNameIndexMap_, divLevel_, extent_);
        // 親属性はjsonではなくセル名から求める。
        std::vector<std::string> names = geom::Cell::getHierarchialCellNames(cellName);
        // 直接の親セル名(なければ空文字列)をjsonプロパティに追加
        obj.insert(std::make_pair(PARENT_KEY, (names.size() == 1) ? "" : names.at(1)));
        if(names.size() > 1) {
            // ここからparentオブジェクトを作る。
            // namesには階層セル名が直列に記録されている
            for(size_t j = 1; j < names.size(); ++j) {
                //mDebug() << "parent front===" << parentNames.front();
                // "<"でつないだmcnpでのセル名に戻す。
                std::string parentName = names.at(j);
                std::string grandParentName = (j == names.size()-1) ? "" : names.at(j+1);
                std::string childName = (j == 0) ? "" : names.at(j-1);

                // parentObjsに登録されていなければ追加する。登録済みなら登録済みのオブジェクトのchildrenを追加
                auto objIt = parentObjs.find(parentName);
                if(objIt == parentObjs.end()) {
                    // parentObjsへ単純に追加
                    pj::object parentObj;
                    // さらに祖先が有る場合
                    parentObj.insert(std::make_pair("parent", grandParentName));
                    parentObj.insert(std::make_pair("children", std::vector<pj::value>{pj::value(childName)}));
                    // parentオブジェクトは後でまとめて書き込むのでここではunordered_mapへ保存するだけ。
                    parentObjs.emplace(parentName, parentObj);
                } else {
                    // 登録済みのparentObjのchildrenエントリにcellNameを追加する。（エントリがなければ）
                    auto childrenIt = objIt->second.find("children");
                    assert(childrenIt != objIt->second.end());
                    pj::value childNameValue(childName);
                    pj::array &childArray = childrenIt->second.get<pj::array>();
                    if(std::find(childArray.begin(), childArray.end(), childNameValue) == childArray.end()) {
                        childArray.emplace_back(childName);
                    }
                }
            }
        }
        std::get<0>(*results).emplace_back(std::string("\"") + cellName + "\":" + pj::value(obj).serialize(prettify));
    }




private:
    const std::vector<std::shared_ptr<const geom::Cell>> &targetCells_;
    const std::unordered_map<std::string, int> &surfaceNameIndexMap_;
    int divLevel_;
    int extent_;
};
#endif // CELLJSONIFYWORKER_HPP
