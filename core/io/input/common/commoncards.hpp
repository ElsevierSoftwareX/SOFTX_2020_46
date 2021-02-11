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
#ifndef COMMON_METACARDS_HPP
#define COMMON_METACARDS_HPP

#include <regex>
#include <string>

namespace inp {
namespace comm {

// 行継続
const std::regex &getPreContinuePattern();
const std::regex &getMcnpPostContinuePattern();
const std::regex &getPhitsPostContinuePattern();
// コメント
const std::regex &getPreCommentPattern();
const std::regex &getPostCommentPattern();
std::string preUncommentedString(const std::string &str);

const std::regex &iPattern();
const std::regex &jPattern();
const std::regex &mPattern();
const std::regex &rPattern();

// mcnp後置コメント($)を処理する。コメントを削除した場合はtrue,それ以外はfalse
bool removeMcnpPostComment(std::string *str);
// ＃以外のphits後置コメント($!%)を処理する。コメントを削除した場合はtrue,それ以外はfalse
bool removePhitsPostCommentNotSharp(std::string *str);
// #でのコメントアウト部分を削除
bool removePhitsPostCommentSharp(std::string *str);
// mcnp/phits共通前置コメント(行頭5文字以内にc+空白)を削除する。拡張入力 c *は削除せず、真にコメントだけを削除する。
//bool removeTruePreComment(std::string *str);

// m1 やMat[1]から番号文字列1を返す。
std::string GetMaterialIdStr(const std::string& str);

}
}

#endif // COMMON_METACARDS_HPP
