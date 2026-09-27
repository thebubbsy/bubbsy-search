// SPDX-License-Identifier: GPL-3.0-or-later
// Minimal client for voidtools Everything.
//   * Folder size via the Everything 1.5 named-pipe IPC (fast path).
//   * Falls back to the 1.4 WM_COPYDATA window IPC.
//   * Scoped search launch via the Everything CLI.
// Derived from m417z's GPLv3 Windhawk mod and the voidtools SDK.
#pragma once
#include <windows.h>
#include <cstdint>
#include <string>

namespace bubbsy::ev {

enum class Status {
    Ok,
    NotRunning,     // no Everything instance answered
    NotIndexed,     // Everything answered but has no size for this path
    ReparsePoint,   // zero-size reparse point (link) — caller may resolve & retry
    Timeout,
};

// Query folder size. `instance` may be empty (default instance) or L"BubbsySearch"
// (our managed one) or L"1.5a". Result in `outSize` on Status::Ok.
Status FolderSize(const wchar_t* folderPath, const wchar_t* instance, int64_t* outSize);

// True if any Everything instance (managed or default) is reachable.
bool IsAvailable(const wchar_t* instance);

// Launch an Everything search window scoped to `folderScope` (may be empty) with
// `terms`, honoring the system/hidden exclusions. Returns false if the exe is missing.
bool LaunchScopedSearch(const std::wstring& everythingExe,
                        const std::wstring& folderScope,
                        const std::wstring& terms,
                        bool excludeSystem, bool excludeHidden);

}  // namespace bubbsy::ev
