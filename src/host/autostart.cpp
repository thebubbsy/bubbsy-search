// SPDX-License-Identifier: GPL-3.0-or-later
#include "autostart.h"
#include <windows.h>
namespace bubbsy {
static constexpr wchar_t kRun[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static constexpr wchar_t kName[] = L"BubbsySearch";

bool SetAutostart(bool enable, const std::wstring& exePath) {
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRun, 0, KEY_SET_VALUE, &k) != ERROR_SUCCESS)
        return false;
    LSTATUS r;
    if (enable) {
        std::wstring v = L"\"" + exePath + L"\" --tray";
        r = RegSetValueExW(k, kName, 0, REG_SZ, (const BYTE*)v.c_str(),
                           (DWORD)((v.size() + 1) * sizeof(wchar_t)));
    } else {
        r = RegDeleteValueW(k, kName);
    }
    RegCloseKey(k);
    return r == ERROR_SUCCESS;
}

bool IsAutostartEnabled() {
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRun, 0, KEY_QUERY_VALUE, &k) != ERROR_SUCCESS)
        return false;
    bool on = RegQueryValueExW(k, kName, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
    RegCloseKey(k);
    return on;
}
}  // namespace bubbsy
