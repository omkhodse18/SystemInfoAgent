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
        }

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

        void queryVersionInfo(const std::wstring& path, std::wstring& companyName, std::wstring& fileDescription);
        std::wstring queryOwningUser(DWORD pid);


        uint64_t queryMemoryUsageKb(DWORD pid)
        {
            UniqueHandle process(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid));

            if (!process.valid())
            {
                return 0;
            }

            PROCESS_MEMORY_COUNTERS_EX counter{};
            DWORD dwSize = sizeof(counter);

            if (!::GetProcessMemoryInfo(process.get(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counter), dwSize))
            {
                return 0;
            }

            return ( (static_cast<uint64_t>(counter.WorkingSetSize)) / 1024 );
        }


        // queryImagePath - Get full process path
        std::wstring queryImagePath(DWORD pid)
        {
            UniqueHandle process(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));

            // Process is invalid, so return empty string.
            if (!process.valid()) 
            {
                return L"";
            }

            wchar_t buffer[MAX_PATH] = { };
            DWORD size = MAX_PATH;

            // Get full process path
            if (!QueryFullProcessImageNameW(process.get(), false, buffer, &size))
            {
                return L"";     // Failed to fetch exe path so return empty string.
            }

            return std::wstring(buffer, size);
        }   //  queryImagePath(DWORD pid)


	} // namespace

	std::vector<ProcessInfo> snapshot()
	{
		std::vector<ProcessInfo> vecRunningProcesses;

		// Process snapshot handle.
		UniqueHandle objProcessSnapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));

		if (!objProcessSnapshot.valid()) 
		{
			return vecRunningProcesses;
		}

		ComInitGuard comGuard;
		std::unordered_map<DWORD, std::wstring> commandLines;

		if (comGuard.initialized())
		{
			commandLines = queryCommandLinesViaWmi();
		}

        const auto& setCriticalProcesses = criticalProcessNames();

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32FirstW(objProcessSnapshot.get(), &entry))
        {
            return vecRunningProcesses;
        }

        do {
            ProcessInfo info;
            info.pid = entry.th32ProcessID;
            info.name = entry.szExeFile;
            info.path = queryImagePath(entry.th32ProcessID);                // gets exe path
            info.memoryUsageKb = queryMemoryUsageKb(entry.th32ProcessID);   // Memory usage in kb
            info.userName = queryOwningUser(entry.th32ProcessID);
            info.isCritical = setCriticalProcesses.count(toUpper(info.name)) > 0;

            auto cmdIt = commandLines.find(entry.th32ProcessID);
            if (cmdIt != commandLines.end()) {
                info.commandLine = cmdIt->second;
            }

            queryVersionInfo(info.path, info.companyName, info.fileDescription);

            vecRunningProcesses.push_back(std::move(info));
        } while (Process32NextW(objProcessSnapshot.get(), &entry));

		return vecRunningProcesses;
	}
}