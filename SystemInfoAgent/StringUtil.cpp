#include "StringUtil.h"

#include <algorithm>
#include <cwctype>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace SystemInfoAgent::core {

	// Trim leading/trailing whitespace (space, tab, \r, \n)
	std::wstring trim(const std::wstring& s) 
	{
		// Lambda func to check is current char is whitespace or not
		const auto isSpace = [](wchar_t c) { return std::iswspace(c) != 0; };
		
		auto begin = std::find_if_not(s.begin(), s.end(), isSpace);
		auto end = std::find_if_not(s.rbegin(), s.rend(), isSpace).base();

		if (begin >= end) { return L""; }

		return std::wstring(begin, end);
	}

	//	Convert to lowercase
	std::wstring toLower(const std::wstring& s) 
	{
		std::wstring result = s;
		std::transform(result.begin(), result.end(), result.begin(),
					[](wchar_t c) { return static_cast<wchar_t> (std::tolower(c)); } 
		);

		return result;
	}

#ifdef _WIN32

    std::string narrow(const std::wstring& wide) 
    {
        if (wide.empty()) { return {}; }

        int bytesNeeded = ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
        
        if (bytesNeeded <= 0) { return {}; }

        std::string result(static_cast<size_t>(bytesNeeded), '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), result.data(), bytesNeeded,
            nullptr, nullptr);
        return result;
    }

    std::wstring widen(const std::string& narrowStr) {
        if (narrowStr.empty()) {
            return {};
        }
        int charsNeeded =
            ::MultiByteToWideChar(CP_UTF8, 0, narrowStr.data(), static_cast<int>(narrowStr.size()), nullptr, 0);
        if (charsNeeded <= 0) {
            return {};
        }
        std::wstring result(static_cast<size_t>(charsNeeded), L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, narrowStr.data(), static_cast<int>(narrowStr.size()), result.data(),
            charsNeeded);
        return result;
    }

#else

    // Non-Windows fallback so this translation unit can still build/test on a
    // dev machine without the Win32 headers (e.g. for quick logic-only testing).
    std::string narrow(const std::wstring& wide) {
        return std::string(wide.begin(), wide.end());
    }

    std::wstring widen(const std::string& narrowStr) {
        return std::wstring(narrowStr.begin(), narrowStr.end());
    }

#endif

}	// namespace SystemInfoAgent::core