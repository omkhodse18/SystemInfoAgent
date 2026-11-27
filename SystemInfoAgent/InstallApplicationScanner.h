#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <unordered_map>
#include <vector>

#include "AppInfo.h"

namespace SystemInfoAgent::ApplicationReport {

    // Scans the standard Windows "Uninstall" registry locations -- HKLM native
    // view, HKLM WOW64-32 view, and every per-user hive under HKEY_USERS -- to
    // build a snapshot of installed software, mirroring what Control Panel's
    // "Programs and Features" shows.
    class InstalledAppScanner {
    public:
        static std::unordered_map<std::wstring, AppInfo> scanAll();

    private:
        static void scanUninstallKey(HKEY root, const std::wstring& subKey, const std::wstring& sourceLabel,
                                    std::unordered_map<std::wstring, AppInfo>& results);
        static std::vector<std::wstring> enumerateUserSids();
        static std::wstring normalizeInstallDate(const std::wstring& raw);
    };

} // namespace SystemInfoAgent::ApplicationReport
