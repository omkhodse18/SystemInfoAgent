#pragma once

#include <string>
#include <cstdint>

namespace SystemInfoAgent::procinfo {

    struct ProcessInfo {
        uint32_t pid = 0;
        std::wstring name;
        std::wstring path;
        std::wstring commandLine;
        uint64_t memoryUsageKb = 0;
        std::wstring dateCreated;
        std::wstring dateModified;
        std::wstring companyName;
        std::wstring fileDescription;
        std::wstring userName;
        bool isCritical = false;
    };

}   // namespace SystemInfoAgent::procinfo