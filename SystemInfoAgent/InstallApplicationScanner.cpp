#include "InstallApplicationScanner.h"
#include "RegistryKey.h"

using SystemInfoAgent::core::RegistryKey;
using SystemInfoAgent::core::RegistryView;

namespace SystemInfoAgent::ApplicationReport 
{
	namespace {

        const std::wstring kUninstallSubKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

        bool isNoiseEntry(const std::wstring& keyName, const RegistryKey& key) {
            if (keyName.rfind(L"KB", 0) == 0)
                return true;
            if (keyName.rfind(L"vs_", 0) == 0)
                return true;
            if (keyName.rfind(L"VS_", 0) == 0)
                return true;
            if (keyName.find(L"Package") != std::wstring::npos)
                return true;

            if (auto systemComponent = key.readDword(L"SystemComponent")) {
                if (*systemComponent != 0)
                    return true;
            }

            return false;
        }


	}	// namespace

    std::wstring InstalledAppScanner::normalizeInstallDate(const std::wstring& raw) 
    {
        // Two observed formats in the wild: 8-char YYYYMMDD, and 10-char
        // YYYY-MM-DD. Both normalize to YYYY/MM/DD for the report. Anything
        // else is passed through unchanged rather than guessed at.
        if (raw.size() == 8) 
        {
            return raw.substr(0, 4) + L"/" + raw.substr(4, 2) + L"/" + raw.substr(6, 2);
        }

        if (raw.size() == 10 && raw[4] == L'-' && raw[7] == L'-') 
        {
            return raw.substr(0, 4) + L"/" + raw.substr(5, 2) + L"/" + raw.substr(8, 2);
        }

        return raw;
    }

    void InstalledAppScanner::scanUninstallKey(HKEY root, const std::wstring& subKey,
                                               const std::wstring& sourceLabel,
                                               std::unordered_map<std::wstring, AppInfo>& results) 
    {
        auto uninstallKey = RegistryKey::open(root, subKey, KEY_READ, RegistryView::Default);
        if (!uninstallKey) { return; }
            
        auto info = uninstallKey->queryInfo();
        if (!info) { return; }

        for (DWORD i = 0; i < info->subKeyCount; ++i) 
        {
            auto keyName = uninstallKey->enumSubKey(i);
            if (!keyName) { continue; }
                
            auto entryKey = RegistryKey::open(root, subKey + L"\\" + *keyName, KEY_READ, RegistryView::Default);
            if (!entryKey) { continue; }

            if (isNoiseEntry(*keyName, *entryKey)) { continue; }

            auto displayName = entryKey->readString(L"DisplayName");

            if (!displayName || displayName->empty())
            {
                continue;
            }

            AppInfo appInfo;
            appInfo.name = *displayName;
            appInfo.publisher = entryKey->readString(L"Publisher").value_or(L"");
            appInfo.version = entryKey->readString(L"DisplayVersion").value_or(L"");
            appInfo.installedDate = normalizeInstallDate(entryKey->readString(L"InstallDate").value_or(L""));
            appInfo.sizeBytes = static_cast<uint64_t>(entryKey->readDword(L"EstimatedSize").value_or(0)) * 1024;
            appInfo.registrySource = sourceLabel;

            // Last-writer-wins when the same display name appears under more
            // than one hive/view: a deliberate simplification over the legacy
            // tool's insert-keeps-first map semantics, since in practice a
            // later-scanned duplicate is more often the authoritative entry
            // (e.g. a per-user install shadowing a stale machine-wide one).
            results[appInfo.name] = appInfo;
        }
    } // void InstalledAppScanner::scanUninstallKey


    std::vector<std::wstring> InstalledAppScanner::enumerateUserSids() {
        std::vector<std::wstring> sids;

        auto usersKey = RegistryKey::open(HKEY_USERS, L"", KEY_READ, RegistryView::Default);
        if (!usersKey) { return sids; }

        auto info = usersKey->queryInfo();
        if (!info) { return sids; }

        for (DWORD i = 0; i < info->subKeyCount; ++i) {
            auto keyName = usersKey->enumSubKey(i);
            if (!keyName) { continue; }

            if (keyName->rfind(L"S-1", 0) == 0) 
            {
                sids.push_back(*keyName);
            }
        }

        return sids;
    }   // std::vector<std::wstring> InstalledAppScanner::enumerateUserSids()

    std::unordered_map<std::wstring, AppInfo> InstalledAppScanner::scanAll() {
        std::unordered_map<std::wstring, AppInfo> results;

        scanUninstallKey(HKEY_LOCAL_MACHINE, kUninstallSubKey, L"HKLM", results);

        auto wow64Key = RegistryKey::open(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            KEY_READ, RegistryView::Default);
        if (wow64Key) {
            scanUninstallKey(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                L"HKLM-WOW64", results);
        }

        for (const auto& sid : enumerateUserSids()) {
            scanUninstallKey(HKEY_USERS, sid + L"\\" + kUninstallSubKey, L"HKU\\" + sid, results);
        }

        return results;
    }   // std::unordered_map<std::wstring, AppInfo> InstalledAppScanner::scanAll()


}	// namespace SystemInfoAgent::ApplicationReport 
