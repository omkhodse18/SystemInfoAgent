#include "RegistryKey.h"
#include <vector>

namespace SystemInfoAgent::core {

    DWORD RegistryKey::viewFlag(RegistryView view) {
        switch (view) {
        case RegistryView::Force32:
            return KEY_WOW64_32KEY;
        case RegistryView::Force64:
            return KEY_WOW64_64KEY;
        case RegistryView::Default:
        default:
            return 0;
        }
    }

    RegistryKey::~RegistryKey() {
        if (handle_ != nullptr) {
            ::RegCloseKey(handle_);
        }
    }

    RegistryKey::RegistryKey(RegistryKey&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    RegistryKey& RegistryKey::operator=(RegistryKey&& other) noexcept {
        if (this != &other) {
            if (handle_ != nullptr) {
                ::RegCloseKey(handle_);
            }
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    std::optional<RegistryKey> RegistryKey::open(HKEY root, const std::wstring& subKey, REGSAM access,
        RegistryView view) {
        HKEY h = nullptr;
        LONG status = ::RegOpenKeyExW(root, subKey.c_str(), 0, access | viewFlag(view), &h);
        if (status != ERROR_SUCCESS) {
            return std::nullopt;
        }
        return RegistryKey(h);
    }

    std::optional<RegistryKey> RegistryKey::createOrOpen(HKEY root, const std::wstring& subKey, REGSAM access,
        RegistryView view) {
        HKEY h = nullptr;
        DWORD disposition = 0;
        LONG status = ::RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
            access | viewFlag(view), nullptr, &h, &disposition);
        if (status != ERROR_SUCCESS) {
            return std::nullopt;
        }
        return RegistryKey(h);
    }

    std::optional<std::wstring> RegistryKey::readString(const std::wstring& valueName) const {
        if (!valid())
            return std::nullopt;

        DWORD type = 0;
        DWORD byteSize = 0;
        LONG status = ::RegQueryValueExW(handle_, valueName.c_str(), nullptr, &type, nullptr, &byteSize);
        if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || byteSize == 0) {
            return std::nullopt;
        }

        std::vector<wchar_t> buffer(byteSize / sizeof(wchar_t) + 1, L'\0');
        status = ::RegQueryValueExW(handle_, valueName.c_str(), nullptr, &type,
            reinterpret_cast<LPBYTE>(buffer.data()), &byteSize);
        if (status != ERROR_SUCCESS) {
            return std::nullopt;
        }
        return std::wstring(buffer.data());
    }

    bool RegistryKey::writeString(const std::wstring& valueName, const std::wstring& data) const {
        if (!valid())
            return false;
        DWORD byteSize = static_cast<DWORD>((data.size() + 1) * sizeof(wchar_t));
        return ::RegSetValueExW(handle_, valueName.c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(data.c_str()), byteSize) == ERROR_SUCCESS;
    }

    std::optional<DWORD> RegistryKey::readDword(const std::wstring& valueName) const {
        if (!valid())
            return std::nullopt;
        DWORD value = 0;
        DWORD size = sizeof(DWORD);
        DWORD type = 0;
        LONG status = ::RegQueryValueExW(handle_, valueName.c_str(), nullptr, &type,
            reinterpret_cast<LPBYTE>(&value), &size);
        if (status != ERROR_SUCCESS || type != REG_DWORD) {
            return std::nullopt;
        }
        return value;
    }

    bool RegistryKey::writeDword(const std::wstring& valueName, DWORD data) const {
        if (!valid())
            return false;
        return ::RegSetValueExW(handle_, valueName.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&data),
            sizeof(data)) == ERROR_SUCCESS;
    }

    std::optional<RegistryKey::KeyInfo> RegistryKey::queryInfo() const {
        if (!valid())
            return std::nullopt;
        KeyInfo info;
        LONG status = ::RegQueryInfoKeyW(handle_, nullptr, nullptr, nullptr, &info.subKeyCount, nullptr, nullptr,
            &info.valueCount, nullptr, nullptr, nullptr, nullptr);
        if (status != ERROR_SUCCESS) {
            return std::nullopt;
        }
        return info;
    }

    std::optional<std::wstring> RegistryKey::enumSubKey(DWORD index) const {
        if (!valid())
            return std::nullopt;
        wchar_t name[MAX_PATH] = {};
        DWORD nameLen = MAX_PATH;
        LONG status = ::RegEnumKeyExW(handle_, index, name, &nameLen, nullptr, nullptr, nullptr, nullptr);
        if (status != ERROR_SUCCESS) {
            return std::nullopt;
        }
        return std::wstring(name, nameLen);
    }

    bool RegistryKey::deleteTree(HKEY root, const std::wstring& parentSubKey, const std::wstring& keyToDelete,
        RegistryView view) {
        std::wstring fullPath = parentSubKey + L"\\" + keyToDelete;

        // Recursively remove any children first -- RegDeleteKey (not Ex) will
        // not delete a key that still has subkeys.
        if (auto target = open(root, fullPath, KEY_READ | KEY_WRITE | DELETE, view)) {
            while (auto info = target->queryInfo()) {
                if (info->subKeyCount == 0)
                    break;
                auto childName = target->enumSubKey(0);
                if (!childName)
                    break;
                deleteTree(root, fullPath, *childName, view);
            }
        }

        if (auto parent = open(root, parentSubKey, KEY_READ | KEY_WRITE, view)) {
            return ::RegDeleteKeyW(parent->handle_, keyToDelete.c_str()) == ERROR_SUCCESS;
        }
        return false;
    }

    std::optional<std::wstring> RegistryKey::getString(HKEY root, const std::wstring& subKey,
        const std::wstring& valueName, RegistryView view) {
        auto key = open(root, subKey, KEY_READ, view);
        if (!key)
            return std::nullopt;
        return key->readString(valueName);
    }

    bool RegistryKey::setString(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
        const std::wstring& data, RegistryView view) {
        auto key = createOrOpen(root, subKey, KEY_WRITE, view);
        if (!key)
            return false;
        return key->writeString(valueName, data);
    }

} // namespace SystemInfoAgent::core
