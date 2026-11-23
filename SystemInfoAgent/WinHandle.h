#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tlhelp32.h>
#include <utility>
#include <objbase.h>

namespace SystemInfoAgent::core
{
	// RAII wrapper that wraps a HANDLE that must be released with CloseHandle (process, thread, token, Toolhelp32 snapshot...)
	class UniqueHandle {
	private:
		HANDLE handle_ = nullptr;

	public:
		UniqueHandle() noexcept = default;
		explicit UniqueHandle(HANDLE h) noexcept : handle_(h)  { }
		UniqueHandle(const UniqueHandle&) = delete;				// Copy ctor
		UniqueHandle& operator=(const UniqueHandle&) = delete;	// copy assignment op
		UniqueHandle(UniqueHandle&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }	// Move ctor;
		
		UniqueHandle& operator=(UniqueHandle&& other) noexcept	// move assignment op
		{
			if (this == &other) { return *this; }
			reset(other.handle_);
			other.handle_ = nullptr;
			return *this;
		}

		~UniqueHandle() { reset(); }		// dtor

		// isValid
		[[nodiscard]] bool valid() const noexcept
		{
			return (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE);
		}

		// get()
		[[nodiscard]] HANDLE get() const noexcept { return handle_; }

		// reset()
		void reset(HANDLE h = nullptr) {
			if (valid())
			{
				::CloseHandle(handle_);
			}

			handle_ = h;
		}

		// release()
		HANDLE release() noexcept {
			HANDLE h = handle_;
			handle_ = nullptr;
			return h;
		}
	
	};	// class UniqueHandle


	// RAII guard for CoInitializeEx/CoUninitialize (used by the WMI-based command-line lookup and installed-Store-app enumeration).
	class ComInitGuard {
	private:
		HRESULT hr_ = E_FAIL;

	public:
		explicit ComInitGuard(DWORD coInitFlags = COINIT_MULTITHREADED) 
		{
			hr_ = ::CoInitializeEx(nullptr, coInitFlags);
		}

		~ComInitGuard() 
		{
			if (SUCCEEDED(hr_)) 
			{
				::CoUninitialize();
			}
		}

		ComInitGuard(const ComInitGuard&) = delete;
		ComInitGuard& operator=(const ComInitGuard&) = delete;

		[[nodiscard]] bool succeeded() const noexcept { return SUCCEEDED(hr_); }
		[[nodiscard]] HRESULT result() const noexcept { return hr_; }

		[[nodiscard]] bool initialized() const noexcept { return SUCCEEDED(hr_); }

	};	// class ComInitGuard 


	class UniqueServiceHandle {
	private:
		SC_HANDLE handle_ = nullptr;

	public:
		UniqueServiceHandle() noexcept = default;
		explicit UniqueServiceHandle(SC_HANDLE h) noexcept : handle_(h) {}
		UniqueServiceHandle(const UniqueHandle&) = delete;				// Copy ctor
		UniqueServiceHandle& operator=(const UniqueServiceHandle&) = delete;	// copy assignment op
		UniqueServiceHandle(UniqueServiceHandle&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }	// Move ctor;

		UniqueServiceHandle& operator=(UniqueServiceHandle&& other) noexcept	// move assignment op
		{
			if (this == &other) { return *this; }
			reset(other.handle_);
			other.handle_ = nullptr;
			return *this;
		}

		~UniqueServiceHandle() { reset(); }		// dtor

		// isValid
		[[nodiscard]] bool valid() const noexcept
		{
			return (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE);
		}

		// get()
		[[nodiscard]] SC_HANDLE get() const noexcept { return handle_; }

		// reset()
		void reset(SC_HANDLE h = nullptr) {
			if (valid())
			{
				::CloseHandle(handle_);
			}

			handle_ = h;
		}

		// release()
		SC_HANDLE release() noexcept {
			SC_HANDLE h = handle_;
			handle_ = nullptr;
			return h;
		}

	};	// class UniqueServiceHandle

}	// namespace SystemInfoAgent::core

