// SPDX-License-Identifier: GPL-3.0-or-later
#include "everything_manager.h"
#include "../common/everything_ipc.h"
#include <string>

namespace bubbsy {

void EverythingManager::WriteTunedIni() const {
    const std::wstring& f = cfg_.everythingIniPath;
    if (f.empty()) return;
    auto set = [&](const wchar_t* k, const wchar_t* v) {
        WritePrivateProfileStringW(L"Everything", k, v, f.c_str());
    };
    // Only tune once; if db_cache_size is already ours, skip (avoid clobbering).
    wchar_t cur[64];
    GetPrivateProfileStringW(L"Everything", L"db_cache_size", L"", cur, 64, f.c_str());
    if (wcscmp(cur, L"10737418240") == 0) return;

    set(L"db_cache_size", L"10737418240");            // 10 GB
    set(L"index_size", L"1");
    set(L"index_folder_size", L"1");
    set(L"index_date_modified", L"1");
    set(L"index_attributes", L"1");
    set(L"fast_size_sort", L"1");
    set(L"fast_date_modified_sort", L"1");
    set(L"ntfs_volume_load_usn_journal_on_start", L"1");
    set(L"ntfs_volume_monitor_usn_journal", L"1");
    // Headless: no window, no tray, no update nags.
    set(L"tray_icon", L"0");
    set(L"check_for_updates", L"0");
    set(L"run_on_system_startup", L"0");   // WE manage lifecycle, not Everything's own autostart
}

bool EverythingManager::Start() {
    if (cfg_.source == EverythingSource::Existing)
        return ev::IsAvailable(L"");   // user's own instance

    // Managed headless instance.
    WriteTunedIni();

    std::wstring cmd = L"\"" + cfg_.everythingExePath + L"\""
                       L" -startup"
                       L" -instance " + std::wstring(kEverythingInstance) +
                       L" -config \"" + cfg_.everythingIniPath + L"\"";
    STARTUPINFOW si{sizeof(si)};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    std::wstring mutableCmd = cmd;
    if (!CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &child_))
        return false;
    return true;
}

bool EverythingManager::Healthy() const {
    const wchar_t* inst = (cfg_.source == EverythingSource::Managed)
                              ? kEverythingInstance : L"";
    return ev::IsAvailable(inst);
}

bool EverythingManager::EnsureAlive() {
    if (Healthy()) return true;
    if (cfg_.source == EverythingSource::Existing) return false;  // not ours to launch
    Stop();
    return Start();
}

void EverythingManager::Stop() {
    if (child_.hProcess) {
        // Graceful: tell our instance to quit; fall back to terminate.
        std::wstring cmd = L"\"" + cfg_.everythingExePath + L"\" -instance " +
                           std::wstring(kEverythingInstance) + L" -exit";
        STARTUPINFOW si{sizeof(si)}; PROCESS_INFORMATION pi{};
        std::wstring m = cmd;
        if (CreateProcessW(nullptr, m.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                           nullptr, nullptr, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, 3000);
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
        }
        if (WaitForSingleObject(child_.hProcess, 2000) != WAIT_OBJECT_0)
            TerminateProcess(child_.hProcess, 0);
        CloseHandle(child_.hProcess); CloseHandle(child_.hThread);
        child_ = {};
    }
}

}  // namespace bubbsy
