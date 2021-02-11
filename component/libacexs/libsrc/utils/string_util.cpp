
#include "string_util.hpp"

std::string utils::GetDataBlock(const int &num, const std::string& str)
{
	std::string::size_type spos = std::string::npos, epos = std::string::npos;
//	std::string data;
	for(int i = 0; i <= num; i++) {
		spos =str.find_first_not_of(" \t");
		if(spos == std::string::npos) return "";

		epos = str.find_first_of(" \t", spos);
	//	if(epos == std::string::npos) return "";
	//	data = str.substr(spos, epos - spos);
	}

	if(epos == std::string::npos) {
		return str.substr(spos);
	} else {
		return str.substr(spos, epos - spos);
	}
}

#if defined(_WIN32) || defined(_WIN64)
#include <vector>
#include <Windows.h>
#endif
std::string utils::toEncodedString(const std::string &sourceStr)
{
#if defined(_WIN32) || defined(_WIN64)
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

std::string utils::toUTF8(const std::string &sourceStr)
{
#if defined(_WIN32) || defined(_WIN64)
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
//    std::cout << "Conv utf result=" << std::endl;
//    for(size_t i = 0 ; i < u8Buff.size(); ++i) {
//        std::cout << u8Buff.at(i) << std::endl;
//    }
//    std::cout << "Done!" << std::endl;
    return std::string(u8Buff.begin(), u8Buff.end()-1);
#else
    return sourceStr;
#endif
}

