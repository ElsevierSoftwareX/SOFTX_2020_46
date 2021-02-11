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
#include "system_utils.hpp"

#include <fstream>
#include <locale>
#include <map>
#include <regex>
#include <thread>
#include <vector>

#include "message.hpp"




// mac/linux共通
// linuxのみ
// macのみ
// windowsのみ
// の4通りを考える必要がある。

#if defined(__linux) || defined(__linux__)
// Linux固有
#include<sys/sysinfo.h>  // メモリ関係(linux)
#elif defined(__APPLE__)
// mac固有
#endif

#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
// Windows固有
#include <Windows.h>
#include <vector>  // windows固有ヘッダではないがwindows以外では使わない。
#else
// Linux/mac共通
#include <sys/param.h>  // パスの最高バイト数 (linux/mac)
#include <unistd.h>       // ファイルシステム関係(linux/mac)
#endif


std::string utils::utf8ToSystemEncoding(const std::string &sourceStr)
{
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
	std::string u8str = sourceStr;
	std::vector<wchar_t> uniBuff(MultiByteToWideChar(CP_UTF8, 0, u8str.c_str(),
													 static_cast<int>(u8str.size() + 1), NULL, 0));
	MultiByteToWideChar(CP_UTF8, 0, u8str.c_str(), static_cast<int>(u8str.size() + 1),
						uniBuff.data(), static_cast<int>(uniBuff.size()));
	std::vector<char> sjisBuff(WideCharToMultiByte(CP_THREAD_ACP, 0, uniBuff.data(), -1, NULL, 0, NULL, NULL));
	WideCharToMultiByte(CP_THREAD_ACP, 0, uniBuff.data(), static_cast<int>(uniBuff.size()),
						sjisBuff.data(), static_cast<int>(sjisBuff.size()), NULL, NULL);
	return std::string(sjisBuff.begin(), sjisBuff.end()-1);
#else
	return sourceStr;
#endif
}

std::string utils::systemEncodingToUtf8(const std::string &sourceStr)
{
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
	std::string sjisStr = sourceStr;
	//ShiftJISからUnicodeへ変換
	std::vector<wchar_t> uniBuff(MultiByteToWideChar(CP_THREAD_ACP, 0, sjisStr.c_str(),
													 static_cast<int>(sjisStr.size() + 1), NULL, 0));
	MultiByteToWideChar(CP_THREAD_ACP, 0, sjisStr.c_str(), sjisStr.size() + 1,
						uniBuff.data(), static_cast<int>(uniBuff.size()));
	//UnicodeからUTF8へ変換
	std::vector<char> u8Buff(WideCharToMultiByte(CP_UTF8, 0, uniBuff.data(), -1, NULL, 0, NULL, NULL));
	WideCharToMultiByte(CP_UTF8, 0, uniBuff.data(), static_cast<int>(uniBuff.size()),
						u8Buff.data(), static_cast<int>(u8Buff.size()), NULL, NULL);
	// .end()の一つ前はnullで終端されているので-1する
	return std::string(u8Buff.begin(), u8Buff.end()-1);
#else
	return sourceStr;
#endif
}

#include <iostream>
// 文字列が相対パスならtrue. linux環境ではwindows式絶対パスも絶対パスと認識する。
/*
 * 引数         win環境返り値     lin環境返り値
 * "c:/data"        false             false
 * "data"           true              true
 * /data            true              false
 *
 *
 */
bool utils::isRelativePath(const std::string &fileName)
{
	// 空白文字列は絶対パスでは無いのでtrueを返す。
	if(fileName.empty()) return true;
	static std::regex absPathPattern(R"(^[a-zA-Z]:)");
	bool isAbsWinPath = std::regex_search(fileName, absPathPattern);

#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
	return !isAbsWinPath;
#else

	//linux環境でもwindows式絶対パスを絶対パスと認識することにする。
	if(isAbsWinPath) {
		return false;
	} else {
		return fileName.front() != '/';
	}
#endif
}




// system依存したメモリ両検知関数

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

