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
#ifndef SYSTEM_UTILS_HPP
#define SYSTEM_UTILS_HPP

#include <cassert>
#include <chrono>
#include <string>
#include <vector>

// ここはC++17で<filesystem>が入るまで我慢するしかない
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
static constexpr char PATH_SEP = '\\';

#else
static constexpr char PATH_SEP = '/';
#endif


#ifdef ENABLE_GUI
#include <QFont>
#endif

namespace utils{

// このアプリケーションではutf-8をデフォルトとするのでwindowsではSJISに変換する
std::string utf8ToSystemEncoding(const std::string &sourceStr);
// ↑の逆でシステムの文字コードからUTF8へ変換
std::string systemEncodingToUtf8(const std::string &sourceStr);
// 引数が絶対パスか相対パスか環境に応じて適当に判定する。
bool isRelativePath(const std::string &fileName);
// CRLF改行ファイルでgetlineすると末尾に\rが入るので削除
void sanitizeCR(std::string *str);

// ファイルの存在確認
bool exists(const std::string & fileName);

// カレントディレクトリ文字列の取得
std::string getCurrentDirectory();

//! 割当可能なメモリ量をMB単位で返す
unsigned long getAvailMemMB();
//! 物理メモリ量をMB単位で返す
unsigned long getTotalMemMB();

//! スレッド数を適当に設定する。
size_t guessNumThreads(int n);

// QFontからfontのファイルパスを取得する。
#ifdef ENABLE_GUI
std::string fontFilePath(const QFont &font);
#endif


//（フルパスではなく個別の）ファイル名を予約語などで禁止されているか判定
bool isReservedFileName(const std::string &fileName);
//（フルパスではなく個別の）ファイル名を予約語などで禁止されていないファイル名に変換
std::string toValidEachFileName(const std::string &fileName, bool quiet = false);
// ファイルパスからファイル名を取得する。
std::string getFileName(const std::string &filePath);
// ファイルパスからディレクトリ名を取得する。空なら"."を返す。末尾に区切り文字は付けない
std::string getDirectoryName(const std::string &filePath);
}// end namespace util

#endif // SYSTEM_UTILS_HPP
