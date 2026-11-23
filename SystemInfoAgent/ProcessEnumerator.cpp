#include <windows.h>      
#include <Psapi.h>
#include <TlHelp32.h>
#include <comdef.h>
#include <sddl.h>
#include <wbemidl.h>

#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include "ProcessEnumerator.h"
#include "WinHandle.h"

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "version.lib")

namespace SystemInfoAgent::procinfo {
	using namespace SystemInfoAgent::core;

	namespace {
        std::wstring toUpper(std::wstring value) {
            std::transform(value.begin(), value.end(), value.begin(),
                        [](wchar_t c) { return static_cast<wchar_t>(::towupper(c)); }
            );
            
            return value;
        }   // toUpper

        // Queries WMI's Win32_Process class for each process's command line, since
        // the Toolhelp32 snapshot API does not expose it. Keyed by PID.
        std::unordered_map<DWORD, std::wstring> queryCommandLinesViaWmi() 
        {
            std::unordered_map<DWORD, std::wstring> result;

            IWbemLocator* rawLocator = nullptr;

            HRESULT hr = ::CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID*>(&rawLocator));
            
            if (FAILED(hr) || rawLocator == nullptr) { return result; }

            _COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
            IWbemLocatorPtr locator(rawLocator, false);

            IWbemServices* rawServices = nullptr;
            hr = locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &rawServices);

            if (FAILED(hr) || rawServices == nullptr) { return result; }

            _COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
            IWbemServicesPtr services(rawServices, false);

            hr = ::CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
            if (FAILED(hr)) { return result; }

