#pragma once

// RAII + typed wrapper over HKEY, replacing the legacy RegFunctions.h
// pattern of free functions taking raw char buffers and manually pairing
// RegOpenKey/RegCloseKey on every call site (and the giant hand-rolled
// SetRegPermission() ACL function).
//
// Supports both the native registry view and the explicit 32-bit view on
// 64-bit Windows (KEY_WOW64_32KEY), replacing the legacy code's duplicated
// "Value" vs "Value64Bit" function pairs.

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <optional>
#include <string>

namespace SystemInfoAgent::core {

    enum class RegistryView {
        Default, // native view: 64-bit on 64-bit Windows, 32-bit on 32-bit
        Force32, // KEY_WOW64_32KEY
        Force64, // KEY_WOW64_64KEY
    };

    class RegistryKey {
    public:
        RegistryKey() noexcept = default;
        ~RegistryKey();

        RegistryKey(const RegistryKey&) = delete;
        RegistryKey& operator=(const RegistryKey&) = delete;
        RegistryKey(RegistryKey&& other) noexcept;
        RegistryKey& operator=(RegistryKey&& other) noexcept;

        // Opens an existing key for the given access mask. Returns std::nullopt
        // on failure (use tryOpen() to get the key, or check valid()).
        static std::optional<RegistryKey> open(HKEY root, const std::wstring& subKey, REGSAM access = KEY_READ,
            RegistryView view = RegistryView::Default);

        // Opens the key, creating it (and any missing parent keys) if absent.
        static std::optional<RegistryKey> createOrOpen(HKEY root, const std::wstring& subKey,
            REGSAM access = KEY_READ | KEY_WRITE,
            RegistryView view = RegistryView::Default);

        [[nodiscard]] bool valid() const noexcept { return handle_ != nullptr; }

        // String (REG_SZ) value read/write.
        [[nodiscard]] std::optional<std::wstring> readString(const std::wstring& valueName) const;
        bool writeString(const std::wstring& valueName, const std::wstring& data) const;

        // DWORD (REG_DWORD) value read/write.
        [[nodiscard]] std::optional<DWORD> readDword(const std::wstring& valueName) const;
        bool writeDword(const std::wstring& valueName, DWORD data) const;

        // Counts of subkeys/values under this key.
        struct KeyInfo {
            DWORD subKeyCount = 0;
            DWORD valueCount = 0;
        };
        [[nodiscard]] std::optional<KeyInfo> queryInfo() const;

        // Enumerates the Nth subkey name (0-based). Returns std::nullopt once
        // index is out of range.
        [[nodiscard]] std::optional<std::wstring> enumSubKey(DWORD index) const;

        // Deletes a named subkey (and recursively, its own subkeys) under this
        // key -- replaces DeleteRegistryKeyAndSubkeys from the legacy code.
        static bool deleteTree(HKEY root, const std::wstring& parentSubKey, const std::wstring& keyToDelete,
            RegistryView view = RegistryView::Default);

        // Convenience one-shot helpers (open + read/write + close in one call),
        // for the common "just get/set one value" case.
        static std::optional<std::wstring> getString(HKEY root, const std::wstring& subKey,
            const std::wstring& valueName,
            RegistryView view = RegistryView::Default);
        static bool setString(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
            const std::wstring& data, RegistryView view = RegistryView::Default);

    private:
        explicit RegistryKey(HKEY handle) noexcept : handle_(handle) {}
        static DWORD viewFlag(RegistryView view);

        HKEY handle_ = nullptr;
    };

} // namespace SystemInfoAgent::core
