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
#include "celljsonifyworker.hpp"


OperationInfo CellJsonifyWorker::info() {
#ifdef ENABLE_GUI
    std::string title = QObject::tr("Progress").toStdString();
    std::string operatingText = QObject::tr("Creating cell CSG data ").toStdString();
    std::string cancelingText = QObject::tr("Waiting for the subthreads to finish ").toStdString();
    std::string cbuttonLabel = QObject::tr("Cancel").toStdString();
#else
    std::string title = "Progress";
    std::string operatingText = "Creating cell CSG data ";
    std::string cancelingText ="Waiting for the subthreads to finish ";
    std::string cbuttonLabel = "";
#endif
    return OperationInfo(title, operatingText, cancelingText, cbuttonLabel);
}
//constexpr char CellJsonifyWorker::PARENT_KEY[];
CellJsonifyWorker::result_type
CellJsonifyWorker::collect(std::vector<CellJsonifyWorker::result_type> *results)
{
    namespace pj = picojson;
    std::size_t retVecSize = 0;
    for(const auto& res: *results) {
        retVecSize += std::get<0>(res).size();
    }
    // 返り値はタプルで
    // 1番目の要素がjsonString,
    // 2番めの要素が親セルの名前とjsonのunordered_map
    //tuple<vector<string>, unordered_map<string, pj::object>>
    CellJsonifyWorker::result_type retTuple;
    std::vector<std::string> &retVec = std::get<0>(retTuple);
    std::unordered_map<std::string, picojson::object> &retMap = std::get<1>(retTuple);
    retVec.reserve(retVecSize);
    for(auto &eachResult: *results) {
        // タプルのvectorマージ
        if(!std::get<0>(eachResult).empty()) {
            retVec.insert(retVec.end(),
                std::make_move_iterator(std::get<0>(eachResult).begin()),
                std::make_move_iterator(std::get<0>(eachResult).end()));
        }
        // タプルのhashmapマージ.このマップは親セルのエントリなので、
        // 同一キーがあった場合、上書きではなく、childrenプロパティをマージする。
        for(auto &srcPair: std::get<1>(eachResult)) {
            auto targetIt = retMap.find(srcPair.first);
            if(targetIt == retMap.end()) {
                retMap.insert(srcPair);
            } else {
                // 追加されるjsonobjectのchildrenプロパティのvalue(pj::array)を取得する。
                auto targetChildrenIt = targetIt->second.find("children");  // it->secondはpj::object
                assert(targetChildrenIt != targetIt->second.end());
                pj::array &targetChildArray = targetChildrenIt->second.get<pj::array>();
                // 追加する方のchildrenプロパティを取得
                auto srcChildrenIt = srcPair.second.find("children");
                assert(srcChildrenIt != srcPair.second.end());
                pj::array &srChildArray = srcChildrenIt->second.get<pj::array>();
                // マージ
                for(const auto &val: srChildArray) {
                    if(std::find(targetChildArray.begin(), targetChildArray.end(), val) == targetChildArray.end()) {
                        targetChildArray.emplace_back(val);
                    }
                }

//                if(std::find(childArray.begin(), childArray.end(), childNameValue) == childArray.end()) {
//                    childArray.emplace_back(childName);
//                }
            }

        }
    }
    return retTuple;
}
