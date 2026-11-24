#pragma once

#include <string>
#include <vector>

namespace SystemInfoAgent::cli {

    enum class Mode {
        Unknown,

        // process info 
        ProcInfo,                   // /procinfo            -- enumerate & report running processes
        Kill,                       // /kill a.exe,b.exe
        Block,                      // /block a.exe,b.exe
        Unblock,                    // /unblock a.exe,b.exe
        Wallpaper,                  // /wallpaper
        StartService,               // /startsrv <name>
        StopService,                // /stopsrv <name>
        DisableService,             // /disablesrv <name>
        EnableIPv6,                 // /eipv6
        DisableIPv6,                // /dipv6
        EnableWindowsUpdatePopups,  // /enablewinupd
        DisableWindowsUpdatePopups, // /disablewinupd

        // Screenshot
        Screenshot,                 // /screenshot

        // App report
        InstallAppReport,           // /install-app-report  -- full installed-software scan/diff/report
    };

struct Command {
	Mode mode = Mode::Unknown;

    // Populated for Kill / Block / Unblock: the comma-separated process
    std::vector<std::wstring> processNames;     // /kill a.exe, b.exe.....; /block a.exe,.....
    
    //  // Populated for StartService / StopService / DisableService.
    std::wstring serviceName;                   //  store service name

    // Raw argv, kept for diagnostics/logging only.
    std::vector<std::wstring> rawArgs;
};

}   // namespace SystemInfoAgent::cli