unsigned long getMacTotalMem()
{
//    xsw_usage vmusage;
//	size_t size = sizeof(vmusage);
//	if( sysctlbyname("vm.swapusage", &vmusage, &size, NULL, 0) != 0 ) {
//		mWarning() << "Initializing vmusage failed";
//		return 0;
//	} else {
//        // これは全部０しか返さないからダメ。
//		mDebug() << "virtual memory usage, total, avail, used===" << vmusage.xsu_total << vmusage.xsu_avail << vmusage.xsu_used;

    vm_size_t pagesize = 0;
    host_page_size(mach_host_self(), &pagesize);
    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vmstat;
    if(KERN_SUCCESS != host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count)) {
        mWarning() << "Initializing vmstat failed";
        return 0;
    } else {
        // 物理メモリサイズ
//        mDebug() << "free, active, inactive, wired===" << vmstat.free_count*pagesize << vmstat.active_count*pagesize
//                 << vmstat.inactive_count*pagesize << vmstat.wire_count*pagesize;
        auto totalmem = static_cast<unsigned long>(vmstat.free_count + vmstat.active_count + vmstat.inactive_count + vmstat.wire_count)
                * static_cast<unsigned long>(pagesize);
        return totalmem*9.5367E-7; // 9.5367E-7 = 1/(1024*1024)
    }

}
unsigned long getMacAvailMem()
{
    vm_size_t pagesize = 0;
    host_page_size(mach_host_self(), &pagesize);
    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vmstat;
    if(KERN_SUCCESS != host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count)) {
        mWarning() << "Initializing vmstat failed";
        return 0;
    } else {
        auto freemem = (static_cast<unsigned long>(vmstat.free_count) + static_cast<unsigned long>(vmstat.inactive_count))
                * static_cast<unsigned long>(pagesize);
        //auto freemem = static_cast<unsigned long>(vmstat.free_count) * static_cast<unsigned long>(pagesize);
        return freemem*9.5367E-7; // 9.5367E-7=1/(1024*1024)
    }

}
// top による取得。vm_statistics_data_tと比べてどうなのか忘れた。(unsigned long)vmstat.free_count * (unsigned long)vmusage.xsu_pagesize;
// top -l 1 | grep PhysMem: | awk '{print $6}' みたいな方法
#include <cstdio>
#include <regex>
unsigned long getMacFreeMem3()
{
    // megaに換算するときの因子
    static std::unordered_map<std::string, double> fMap{
        {"k", 1e-3}, {"K", 1e-3}, {"M", 1.0}, {"m", 1.0}, {"G", 1e+3}, {"g", 1e+3}
    };
    // top -l 1 | grep PhysMem: | awk '{print $6}' ベースの評価
    char filePath[] = "/tmp/asdfasdfsaXXXXXX";
    auto fd =mkstemp(filePath);
    if(fd == -1) {
        // error
        mWarning() << "mkstemp failed.";
        return 0;
    }
    std::string tmpFileName = std::string(filePath);
    std::string command = std::string("top -l 1 |grep PhysMem: > ") + tmpFileName;
    std::system(command.c_str());
    std::ifstream ifs(tmpFileName.c_str());
    if(ifs.fail()) {
        mWarning() << "Openning tmp file =" << tmpFileName << "failed.";
        return 0;
    }
    std::string buff;
    std::getline(ifs, buff);
    ifs.close();
    std::remove(tmpFileName.c_str());
    static const std::regex memRegex(R"(, ([0-9]+)([kKmMgG]) unused)");
    std::smatch sm;
    if(std::regex_search(buff, sm, memRegex)) {
        return static_cast<unsigned long>(std::stoul(sm.str(1))*fMap.at(sm.str(2)));
    } else {
        mDebug() << "no mem data found in str===" << buff;
        return 0;
    }
}

#elif defined(__linux) || defined(__linux__)

// sysinfoを利用したメモリ使用量検知関数。
// sysinfoを使っているのでcachedな分が入っておらず過小評価になる。
unsigned long getLinuxAvailMemMB1()
{
	static struct sysinfo memInfo;
	sysinfo (&memInfo);
	return (memInfo.freeram + memInfo.bufferram)/1048576; // divide by 1024*1024 = 1048576
}

