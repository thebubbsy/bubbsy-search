// SPDX-License-Identifier: GPL-3.0-or-later
//
// bubbsy-host.exe — the single user-facing process.
//  * Supervises the managed headless Everything instance.
//  * Injects bubbsy-hook.dll into explorer.exe (and re-injects on restart).
//  * Lives in the tray; owns settings and autostart.
//
#include <windows.h>
#include <shellapi.h>
#include <string>
#include "injector.h"
#include "everything_manager.h"
#include "autostart.h"
#include "../common/config.h"

using namespace bubbsy;

namespace {
constexpr UINT WM_TRAY   = WM_APP + 1;
constexpr UINT ID_QUIT   = 1;
constexpr UINT ID_PAUSE  = 2;
constexpr UINT ID_OPEN   = 3;
constexpr UINT TIMER_SUPERVISE = 100;

Settings          g_cfg;
EverythingManager* g_ev = nullptr;
NOTIFYICONDATAW   g_nid{};
std::wstring      g_dllPath;
std::wstring      g_exePath;

std::wstring DirOf(const std::wstring& p) {
    size_t s = p.find_last_of(L"\\/");
    return s == std::wstring::npos ? L"" : p.substr(0, s);
}

void RestartExplorer() {
    // Ask Explorer to exit cleanly, then relaunch the shell. Windows respawns it
    // automatically in most configs; we relaunch to be safe.
    system("taskkill /f /im explorer.exe >nul 2>&1");
    Sleep(500);
    ShellExecuteW(nullptr, L"open", L"explorer.exe", nullptr, nullptr, SW_SHOW);
}

void AddTray(HWND hwnd) {
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAY;
    g_nid.hIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(101),
                                    IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    if (!g_nid.hIcon) g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"Bubbsy Search");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void ShowMenu(HWND hwnd) {
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, ID_OPEN, L"Settings…");
    AppendMenuW(m, MF_STRING | (g_cfg.paused ? MF_CHECKED : 0), ID_PAUSE, L"Pause interception");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, ID_QUIT, L"Quit Bubbsy Search");
    POINT pt; GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    UINT cmd = TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(m);
    switch (cmd) {
        case ID_OPEN:
            ShellExecuteW(nullptr, L"open", ConfigPath().c_str(), nullptr, nullptr, SW_SHOW);
            break;
        case ID_PAUSE:
            g_cfg.paused = !g_cfg.paused;
            SaveSettings(g_cfg);
            break;
        case ID_QUIT:
            PostQuitMessage(0);
            break;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_TRAY:
            if (LOWORD(lp) == WM_RBUTTONUP || LOWORD(lp) == WM_LBUTTONUP) ShowMenu(hwnd);
            return 0;
        case WM_TIMER:
            if (wp == TIMER_SUPERVISE) {
                g_ev->EnsureAlive();
                // Re-inject any explorer instance that isn't hooked yet. Injection is
                // idempotent (LoadLibraryW ref-counts; the DLL early-outs if present).
                InjectIntoExplorer(g_dllPath);
            }
            return 0;
        case WM_DESTROY:
            Shell_NotifyIconW(NIM_DELETE, &g_nid);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
}  // namespace

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR cmdLine, int) {
    // Single-instance guard.
    HANDLE mtx = CreateMutexW(nullptr, TRUE, L"Local\\BubbsySearchHost");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    g_exePath = self;
    g_dllPath = DirOf(g_exePath) + L"\\bubbsy-hook.dll";

    g_cfg = LoadSettings();
    // Resolve default paths on first launch if the installer didn't set them.
    if (g_cfg.everythingIniPath.empty())
        g_cfg.everythingIniPath = AppDataDir() + L"\\Everything.ini";
    if (g_cfg.source == EverythingSource::Managed && g_cfg.everythingExePath.empty())
        g_cfg.everythingExePath = DirOf(g_exePath) + L"\\everything\\Everything.exe";
    SaveSettings(g_cfg);

    g_ev = new EverythingManager(g_cfg);
    g_ev->Start();

    SetAutostart(true, g_exePath);

    // First activation (installer passes --setup) restarts Explorer so the hook loads now.
    if (wcsstr(cmdLine, L"--setup")) {
        InjectIntoExplorer(g_dllPath);
        RestartExplorer();
    } else {
        InjectIntoExplorer(g_dllPath);
    }

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"BubbsySearchHostWnd";
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Bubbsy Search", 0, 0, 0, 0, 0,
                              HWND_MESSAGE, nullptr, hInst, nullptr);
    AddTray(hwnd);
    SetTimer(hwnd, TIMER_SUPERVISE, 5000, nullptr);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) { TranslateMessage(&m); DispatchMessageW(&m); }

    g_ev->Stop();
    delete g_ev;
    ReleaseMutex(mtx);
    return 0;
}
