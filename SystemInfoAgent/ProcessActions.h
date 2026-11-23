#pragma once
#include <string>

namespace SystemInfoAgent::ProcInfo {
	
	class ProcessActions {
	private:
		static void enableDebugPrivilege();

	public:
		static bool kill(const std::wstring& processName);
		static bool block(const std::wstring& processName);
		static bool unblock(const std::wstring& processName);
		static bool isProtected(const std::wstring& processName);
	};

}