unsigned long getLinuxAvailMemMB2()
{
	unsigned long freemem;
	// sysinfoではcachedメモリの量を検知できないので/proc/meminfoを使う
	// MBへ換算するためのfactor
	static const std::unordered_map<std::string, double> toMBFactors {
		{"B", 1.0/1024*1024}, {"kB", 1.0/1024}, {"MB", 1}, {"GB", 1e+3}
	};
	std::ifstream memIfs("/proc/meminfo");
	if(memIfs.fail()) {
	// /proc/meminfoがアクセス不能ならsysinfoを使う。
		mWarning() << "/proc/meminfo is not available. Sysinfo is used instead.";
		return getLinuxAvailMemMB1();
	}

	std::string tmpStr, unitStr;
	while(memIfs >> tmpStr) {
		if(tmpStr == "MemAvailable:") {
			if(memIfs >> freemem && memIfs >> unitStr) {
				try {
					return freemem*toMBFactors.at(unitStr);
				} catch (...) {
					mWarning() << "no unit string found in /proc/meminfo, Sysinfo is used instead.";
					return getLinuxAvailMemMB1();
				}
			} else {
				mWarning() << "no \"MemTotal:\" entry. getAvailMemMB failed. Sysinfo is used instead.";
				return getLinuxAvailMemMB1();
			}
		}
		// ignore rest of the line
		memIfs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	mWarning() << "No valid entry for mem available found. Sysinfo is used instead.";
	return getLinuxAvailMemMB1();;
}

#endif

unsigned long utils::getAvailMemMB(){
	unsigned long freemem = 0;
#if defined(__linux) || defined(__linux__)
	freemem = getLinuxAvailMemMB2();

#elif  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);
	GlobalMemoryStatusEx(&statex);
	freemem = static_cast<unsigned long>(statex.ullAvailPhys/1048576);
#else
    //freemem = getMacFreeMem3();
    freemem = getMacAvailMem();
#endif

	// mingw32のように64bitOS上で32ビットアプリを走らす場合2GBが上限となる
#if defined(__i386__)
	return (freemem > 2048) ? 2048 : freemem;
#else
	return freemem;
#endif
}

unsigned long utils::getTotalMemMB()
{
	unsigned long totalmem = 0;
#if defined(__linux) || defined(__linux__)
	static struct sysinfo memInfo;
	sysinfo (&memInfo);
	totalmem = memInfo.totalram/1048576; // divide by 1024*1024 = 1048576
#elif  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);
	GlobalMemoryStatusEx(&statex);
	totalmem = static_cast<unsigned long>(statex.ullTotalPhys/1048576);
#else
	totalmem = getMacTotalMem();
#endif


#if defined(__i386__)
	return (totalmem > 2048) ? 2048 : totalram;
#else
	return totalmem;
#endif

}


size_t utils::guessNumThreads(int n)
{
	if(n == 0) {
		return 0;
	} else if(n > 0) {
		return static_cast<size_t>(n);
	} else {
		auto guess = std::thread::hardware_concurrency();
		if(guess <= 0) {
			return 1;
		} else {
			return guess;
		}
	}
}

// CRLFファイルをLinuxなどでLF改行システムでgetlineすると末尾にCRが残るので削除する
void utils::sanitizeCR(std::string *str)
{
    if(str->empty()) return;
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
    return;
#else
    //str->pop_back();
    if(str->back() == '\r') str->pop_back();
	return;
#endif
}

bool utils::exists(const std::string &fileName)
{
	// 文字コードを考慮する
	std::ifstream ifs(utf8ToSystemEncoding(fileName));
	return ifs.is_open();
}




// windows下でのcodepage変換
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
// CodePage のマルチバイト文字列へ変換
template<int CodePage>
std::string toMultiString(const LPWSTR wstr, DWORD size)
{
    std::vector<char> mbuff(WideCharToMultiByte(CodePage, 0, wstr, -1, NULL, 0, NULL, NULL));
    WideCharToMultiByte(CodePage, 0, wstr, size, mbuff.data(), mbuff.size(), NULL, NULL);
    return std::string(mbuff.begin(), mbuff.end()-1);
}
// wstringからu8encodingのstringへ
std::string WstoU8s(const std::wstring &wstr)
{
    return toMultiString<CP_UTF8>((const LPWSTR)wstr.data(), wstr.size());
}
#endif

#ifdef ENABLE_GUI
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
#include <cstdlib>
#include <clocale>
namespace {



/*
 * faceNameをキーに、フォントファイル名を値にしたマップを作る
 *
 * ・QFont::family().toStdString()はUTF8
 * ・一方レジストリを参照して得られるキー、値は当然sjis
 * ・vtkTextProperty::SetFontFile(char*)はどちらのエンコードか不明。あんまり何も変換してなさそうだから多分SJIS
 *
 * ここでは key, valueともにUTF8とする。
 */

std::map<std::string, std::string> makeFontMap()
{
    std::map<std::string, std::string> fmap;
    static const LPCWSTR fontRegistryPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";

    HKEY hKey;
    LONG result;
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, fontRegistryPath, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) throw std::invalid_argument("Registry open faled");

    DWORD maxValueNameSize, maxValueDataSize;
    result = RegQueryInfoKey(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
    if (result != ERROR_SUCCESS) throw std::invalid_argument("Registry key query failed");

    DWORD valueIndex = 0;
    LPWSTR valueName = new WCHAR[maxValueNameSize];
    LPBYTE valueData = new BYTE[maxValueDataSize];
    DWORD valueNameSize, valueDataSize, valueType;
    /*
     * NOTE mingwではstd::setlocale(LC_ALL, "");してからstd::regex reg2("^ [cC]");すると死ぬ
     */

    do {
        valueDataSize = maxValueDataSize;
        valueNameSize = maxValueNameSize;

        // 第七引数はLPDWORD型だが、これはDWORD*なのでDWORD変数の＆を取っている。
        // unicode定義済み環境なので実態はRegEnumValueWでvalueDataはunicode(UTF16LE=Wide char string)
        result = RegEnumValue(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);
        valueIndex++;
        if (result != ERROR_SUCCESS || valueType != REG_SZ) continue;

        // wsFontNameはレジストリのエントリ（faceName). wchar_t* と文字数をコンストラクタに与えて構築
        std::wstring wsFontName(valueName, valueNameSize);
        /*
         * wsValueNameをいわゆるfamilyにするために
         * １．複数フォント名の場合のアンド&で分割して複数のエントリーとする。
         * 末尾の(truetype)とかビットマップフォントのポイント数とかは無視する。
         * 比較時にフォントファミリー文字列の文字数分しか比較しないので。
         */
        std::vector<std::wstring> wsFontNameVec;
        std::wstring::size_type prevAnd = 0, nextAnd = wsFontName.find_first_of(L"&");
        if(nextAnd != std::wstring::npos) {
            while(nextAnd != std::wstring::npos) {
                wsFontNameVec.emplace_back(wsFontName.substr(prevAnd, nextAnd-prevAnd));
                prevAnd = wsFontName.find_first_not_of(L" ", nextAnd+1);  // nextAnd(&文字)+1(の次)から非空白まで
                nextAnd = wsFontName.find_first_of(L"&", prevAnd);
            }
            wsFontNameVec.emplace_back(wsFontName.substr(prevAnd));
        } else {
            wsFontNameVec.emplace_back(wsFontName);
        }
        /*
         * valueDataは     LPBYTE　→ unsigned char*
         * valueDataSizeは DWORD   → unsigned int
         *                 LPWSTR  → WCHAR*=wchar_t*
         *
         * ここでvalueDataはvalueDataSizeバイトの配列(先頭へのポインタ)でencodingは当然sjis
         * 次にvalueDataSizeバイトのunsigned char*をwchar_t*へ変換する。
         * つまりunsigned char*(マルチバイトsjis) →　wchar_t*(utf16)　→ string(utf8) へ変換している
         */

        std::string fontFile = toMultiString<CP_UTF8>((LPWSTR)valueData, valueDataSize);
        //mDebug() << "fontfile===" << fontFile;

        for(const auto& wfname: wsFontNameVec) {
            //std::wcout << "fontnames ===" << fname << std::endl;// << ", filename =" << wsFontFile << std::endl;
            //std::string u8fname = wsToU8String(wfname);
            std::string fname = toMultiString<CP_UTF8>((const LPWSTR)wfname.data(), wfname.size());
            //mDebug() << "font name ===" << fname << ", file===" << fontFile;
            fmap.emplace(fname, fontFile);
        }
    } while (result != ERROR_NO_MORE_ITEMS);

    delete[] valueName;
    delete[] valueData;
    RegCloseKey(hKey);

//    int n = 0;
//    for(const auto &fpair : fmap) {
//        //std::wcout.imbue(std::locale(""));
//        mDebug() << "n = " << n++ << ", name === " << fpair.first << ", file === " << fpair.second;
//    }
//    mDebug() << "num fonts === " << fmap.size();

    return fmap;
}
// フォントファミリーをキーに、ファイル名を値にしたマップを返す。encodingはutf8
const std::map<std::string, std::string> &getFontMap()
{
    static const std::map<std::string, std::string> fmap = makeFontMap();
    return fmap;
}

// QFont::weightを文字列化する
std::string weightString(int w)
{
    static const std::map<int, std::string> weightMap {
        {QFont::Thin, "Thin"},
        {QFont::ExtraLight, "ExtraLight"},
        {QFont::Light,"Light"},
        {QFont::Normal,""},
        {QFont::Medium,"Medium"},
        {QFont::DemiBold,"DemiBold"},
        {QFont::Bold,"Bold"},
        {QFont::ExtraBold,"ExtraBold"},
        {QFont::Black, "Black"}
    };

    if(weightMap.find(w) == weightMap.end()) {
        return "";
    } else {
        return weightMap.at(w);
    }
}

std::string searchFontMap(const std::string &faceName)
{
//    auto fontMap = getFontMap();
//    for(const auto &strPair: fontMap) {
    for(const auto &strPair: getFontMap()) {
        // strPair.firstがファミリー(utf8 string), secondがファイル名(utf8, string)
        if (_strnicmp(faceName.c_str(), strPair.first.c_str(), faceName.length()) == 0) {
            // 該当フォント発見
            // Build full font file path
            WCHAR winDir[MAX_PATH];
            GetWindowsDirectory(winDir, MAX_PATH);

            std::wstringstream ss;
            ss << winDir << "\\Fonts";
            std::wstring wpath = ss.str();
            std::string fontDir = WstoU8s(wpath);
            return fontDir + "\\" + strPair.second;
        }
    }
    //mWarning() << "Font " << font.family() << " not found";
    return "";
}

}  // end anonymous namespace

// windows環境でfontファイルパスを取得する。
// https://stackoverflow.com/questions/11387564/get-a-font-filepath-from-name-and-style-in-c-windows
#include <windows.h>
std::string getFontFilePath(const QFont &font)
{
    /*
     * Windowsフォント選択での問題点
     * ・レジストリのキー名とダイアログでのフォント名が異なる（游明朝とYu Minchoなど)
     *・ウェイト値でなくウェイト文字列でウェイトが表現されているがフォント間で一貫性がなく、
     *  QFont::weightで数値化されてしまうと元のスタイル名(フォント名の一部になる)が復元できない。(SemiLightっていくつよ）
     * ・ウェイトなしの場合のフォント名にスタイル文字列がついている場合となにもない場合の２通りある
     * ・ウェイトなしの場合の文字列がRegularとNormalと２通りある
     * ・ttcフォントには複数の書体が含まれるがvtkではそのうち１つしか扱えない。
     */
    /*
     * レジストリキーとフォントダイアログ名が異なるフォントがいくつかあるので、そのあたりは個別に対応。
     *　游明朝、游ゴシック、メイリオ、MSゴシック・明朝、あたり。
     * 一方、源とかHG創英とかはレジストリに日本語キー名で登録されているものもある。そういうのは対処不要で助かる。
     */
    static const std::map<std::string, std::regex> faceConvMap {
        {"Yu Mincho", std::regex("游明朝")},
        {"Yu Gothic", std::regex("游ゴシック")},
        {"Meiryo", std::regex("メイリオ")},
        {"MS Gothic", std::regex("ＭＳ ゴシック")},
        {"MS PGothic", std::regex("ＭＳ Ｐゴシック")},
        {"MS Mincho", std::regex("ＭＳ 明朝")},
        {"MS PMincho", std::regex("ＭＳ Ｐ明朝")},

    };
    // Yu Gothicは　レジストリではYu Gothic Mediumなので、Yu Gothicで検索すると最初に出てくる Yu Gothic Boldがヒットしてしまう
    // "游明朝　Light"　の　"游明朝"だけを検索して置換する必要がある。
    std::string faceName = font.family().toStdString();
    std::smatch sm;
    for(const auto& spair: faceConvMap) {
        if(std::regex_search(faceName, sm, spair.second)) {
            faceName = sm.prefix().str() + spair.first + sm.suffix().str();
            break;
        }
    }

    std::string wstr = weightString(font.weight()).empty() ? "" : " " + weightString(font.weight());
    std::string istr = font.italic() ? " Italic" : "";
    // weight→italicの順番にする。
     // meiryo midiumは　名前にmidiumが入ってかつfont.weightもmidiumなので重複するが気にしない。検索一回分無駄になるだけ

    /*
     * 一応はwindows環境でもitalic・boldが適用できるようにした
     * ただし、一部のフォントはsemilight見たいな指定があって
     * QFONTではこれはLight扱いのためフォント名が一致せず、選択できない。
     * 仕方ないね。
     *
     */

    // font名としてただしそうな候補(複数)を検索する。より詳しい名前をvectorの前の方に置く。
    std::vector<std::string> fontNames;
    if(wstr.empty()) {
        fontNames.push_back(faceName + " Medium" + istr);
        fontNames.push_back(faceName + " Regular" + istr);
    }

    fontNames.push_back(faceName + wstr + istr);
    fontNames.push_back(faceName);
    //mDebug() << "Searching names===" << fontNames;
    std::string result;
    for(const auto &name: fontNames) {
        result = searchFontMap(name);
        if(!result.empty()) return result;
    }

    mWarning() << "Font = " << fontNames << "not found.";
    return "";
}

#else
// ここから非windows
#include <fontconfig/fontconfig.h>
// familyをキーにした タプル(slant, weight, fontfile名)のマルチマップ
std::multimap<std::string, std::tuple<int, int, std::string>> makeFontMap()
{
	// この辺参照
	// https://stackoverflow.com/questions/10542832/how-to-use-fontconfig-to-get-font-list-c-c/14634033
	std::multimap<std::string, std::tuple<int, int, std::string>> fontmap;
	FcConfig* config = FcInitLoadConfigAndFonts();
	FcPattern* pat = FcPatternCreate();
	//FcObjectSet* os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, (char *) 0);
	FcObjectSet* os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, FC_SLANT, FC_WEIGHT, (char *) 0);
	FcFontSet* fs = FcFontList(config, pat, os);
	//printf("Total matching fonts: %d\n", fs->nfont);
	for (int i=0; fs && i < fs->nfont; ++i) {
		FcPattern* font = fs->fonts[i];
		// FcChar8 は要するに8bit unsigned char
		// char* から std::string へはコンストラクタに与えれば変換可能。
		FcChar8 *file, *style, *family;
		int slant, weight;
		if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
			FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch &&
			FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch &&
			FcPatternGetInteger(font, FC_SLANT, 0, &slant) == FcResultMatch &&
			FcPatternGetInteger(font, FC_WEIGHT, 0, &weight) == FcResultMatch)
		{
			//printf("Filename: %s (family %s, style %s, slant %d, weight %d)\n", file, family, style, slant, weight);
			//mDebug() << "making font file list, file===" << std::string(reinterpret_cast<char*>(file)) << "added to mup";
			fontmap.emplace(std::string(reinterpret_cast<char*>(family)),
							std::make_tuple(slant, weight, std::string(reinterpret_cast<char*>(file))));
		}
	}
	if (fs) FcFontSetDestroy(fs);
	return fontmap;
}

