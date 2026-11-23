#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <algorithm>
#include <unordered_set>

#include "ProcessActions.h"
#include "WinHandle.h"
#include "StringUtil.h"
#include "RegistryKey.h"

namespace SystemInfoAgent::ProcInfo {
	
	using SystemInfoAgent::core::UniqueHandle;
	using SystemInfoAgent::core::toLower;
	using SystemInfoAgent::core::RegistryKey;
	using SystemInfoAgent::core::RegistryView;

	namespace {
		const std::wstring kImageFileExecutionOptionsPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";
	
		std::wstring toUpper(const std::wstring& process)
		{
			std::wstring result;
			std::transform(process.begin(), process.end(), result.begin(),
				[](wchar_t c) { return static_cast<wchar_t>(std::toupper(c)); }
			);
		}

		const std::unordered_set<std::wstring> neverKillProcess()
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
		}
	}

	void ProcessActions::enableDebugPrivilege()
	{
		HANDLE rawToken = nullptr;
		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &rawToken))
			return;

		UniqueHandle token(rawToken);

		LUID luid{};
		if (!::LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid))
			return;

		TOKEN_PRIVILEGES privileges{};
		privileges.PrivilegeCount = 1;
		privileges.Privileges[0].Luid = luid;
		privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		::AdjustTokenPrivileges(token.get(), FALSE, &privileges, sizeof(privileges), nullptr, nullptr);
	}

	bool ProcessActions::kill(const std::wstring& processName) 
	{
		if (isProtected(processName)) {
			return false;
		}

		enableDebugPrivilege();

		UniqueHandle snapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));

		if (!snapshot.valid()) {
			return false;
		}

		std::wstring target = toLower(processName);

		bool isKilledAny = false;
		PROCESSENTRY32W entry;
		DWORD dwSize = sizeof(entry);

		if (!::Process32FirstW(snapshot.get(), &entry)) 
		{	
			return false;
		}

		do {
			if (toLower(entry.szExeFile) == processName)
			{
				UniqueHandle process(::OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID));
				if (process.valid() && ::TerminateProcess(process.get(), 1))
				{
					isKilledAny = true;
				}
			}
		} while (::Process32NextW(snapshot.get(), &entry));

		return isKilledAny;
	}

	bool ProcessActions::block(const std::wstring& processName)
	{
		if (isProtected(processName))
			return false;

		kill(processName);

		auto key = RegistryKey::createOrOpen(HKEY_LOCAL_MACHINE, kImageFileExecutionOptionsPath + L"\\" + processName,
									KEY_WRITE, RegistryView::Default);

		if (!key)
			return false;

		// Any non-existent executable works here; Windows tries to launch it as
		// the "debugger" for the target image instead of the image itself.
		return key->writeString(L"Debugger", L"SysOpsAgent_Blocked.exe");
	}

	bool ProcessActions::unblock(const std::wstring& processName)
	{
		return RegistryKey::deleteTree(HKEY_LOCAL_MACHINE, kImageFileExecutionOptionsPath, processName, RegistryView::Default);
	}

	bool ProcessActions::isProtected(const std::wstring& processName)
	{
		static const std::unordered_set<std::wstring> names = {
				L"CSRSS.EXE", L"SVCHOST.EXE", L"WINLOGON.EXE", L"LSASS.EXE", L"CONHOST.EXE",
				L"LOGONUI.EXE", L"CMD.EXE", L"SERVICES.EXE", L"SMSS.EXE", L"REGEDIT.EXE",
				L"EXPLORER.EXE", L"USERINIT.EXE", L"TASKMGR.EXE", L"SPOOLSV.EXE", L"IEXPLORE.EXE",
				L"AUDIODG.EXE", L"DWM.EXE", L"LSM.EXE", L"SEARCHFILTERHOST.EXE", L"TASKHOST.EXE",
				L"WININIT.EXE", L"WMPNETWK.EXE", L"WMIPRVSE.EXE", L"WUAUCLT.EXE", L"SPPSVC.EXE",
				L"TRUSTEDINSTALLER.EXE", L"RUNDLL32.EXE",
		};

		auto it = names.find(processName);

		return (it != names.end());
	}

} // SystemInfoAgent::procinfo 