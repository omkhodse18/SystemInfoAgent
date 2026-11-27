#pragma once
#include <vector>
#include <unordered_map>

#include "AppInfo.h"


namespace SystemInfoAgent::ApplicationReport 
{
    struct AppDiffEntry 
    {
        AppInfo info;
        AppChangeStatus status;
    };

    // Pure, Win32-free comparison logic between two installed-software
    // snapshots. Kept separate from InstalledAppScanner so it can be unit
    // tested directly without touching the registry.
    class AppDiffTracker {
    public:
        static std::vector<AppDiffEntry> diff(const std::unordered_map<std::wstring, AppInfo>& previous,
                                              const std::unordered_map<std::wstring, AppInfo>& current);

        static bool hasChanges(const std::vector<AppDiffEntry>& entries);
    };

} // namespace sysops::swreport