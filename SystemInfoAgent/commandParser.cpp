#include "commandParser.h"
#include "StringUtil.h"


namespace SystemInfoAgent::cli 
{
	using SystemInfoAgent::core::trim;
	using SystemInfoAgent::core::toLower;

	Command CommandParser::parse(int argc, wchar_t* argv[]) {
		//argv[0], is a programs own name
        if (argc <= 1) {
            return parseArgs({});   // no command line
        }

        const size_t count = static_cast<size_t>(argc);
        std::vector<std::wstring> args;
        args.reserve(count - 1);

        for (size_t i = 1; i < count; ++i) {
            args.emplace_back(argv[i]);
        }

        return parseArgs(args);
	}

    Command CommandParser::parseArgs(const std::vector<std::wstring>& args) 
    {
        Command cmd;
        cmd.rawArgs = args;

        if (args.size() == 0) {
            cmd.mode = Mode::Unknown;
            return cmd;
        }
        
        const std::wstring flag = toLower(args[0]);

        if (flag == L"/procinfo") 
        {
            cmd.mode = Mode::ProcInfo;
        }
        else if (flag == L"/kill" || flag == L"/block" || flag == L"/unblock") 
        {
            if (flag == L"/kill")
                cmd.mode = Mode::Kill;
            else if (flag == L"/block")
                cmd.mode = Mode::Block;
            else
                cmd.mode = Mode::Unblock;

            if (args.size() >= 2) {
                cmd.processNames = splitCommaList(args[1]);
            }
        }
        else if (flag == L"/wallpaper") 
        {
            cmd.mode = Mode::Wallpaper;
        }
        else if (flag == L"/startsrv" || flag == L"/stopsrv" || flag == L"/disablesrv") 
        {
            if (flag == L"/startsrv")
                cmd.mode = Mode::StartService;
            else if (flag == L"/stopsrv")
                cmd.mode = Mode::StopService;
            else
                cmd.mode = Mode::DisableService;

            if (args.size() >= 2) 
            {
                cmd.serviceName = trim(args[1]);
            }
        }
        else if (flag == L"/eipv6") 
        {
            cmd.mode = Mode::EnableIPv6;
        }
        else if (flag == L"/dipv6") 
        {
            cmd.mode = Mode::DisableIPv6;
        }
        else if (flag == L"/enablewinupd") 
        {
            cmd.mode = Mode::EnableWindowsUpdatePopups;
        }
        else if (flag == L"/disablewinupd") 
        {
            cmd.mode = Mode::DisableWindowsUpdatePopups;
        }
        else if (flag == L"/install-app-report") 
        {
            cmd.mode = Mode::InstallAppReport;
        }
        else {
            cmd.mode = Mode::Unknown;
        }

        return cmd;
    }

    std::vector<std::wstring> CommandParser::splitCommaList(const std::wstring& csv)
    {
        std::vector<std::wstring> result;

        size_t start = 0;
        while (start <= csv.size()) {
            size_t comma = csv.find(L',', start);
            std::wstring token = (comma == std::wstring::npos) ? csv.substr(start) : csv.substr(start, comma - start);

            token = trim(token);

            if (!token.empty()) { result.push_back(token); }
            if (comma == std::wstring::npos) { break; }

            start = comma + 1;
        }

        return result;
    }


}	// SystemInfoAgent::cli