// フォントのファミリー名をキーにした、slant, weight, ファイル名のタプルを保存するマルチマップを返す
const std::multimap<std::string, std::tuple<int, int, std::string>> &getFontFileMap()
{
	static const std::multimap<std::string, std::tuple<int, int, std::string>> fontmap = makeFontMap();
	return fontmap;
}
std::string getFontFilePath(const QFont &font)
{
	try {
		/*
		 * QFont::family()の返すファミリー名は実際にはfc-matchにかけないと正準なファミリー名にならない。
		 * 例えばLinuxではQFont().family()は"Sans"を返すがSansの実体をfc-match Sansで調べるとVLゴシックと出る。
		 * fontconfigのフォントリストはVLゴシックで調べる必要がある。
		 */
		//mDebug() << "fontデータ==" << font.toString() << "に対応するファイルを探索";
		std::string canonicalFamilyName = font.family().toStdString();
		// マッチング処理
		FcPattern *pat;
		FcFontSet *fs;
		FcResult result;

		pat = FcNameParse ((FcChar8 *) font.family().toStdString().c_str());
		if (!pat){
			mWarning() << "Unable to parse the pattern =" << font.family();
		}

		FcConfigSubstitute(0, pat, FcMatchPattern);
		FcDefaultSubstitute(pat);
		fs = FcFontSetCreate ();

		FcPattern   *match;
		match = FcFontMatch (0, pat, &result);
		if (match) FcFontSetAdd (fs, match);

		FcChar8 *family;
		if (FcPatternGetString(match, FC_FAMILY, 0, &family) == FcResultMatch)
		{
			//printf("HIT!!!QFont's family=%s equal to %s \n",font.family().toStdString().c_str(), family);
			canonicalFamilyName = std::string(reinterpret_cast<char*>(family));
		}
		FcPatternDestroy (pat);
		if(fs) FcFontSetDestroy(fs);

		// slant値は適当。だいたいobliqueは110を、italicは100になっている。例外も多数あるが割とどうでもいい。
		int slant = 0;
		if(font.style() == QFont::Style::StyleItalic) {
			slant = 100;
		} else if (font.style() == QFont::Style::StyleOblique) {
			slant = 110;
		}
		int weight = 2.41*font.weight(); // QFont::Black=87であるのに対し、fontconfigでのblackは210なので.
		std::tuple<int, int, std::string> tp;
		auto range = getFontFileMap().equal_range(canonicalFamilyName);
		for(auto it = range.first; it != range.second; ++it) {
			//mDebug() << "key===" << it->first << "file===" << std::get<2>(it->second);
			// slant値が近い方を選ぶ。slantが同じならばweightが近い方を選ぶ。
			if(std::get<2>(tp).empty()) {
				tp = it->second;
			// slant値が近いか、slant値が等しくかつweightが近けれ候補を更新
			} else if((std::abs(std::get<0>(it->second) - slant) < std::abs(std::get<0>(tp) - slant))
				||(std::get<0>(it->second) ==  std::get<0>(tp)
				   && (std::abs(std::get<1>(it->second) - weight) < std::abs(std::get<1>(tp) - weight)))) {
				tp = it->second;
			}
		}
		std::string fontFile = std::get<2>(tp);
		//mDebug() << "Equivalent Font of " << font.toString() << " is " << fontFile;
		return fontFile;
	} catch (std::out_of_range &oor) {
		mWarning() << "Font file for = " << font.family() << " not found.";
		return "";
	}
}
#endif  // ここまで非windows

