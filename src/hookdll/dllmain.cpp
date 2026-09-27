// SPDX-License-Identifier: GPL-3.0-or-later
//
// bubbsy-hook.dll — injected into explorer.exe by bubbsy-host.exe.
// Provides: folder sizes from Everything, Explorer search redirect, custom
// result columns, and IEC size units. Derived from m417z's GPLv3 Windhawk mod;
// the Windhawk runtime is replaced by MinHook + our own symbol resolver.
//
#include <windows.h>
#include <propsys.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <atomic>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "MinHook.h"
#include "symbols.h"
#include "../common/config.h"
#include "../common/everything_ipc.h"

using namespace std::string_view_literals;
using namespace bubbsy;

static Settings g_cfg;
static std::atomic<bool> g_paused{false};

// ---------------------------------------------------------------------------
// Folder size hook: CFSFolder::_GetSize (resolved via symbols.cpp)
// ---------------------------------------------------------------------------
using GetSize_t = HRESULT(WINAPI*)(void*, const ITEMID_CHILD*, const void*, PROPVARIANT*);
static GetSize_t g_origGetSize = nullptr;

static std::wstring PathFromChild(IShellFolder2* parent, const ITEMID_CHILD* child) {
    STRRET sr{};
    if (FAILED(parent->GetDisplayNameOf(child, SHGDN_FORPARSING, &sr))) return {};
    PWSTR name = nullptr;
    if (FAILED(StrRetToStrW(&sr, child, &name)) || !name) return {};
    std::wstring path = name;
    CoTaskMemFree(name);
    constexpr auto pfx = L"\\\\?\\"sv;
    if (path.starts_with(pfx)) path = path.substr(pfx.size());
    return path;
}

static HRESULT WINAPI GetSize_Hook(void* self, const ITEMID_CHILD* child,
                                   const void* idFolder, PROPVARIANT* pv) {
    HRESULT hr = g_origGetSize(self, child, idFolder, pv);
    if (g_paused || !g_cfg.calculateFolderSizes || hr != S_OK || pv->vt != VT_EMPTY)
        return hr;

    IShellFolder2* sf2 = nullptr;
    if (FAILED(((IUnknown*)self)->QueryInterface(IID_IShellFolder2, (void**)&sf2)) || !sf2)
        return S_OK;
    std::wstring path = PathFromChild(sf2, child);
    sf2->Release();
    if (path.empty()) return S_OK;

    const wchar_t* inst = (g_cfg.source == EverythingSource::Managed)
                              ? kEverythingInstance : L"";
    int64_t size = 0;
    if (ev::FolderSize(path.c_str(), inst, &size) == ev::Status::Ok) {
        pv->vt = VT_UI8;
        pv->uhVal.QuadPart = (ULONGLONG)size;
    }
    return S_OK;
}

// ---------------------------------------------------------------------------
// Search redirect: intercept search-ms: navigations via ShellExecuteExW
// ---------------------------------------------------------------------------
using ShellExecuteExW_t = decltype(&ShellExecuteExW);
static ShellExecuteExW_t g_origShellExecuteExW = nullptr;

static std::wstring UrlField(const std::wstring& uri, const wchar_t* key) {
    size_t p = uri.find(key);
    if (p == std::wstring::npos) return {};
    p += wcslen(key);
    size_t e = uri.find(L'&', p);
    std::wstring raw = uri.substr(p, e == std::wstring::npos ? e : e - p);
    wchar_t out[2048]; DWORD n = (DWORD)std::size(out);
    if (SUCCEEDED(UrlUnescapeW(raw.data(), out, &n, 0))) return std::wstring(out, n);
    return raw;
}

static BOOL WINAPI ShellExecuteExW_Hook(SHELLEXECUTEINFOW* ei) {
    if (!g_paused && g_cfg.integrateSearch && ei && ei->lpFile &&
        std::wstring_view(ei->lpFile).starts_with(L"search-ms:"sv)) {
        std::wstring uri = ei->lpFile;
        std::wstring location = UrlField(uri, L"crumb=location:");
        std::wstring query    = UrlField(uri, L"query=");
        if (query.empty()) query = UrlField(uri, L"crumb=");
        if (!query.empty()) {
            ev::LaunchScopedSearch(g_cfg.everythingExePath, location, query,
                                   g_cfg.excludeSystemFiles, g_cfg.excludeHiddenFiles);
            return TRUE;  // swallow the slow Windows Search navigation
        }
    }
    return g_origShellExecuteExW(ei);
}

