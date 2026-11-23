#pragma once

#include <string>

namespace SystemInfoAgent::Services
{
	class ServiceControl
	{
	public:
		static bool start(const std::wstring& serviceName);
		static bool stop(const std::wstring& serviceName);
		static bool disable(const std::wstring& serviceName);
	};
}