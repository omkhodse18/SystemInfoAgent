#pragma once

#include "Command.h"

namespace SystemInfoAgent::cli {

    class CommandParser {
    private:
        static std::vector<std::wstring> splitCommaList(const std::wstring& csv);
 
    public:
        // argc/argv as received by wmain(). Returns a Command with
        // Mode::Unknown if nothing recognizable was supplied.
        static Command parse(int argc, wchar_t* argv[]);

        // Testable overload: same logic, takes already-tokenized args
        // (argv[0], the exe name, is NOT included here).
        static Command parseArgs(const std::vector<std::wstring>& args);

    };


}   // namespace SystemInfoAgent::cli