            IEnumWbemClassObject* rawEnumerator = nullptr;
            hr = services->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT ProcessId, CommandLine FROM Win32_Process"),
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &rawEnumerator);

            if (FAILED(hr) || rawEnumerator == nullptr) { return result; }

            _COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
            IEnumWbemClassObjectPtr enumerator(rawEnumerator, false);

            IWbemClassObject* rawObject = nullptr;
            ULONG returned = 0;
            while (enumerator->Next(WBEM_INFINITE, 1, &rawObject, &returned) == S_OK && returned > 0) {
                _COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
                IWbemClassObjectPtr object(rawObject, false);

                VARIANT pidVariant;
                VariantInit(&pidVariant);
                VARIANT cmdVariant;
                VariantInit(&cmdVariant);

                if (SUCCEEDED(object->Get(L"ProcessId", 0, &pidVariant, nullptr, nullptr)) &&
                    SUCCEEDED(object->Get(L"CommandLine", 0, &cmdVariant, nullptr, nullptr))) {
                    DWORD pid = pidVariant.uintVal;
                    if (cmdVariant.vt == VT_BSTR && cmdVariant.bstrVal != nullptr) {
                        result[pid] = std::wstring(cmdVariant.bstrVal, ::SysStringLen(cmdVariant.bstrVal));
                    }
                }

                VariantClear(&pidVariant);
                VariantClear(&cmdVariant);
            }

            return result;
        }   //  queryCommandLinesViaWmi() 

        const std::unordered_set<std::wstring>& criticalProcessNames()
        {
            static const std::unordered_set<std::wstring> names = {
                L"CSRSS.EXE", L"SVCHOST.EXE", L"WINLOGON.EXE", L"LSASS.EXE", L"CONHOST.EXE",
                L"LOGONUI.EXE", L"CMD.EXE", L"SERVICES.EXE", L"SMSS.EXE", L"REGEDIT.EXE",
                L"EXPLORER.EXE", L"USERINIT.EXE", L"TASKMGR.EXE", L"SPOOLSV.EXE", L"IEXPLORE.EXE",
                L"AUDIODG.EXE", L"DWM.EXE", L"LSM.EXE", L"SEARCHFILTERHOST.EXE", L"TASKHOST.EXE",
                L"WININIT.EXE", L"WMPNETWK.EXE", L"WMIPRVSE.EXE", L"WUAUCLT.EXE", L"SPPSVC.EXE",
                L"TRUSTEDINSTALLER.EXE", L"RUNDLL32.EXE",
            };

            return names;
        }   // criticalProcessNames() 


        // This function fetch the version information from a Windows EXE/DLL.
        void queryVersionInfo(const std::wstring& path, std::wstring& companyName, std::wstring& fileDescription)
        {
            if (path.empty())
            {
                return;
            }

            DWORD handle = 0;
            DWORD dwSize = ::GetFileVersionInfoSizeW(path.c_str(), &handle);
            if (dwSize == 0)
            {
                return;
            }

            std::vector<BYTE> buffer(dwSize);
            if (!::GetFileVersionInfoW(path.c_str(), handle, dwSize, buffer.data()))
            {      
                return;
            }

            struct LangCodePage {
                WORD language;
                WORD codePage;
            }*translations = nullptr;

            UINT translationBytes = 0;
            if (!::VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation",
                reinterpret_cast<LPVOID*>(&translations), &translationBytes) || translationBytes < sizeof(LangCodePage)) 
            {
                return;
            }

            wchar_t subBlock[64] = {};
            swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\CompanyName", translations[0].language, translations[0].codePage);
            
            LPVOID value = nullptr;
            UINT valueLen = 0;
            if (::VerQueryValueW(buffer.data(), subBlock, &value, &valueLen) && valueLen > 0)
            {
                companyName.assign(static_cast<wchar_t*>(value));
            }

            swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\FileDescription", translations[0].language, translations[0].codePage);

            value = nullptr;
            valueLen = 0;
            if (::VerQueryValueW(buffer.data(), subBlock, &value, &valueLen) && valueLen > 0) 
            {
                fileDescription.assign(static_cast<wchar_t*>(value));
            }

        }   // queryVersionInfo

        std::wstring queryOwningUser(HANDLE process)
        {

            HANDLE rawToken = nullptr;

            if (!::OpenProcessToken(process, TOKEN_QUERY, &rawToken))
            {
                return L"";
            }

            UniqueHandle token(rawToken);
            DWORD dwNeededSize = 0;

            if (!token.valid())
            {
                return L"";
            }

            ::GetTokenInformation(token.get(), TokenUser, nullptr, 0, &dwNeededSize);

            if (dwNeededSize == 0)
            {
                return L"";
            }

            std::vector<BYTE> buffer(dwNeededSize);
            if(!::GetTokenInformation(token.get(), TokenUser, buffer.data(), dwNeededSize, &dwNeededSize))
            {
                return L"";
            }

            auto* tokenUser = reinterpret_cast<TOKEN_USER*>(buffer.data());

            wchar_t name[256] = {};
            wchar_t domain[256] = {};
            DWORD nameLen = 256;
            DWORD domainLen = 256;
            SID_NAME_USE sidType;
            if (!::LookupAccountSidW(nullptr, tokenUser->User.Sid, name, &nameLen, domain, &domainLen, &sidType)) 
            {
                return L"";
            }

            return std::wstring(domain) + L"\\" + name;
        }   // queryOwningUser


        uint64_t queryMemoryUsageKb(HANDLE process)
        {

            PROCESS_MEMORY_COUNTERS_EX counter{};
            DWORD dwSize = sizeof(counter);

            if (!::GetProcessMemoryInfo(process, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counter), dwSize))
            {
                return 0;
            }

            return ( (static_cast<uint64_t>(counter.WorkingSetSize)) / 1024 );
        }   //queryMemoryUsageKb


        // queryImagePath - Get full process path
        std::wstring queryImagePath(HANDLE process)
        {
            wchar_t buffer[MAX_PATH] = { };
            DWORD size = MAX_PATH;

            // Get full process path
            if (!QueryFullProcessImageNameW(process, false, buffer, &size))
            {
                return L"";     // Failed to fetch exe path so return empty string.
            }

            return std::wstring(buffer, size);
        }   //  queryImagePath(DWORD pid)


	} // namespace

	std::vector<ProcessInfo> ProcessEnumerator::snapshot()
	{
		std::vector<ProcessInfo> vecRunningProcesses;

		// Process snapshot handle.
		UniqueHandle objProcessSnapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));

		if (!objProcessSnapshot.valid()) 
		{
			return vecRunningProcesses;
		}

        // Prcess doesn't provide command lines, that why use com.
		ComInitGuard comGuard;
		std::unordered_map<DWORD, std::wstring> commandLines;

		if (comGuard.initialized())
		{
			commandLines = queryCommandLinesViaWmi();
		}

        // Critical or important system exe's. - DO NOT KILL OR BLOCK THEM.
        const auto& setCriticalProcesses = criticalProcessNames();

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(PROCESSENTRY32W);

        // START ITERATING PROCESS NOW.
        if (!::Process32FirstW(objProcessSnapshot.get(), &entry))
        {
            return vecRunningProcesses;
        }

        do {
            ProcessInfo info;
            info.pid = entry.th32ProcessID;
            info.name = entry.szExeFile;

            // Opens process
            UniqueHandle process(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID));

            if (process.valid())
            {
                info.path = queryImagePath(process.get());                // gets exe path
                info.memoryUsageKb = queryMemoryUsageKb(process.get());   // Memory usage in kb
                info.userName = queryOwningUser(process.get());           // Finds which Windows user account owns/runs that process
            }

            info.isCritical = setCriticalProcesses.count(toUpper(info.name)) > 0;

            auto cmdIt = commandLines.find(entry.th32ProcessID);
            if (cmdIt != commandLines.end()) {
                info.commandLine = cmdIt->second;
            }

            queryVersionInfo(info.path, info.companyName, info.fileDescription);

            vecRunningProcesses.push_back(std::move(info));         // add in vector.

        } while (Process32NextW(objProcessSnapshot.get(), &entry));

		return vecRunningProcesses;
	}
}