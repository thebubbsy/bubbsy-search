// SPDX-License-Identifier: GPL-3.0-or-later
#include "symbols.h"
#include "../common/config.h"
#include <dbghelp.h>
#include <mutex>
#include <string>
#pragma comment(lib, "dbghelp.lib")

namespace bubbsy::sym {
namespace {
std::once_flag g_once;
HANDLE g_proc = nullptr;

void EnsureInit() {
    std::call_once(g_once, [] {
        g_proc = GetCurrentProcess();
        // Cache PDBs locally, download missing ones from the public MS server.
        std::wstring cache = bubbsy::AppDataDir() + L"\\symbols";
        CreateDirectoryW(cache.c_str(), nullptr);
        std::string cacheA(cache.begin(), cache.end());
        std::string search = "srv*" + cacheA + "*https://msdl.microsoft.com/download/symbols";
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_AUTO_PUBLICS);
        SymInitializeA(g_proc, search.c_str(), FALSE);
    });
}
}  // namespace

void* Resolve(HMODULE module, const char* name) {
    if (!module || !name) return nullptr;
    EnsureInit();

    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(module, path, MAX_PATH)) return nullptr;
    MODULEINFO mi{};
    // Load module symbols (idempotent).
    DWORD64 base = SymLoadModuleExW(g_proc, nullptr, path, nullptr, 0, 0, nullptr, 0);
    if (!base) base = (DWORD64)module;

    char buf[sizeof(SYMBOL_INFO) + 1024] = {};
    auto* si = reinterpret_cast<SYMBOL_INFO*>(buf);
    si->SizeOfStruct = sizeof(SYMBOL_INFO);
    si->MaxNameLen = 1024;
    if (SymFromNameW ? false : true) { /* prefer ANSI below */ }
    if (!SymFromName(g_proc, name, si)) return nullptr;
    return reinterpret_cast<void*>(si->Address);
}

void* ResolveCFSFolderGetSize() {
    HMODULE m = LoadLibraryExW(L"windows.storage.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!m) return nullptr;
    // Undecorated form; DbgHelp with SYMOPT_UNDNAME matches these.
    return Resolve(m, "CFSFolder::_GetSize");
}

}  // namespace bubbsy::sym