// ---------------------------------------------------------------------------
// Custom columns: RegQueryValueExW injection of shell view templates
// ---------------------------------------------------------------------------
using RegQueryValueExW_t = decltype(&RegQueryValueExW);
static RegQueryValueExW_t g_origRegQuery = nullptr;

static std::wstring DefaultSearchColumns() {
    return LR"(prop:~System.ItemNameDisplay;System.DateModified;~System.ItemFolderPathDisplay;System.Size)";
}

static LSTATUS WINAPI RegQueryValueExW_Hook(HKEY key, LPCWSTR name, LPDWORD res,
                                            LPDWORD type, LPBYTE data, LPDWORD cb) {
    DWORD inCap = (data && cb) ? *cb : 0;
    LSTATUS r = g_origRegQuery(key, name, res, type, data, cb);
    if (g_paused || !name) return r;

    // We only augment the search-results column template here; other templates
    // (TileInfo/PreviewDetails/StatusBar) can be added the same way.
    if (_wcsicmp(name, L"ContentViewModeForSearch") != 0) return r;

    std::wstring repl = g_cfg.searchColumns.empty() ? DefaultSearchColumns()
                                                     : g_cfg.searchColumns;
    DWORD need = (DWORD)((repl.size() + 1) * sizeof(wchar_t));

    if (!data || !cb) { if (cb) *cb = need; if (type) *type = REG_SZ; return ERROR_SUCCESS; }
    if (inCap < need) { *cb = need; return ERROR_MORE_DATA; }
    memcpy(data, repl.c_str(), need);
    if (type) *type = REG_SZ;
    *cb = need;
    return ERROR_SUCCESS;
}

// ---------------------------------------------------------------------------
// IEC units: append 'i' to K/M/G/T size units when enabled
// ---------------------------------------------------------------------------
using PSStrFormatByteSizeW_t = void*(WINAPI*)(ULONGLONG, LPWSTR, DWORD);
static PSStrFormatByteSizeW_t g_origByteSize = nullptr;

static void* WINAPI PSStrFormatByteSizeW_Hook(ULONGLONG size, LPWSTR text, DWORD cch) {
    void* ret = g_origByteSize(size, text, cch);
    if (g_paused || !g_cfg.useIecTerms || !text || !cch) return ret;
    int len = (int)wcslen(text);
    if (len >= 2 && text[len - 1] == L'B') {
        wchar_t u = text[len - 2];
        if ((u==L'K'||u==L'M'||u==L'G'||u==L'T'||u==L'P') && (DWORD)len + 2 <= cch) {
            text[len - 1] = L'i'; text[len] = L'B'; text[len + 1] = 0;
        }
    }
    return ret;
}

// ---------------------------------------------------------------------------
// Install / uninstall
// ---------------------------------------------------------------------------
template <class T>
static bool Hook(void* target, void* detour, T* orig) {
    return target && MH_CreateHook(target, detour, (void**)orig) == MH_OK;
}

static bool InstallHooks() {
    if (MH_Initialize() != MH_OK) return false;

    g_origGetSize = (GetSize_t)sym::ResolveCFSFolderGetSize();
    if (g_origGetSize)
        Hook(g_origGetSize, (void*)GetSize_Hook, &g_origGetSize);  // note: rebinds orig via trampoline

    HMODULE shell32 = GetModuleHandleW(L"shell32.dll");
    if (auto p = shell32 ? GetProcAddress(shell32, "ShellExecuteExW") : nullptr)
        Hook((void*)p, (void*)ShellExecuteExW_Hook, &g_origShellExecuteExW);

    HMODULE kbase = GetModuleHandleW(L"kernelbase.dll");
    if (auto p = kbase ? GetProcAddress(kbase, "RegQueryValueExW") : nullptr)
        Hook((void*)p, (void*)RegQueryValueExW_Hook, &g_origRegQuery);

    if (g_cfg.useIecTerms) {
        HMODULE ps = GetModuleHandleW(L"propsys.dll");
        if (auto p = ps ? GetProcAddress(ps, (LPCSTR)421) : nullptr)  // PSStrFormatByteSizeW
            Hook((void*)p, (void*)PSStrFormatByteSizeW_Hook, &g_origByteSize);
    }

    return MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(mod);
        g_cfg = LoadSettings();
        g_paused = g_cfg.paused;
        InstallHooks();
    } else if (reason == DLL_PROCESS_DETACH) {
        MH_Uninitialize();
    }
    return TRUE;
}
