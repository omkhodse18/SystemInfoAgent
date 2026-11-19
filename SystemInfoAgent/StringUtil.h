#pragma once
#include <string>

namespace SystemInfoAgent::core {

	// Trim leading/trailing whitespace (space, tab, \r, \n).
	std::wstring trim(const std::wstring& s);

	// ASCII-range lowercase (flags are always ASCII, so this is sufficient and
	// avoids locale-sensitive behavior).
	std::wstring toLower(const std::wstring& s);

	// Narrow (UTF-16 -> UTF-8) / widen (UTF-8 -> UTF-16) conversions
	std::string narrow(const std::wstring& wide);
	std::wstring widen(const std::string& narrow);

}	// namespace SystemInfoAgent::core