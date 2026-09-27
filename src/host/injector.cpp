// SPDX-License-Identifier: GPL-3.0-or-later
#include "injector.h"
#include <tlhelp32.h>
#include <vector>

namespace bubbsy {

bool InjectInto(DWORD pid, const std::wstring& dllPath) {
    HANDLE h = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
                           PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION,
                           FALSE, pid);
    if (!h) return false;

    bool ok = false;
    SIZE_T bytes = (dllPath.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(h, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (remote && WriteProcessMemory(h, remote, dllPath.c_str(), bytes, nullptr)) {
        auto k32 = GetModuleHandleW(L"kernel32.dll");
        auto load = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryW");
        HANDLE t = CreateRemoteThread(h, nullptr, 0, load, remote, 0, nullptr);
        if (t) {
            WaitForSingleObject(t, 10000);
            DWORD exitCode = 0;
            GetExitCodeThread(t, &exitCode);  // nonzero HMODULE => loaded
            ok = exitCode != 0;
            CloseHandle(t);
        }
    }
    if (remote) VirtualFreeEx(h, remote, 0, MEM_RELEASE);
    CloseHandle(h);
    return ok;
}

int InjectIntoExplorer(const std::wstring& dllPath) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    int n = 0;
    PROCESSENTRY32W pe{sizeof(pe)};
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
        if (_wcsicmp(pe.szExeFile, L"explorer.exe") == 0)
            if (InjectInto(pe.th32ProcessID, dllPath)) ++n;
    }
    CloseHandle(snap);
    return n;
}

}  // namespace bubbsy