// fontからファイル名を取得する。↓の関数は非OS依存のインターフェイスだけ。
std::string utils::fontFilePath(const QFont &font)
{
    std::string filepath = getFontFilePath(font);
//    mDebug() << "Family===" << font.family() <<"point=" << font.pointSize()
//             << "bold=" << font.bold() << "italic=" << font.italic() << "file=" << filepath;
    return filepath;
}

#endif  // ここまでifdef ENABLE_GUI


std::string utils::getCurrentDirectory()
{
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
	TCHAR dir[MAX_PATH + 1];
    GetCurrentDirectory(MAX_PATH + 1, dir);
    // TCHARは最近のwindowsではもうwchar_tとして良いはず
    std::wstring wstr(dir);
    return  WstoU8s(wstr);  // system encodingからu8に変換して返す。
#else
    // 非windows == unix環境
    // get_current_dir_nameはGNU拡張なのでmacでは使えない。
    //return std::string(get_current_dir_name());
    char dname[static_cast<size_t>(MAXPATHLEN)];
    getcwd(&dname[0], static_cast<size_t>(MAXPATHLEN));
    return std::string(dname);
#endif
}


// windows, dosでの予約済みファイル名を判定する。
// 安全と単純のためcon1.txtみたいなものもtrue判定する。
bool utils::isReservedFileName(const std::string &fileName)
{
	bool retval = false;
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
	// 行頭から拡張子/行末までの間が予約済みファイル名なら修正する。
	// このルールはwindowsのみ
	static std::vector<std::regex> regVec{
		std::regex("^(aux)([0-9]{0,1})(\\.*|$)", std::regex_constants::icase),
		std::regex("^(com)([0-9]{0,1})(\\.*|$)", std::regex_constants::icase),
		std::regex("^(con)([0-9]{0,1})(\\.*|$)", std::regex_constants::icase),
		std::regex("^(lpt)([0-9]{0,1})(\\.*|$)", std::regex_constants::icase),
		std::regex("^(nul)([0-9]{0,1})(\\.*|$)", std::regex_constants::icase),
		std::regex("^(prn)([0-9]{0,1})(\\.*|$)", std::regex_constants::icase)
	};
	std::smatch sm;

	for(const auto& reg: regVec) {
		if(std::regex_search(fileName, sm, reg)) {
			return true;
		}
	}
#else
	(void) fileName;
#endif
	return retval;
}


