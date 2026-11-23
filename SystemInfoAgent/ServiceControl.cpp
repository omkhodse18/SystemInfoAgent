#include <Windows.h>

#include "ServiceControl.h"
#include "WinHandle.h"

namespace SystemInfoAgent::Services
{
	using SystemInfoAgent::core::UniqueServiceHandle;

	bool ServiceControl::start(const std::wstring& serviceName)
	{
		UniqueServiceHandle scm(::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));

		if (!scm.valid()) {
			return false;
		}

		UniqueServiceHandle service(::OpenServiceW(scm.get(), serviceName.c_str(), SERVICE_START));
		if (!service.valid()) {
			return false;
		}

		return (::StartServiceW(service.get(), 0, nullptr) != FALSE);
	}

	bool ServiceControl::stop(const std::wstring& serviceName)
	{
		UniqueServiceHandle scm(::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));

		if (!scm.valid()) {
			return false;
		}

		UniqueServiceHandle service(::OpenServiceW(scm.get(), serviceName.c_str(), SERVICE_STOP));
		if (!service.valid()) {
			return false;
		}

		SERVICE_STATUS status;
		return (::ControlService(service.get(), SERVICE_CONTROL_STOP, &status) != FALSE);
	}

	bool ServiceControl::disable(const std::wstring& serviceName)
	{
		UniqueServiceHandle scm(::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));

		if (!scm.valid()) {
			return false;
		}

		UniqueServiceHandle service(::OpenServiceW(scm.get(), serviceName.c_str(), SERVICE_CHANGE_CONFIG));
		if (!service.valid()) {
			return false;
		}

		return (::ChangeServiceConfigW(service.get(), SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE,
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) != FALSE);
	}
}