// SPDX-License-Identifier: GPL-3.0-or-later
#include "config.h"
#include <shlobj.h>

namespace bubbsy {
namespace {
std::wstring GetString(const wchar_t* section, const wchar_t* key,
                       const wchar_t* def, const std::wstring& file) {
    wchar_t buf[4096];
    DWORD n = GetPrivateProfileStringW(section, key, def, buf,
                                       (DWORD)std::size(buf), file.c_str());
    return std::wstring(buf, n);
}
bool GetBool(const wchar_t* key, bool def, const std::wstring& file) {
    return GetPrivateProfileIntW(L"bubbsy", key, def ? 1 : 0, file.c_str()) != 0;
}
void PutBool(const wchar_t* key, bool v, const std::wstring& file) {
    WritePrivateProfileStringW(L"bubbsy", key, v ? L"1" : L"0", file.c_str());
}
}  // namespace

std::wstring AppDataDir() {
    PWSTR local = nullptr;
    std::wstring dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local))) {
        dir = local;
        dir += L"\\BubbsySearch";
        CoTaskMemFree(local);
        CreateDirectoryW(dir.c_str(), nullptr);
    }
    return dir;
}

std::wstring ConfigPath() { return AppDataDir() + L"\\config.ini"; }

Settings LoadSettings() {
    const std::wstring f = ConfigPath();
    Settings s;
    s.calculateFolderSizes = GetBool(L"calculateFolderSizes", true, f);
    s.integrateSearch      = GetBool(L"integrateSearch", true, f);
    s.excludeSystemFiles   = GetBool(L"excludeSystemFiles", true, f);
    s.excludeHiddenFiles   = GetBool(L"excludeHiddenFiles", false, f);
    s.sortSizesMixFolders  = GetBool(L"sortSizesMixFolders", true, f);
    s.disableKbOnlySizes   = GetBool(L"disableKbOnlySizes", true, f);
    s.useIecTerms          = GetBool(L"useIecTerms", false, f);
    s.paused               = GetBool(L"paused", false, f);
    s.searchColumns        = GetString(L"bubbsy", L"searchColumns", L"", f);
    s.everythingExePath    = GetString(L"bubbsy", L"everythingExePath", L"", f);
    s.everythingIniPath    = GetString(L"bubbsy", L"everythingIniPath", L"", f);
    s.source = GetPrivateProfileIntW(L"bubbsy", L"everythingSource", 0, f.c_str()) == 1
                   ? EverythingSource::Existing
                   : EverythingSource::Managed;
    return s;
}

void SaveSettings(const Settings& s) {
    const std::wstring f = ConfigPath();
    PutBool(L"calculateFolderSizes", s.calculateFolderSizes, f);
    PutBool(L"integrateSearch", s.integrateSearch, f);
    PutBool(L"excludeSystemFiles", s.excludeSystemFiles, f);
    PutBool(L"excludeHiddenFiles", s.excludeHiddenFiles, f);
    PutBool(L"sortSizesMixFolders", s.sortSizesMixFolders, f);
    PutBool(L"disableKbOnlySizes", s.disableKbOnlySizes, f);
    PutBool(L"useIecTerms", s.useIecTerms, f);
    PutBool(L"paused", s.paused, f);
    WritePrivateProfileStringW(L"bubbsy", L"searchColumns", s.searchColumns.c_str(), f.c_str());
    WritePrivateProfileStringW(L"bubbsy", L"everythingExePath", s.everythingExePath.c_str(), f.c_str());
    WritePrivateProfileStringW(L"bubbsy", L"everythingIniPath", s.everythingIniPath.c_str(), f.c_str());
    WritePrivateProfileStringW(L"bubbsy", L"everythingSource",
                               s.source == EverythingSource::Existing ? L"1" : L"0", f.c_str());
}

}  // namespace bubbsy