// ファイルシステム的に妥当なファイル名にを取得する。
// 引数にはフルパスではなく個別のファイル名をとる。
std::string utils::toValidEachFileName(const std::string &fileName, bool quiet)
{
	std::string retStr = fileName;
	// latticeで使っているセルには　<>][が含まれているのでそのままファイル名に使ってはだめ。
	// <はwindowsでは使えないし、linux等でもエスケープしないとだめなので面倒。
	// なのでここで置換する。
	// とりあえずlinuxのシェルでエスケープしなくても良い文字に置き換えるべし。
    // 結局使える記号は+-.@くらいしかない
    //
    // あとtetra latticeの場合/が入り得るのでこれも置換する。
	//
	static std::unordered_map<char, char> convMap {
        {',', '.'},
        {'[', '_'},
        {']', '_'},
		{'<', '-'},
        {'>', '-'},
        {'/', '@'}
	};

	for(const auto& convPair: convMap) {
		if(retStr.find(convPair.first) != std::string::npos) {
			std::replace(retStr.begin(), retStr.end(), convPair.first, convPair.second);
		}
	}
	if(!quiet && retStr != fileName) {
		mWarning() << "File name has invalid character(s), changed from"
				   << "\"" + fileName + "\"" << " to "  << "\"" + retStr + "\"";
	}


	// 予約済みファイル名は頭に'x'を付けて修正する。
	if(isReservedFileName(retStr)) {
		auto prev = retStr;
		retStr = "x" + retStr;
		if(!quiet) {
			mWarning() << "File name is reserved by your operation system. changed from "
					   << "\"" + prev + "\" to"  << "\"" + retStr + "\"";
		}
	}
	return retStr;
}

// セキュリティ的にOKなフルパスかチェックする。
// export時にはセル名をファイル名に使うので
// 不当なセル名がリスク発生させるのは良くない。





// ファイルパスからファイル名を取り出す。
std::string utils::getFileName(const std::string &filePath)
{
// FIXME Windowsでも /は使えるので要チェック
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
    const std::string SEPS = "\\";
#else
    const std::string SEPS = "/";
#endif
    std::string::size_type pos = filePath.find_last_of(SEPS);
    if(pos == std::string::npos) {
        return filePath;
    } else {
        return filePath.substr(pos + 1);
    }
}

std::string utils::getDirectoryName(const std::string &filePath)
{
#if  defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WIN64__) || defined(_MSC_VER)
    const std::string SEPS = "\\";
#else
    const std::string SEPS = "/";
#endif
    std::string::size_type pos = filePath.find_last_of(SEPS);
    if(pos == std::string::npos) {
        return ".";
    } else {
        return filePath.substr(0, pos);
    }
}
