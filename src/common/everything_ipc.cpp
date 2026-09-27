// SPDX-License-Identifier: GPL-3.0-or-later
#include "everything_ipc.h"
#include <shellapi.h>
#include <vector>

namespace bubbsy::ev {
namespace {

// ---- Everything 1.5 named-pipe IPC (subset) -------------------------------
constexpr DWORD kCmdGetFolderSize = 18;
constexpr DWORD kRespOk = 200;

struct PipeMsg { DWORD code; DWORD size; };

struct PipeClient {
    HANDLE pipe = INVALID_HANDLE_VALUE;
    HANDLE ev   = nullptr;
    ~PipeClient() {
        if (pipe != INVALID_HANDLE_VALUE) CloseHandle(pipe);
        if (ev) CloseHandle(ev);
    }
    bool Connect(const wchar_t* instance) {
        std::wstring name = L"\\\\.\\PIPE\\Everything IPC";
        if (instance && *instance) { name += L" ("; name += instance; name += L")"; }
        pipe = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (pipe == INVALID_HANDLE_VALUE) return false;
        ev = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        return ev != nullptr;
    }
    bool Io(const void* in, DWORD inLen, void* out, DWORD outCap, DWORD* outLen) {
        PipeMsg hdr{}; DWORD n = 0; OVERLAPPED ov{}; ov.hEvent = ev;
        auto rw = [&](auto fn, void* buf, DWORD len) -> bool {
            ResetEvent(ev);
            if (fn(pipe, buf, len, &n, &ov)) return true;
            if (GetLastError() != ERROR_IO_PENDING) return false;
            WaitForSingleObject(ev, INFINITE);
            return GetOverlappedResult(pipe, &ov, &n, FALSE);
        };
        PipeMsg send{kCmdGetFolderSize, inLen};
        if (!rw(WriteFile, &send, sizeof(send))) return false;
        if (inLen && !rw(WriteFile, const_cast<void*>(in), inLen)) return false;
        if (!rw(ReadFile, &hdr, sizeof(hdr))) return false;
        if (hdr.code != kRespOk || hdr.size > outCap) return false;
        if (hdr.size && !rw(ReadFile, out, hdr.size)) return false;
        if (outLen) *outLen = hdr.size;
        return true;
    }
};

bool PipeFolderSize(const wchar_t* path, const wchar_t* instance, int64_t* out) {
    PipeClient c;
    if (!c.Connect(instance)) return false;
    char utf8[MAX_PATH * 3];
    int len = WideCharToMultiByte(CP_UTF8, 0, path, -1, utf8, sizeof(utf8), nullptr, nullptr);
    if (len <= 0) return false;
    uint64_t v = UINT64_MAX; DWORD got = 0;
    if (!c.Io(utf8, len - 1, &v, sizeof(v), &got) || got != sizeof(v)) return false;
    if (v == UINT64_MAX) return false;   // not indexed
    *out = (int64_t)v;
    return true;
}

bool IsReparse(const wchar_t* p) {
    WIN32_FILE_ATTRIBUTE_DATA fad{};
    return GetFileAttributesExW(p, GetFileExInfoStandard, &fad) &&
           (fad.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
}

}  // namespace

Status FolderSize(const wchar_t* folderPath, const wchar_t* instance, int64_t* outSize) {
    *outSize = 0;
    int64_t v = 0;
    if (PipeFolderSize(folderPath, instance, &v)) {
        *outSize = v;
        if (v == 0 && IsReparse(folderPath)) return Status::ReparsePoint;
        return Status::Ok;
    }
    // (1.4 WM_COPYDATA fallback lives in the hook DLL, which owns a message window.)
    return Status::NotRunning;
}

bool IsAvailable(const wchar_t* instance) {
    PipeClient c;
    return c.Connect(instance);
}

bool LaunchScopedSearch(const std::wstring& everythingExe,
                        const std::wstring& folderScope,
                        const std::wstring& terms,
                        bool excludeSystem, bool excludeHidden) {
    if (everythingExe.empty()) return false;
    std::wstring q = terms;
    if (excludeSystem) q += L" !attrib:s";
    if (excludeHidden) q += L" !attrib:h";
    std::wstring args;
    if (!folderScope.empty()) args += L"-path \"" + folderScope + L"\" ";
    args += L"-search \"" + q + L"\"";
    HINSTANCE r = ShellExecuteW(nullptr, L"open", everythingExe.c_str(),
                                args.c_str(), nullptr, SW_SHOWNORMAL);
    return (INT_PTR)r > 32;
}

}  // namespace bubbsy::ev
