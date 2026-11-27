#pragma once
#include <cstdint>
#include <string>

namespace SystemInfoAgent::ApplicationReport 
{
    struct AppInfo {
        std::wstring name;
        std::wstring publisher;
        std::wstring version;
        std::wstring installedDate;
        uint64_t sizeBytes = 0;
        std::wstring registrySource;
    };

    enum class AppChangeStatus {
        Unchanged,
        Added,
        Removed
    };

}