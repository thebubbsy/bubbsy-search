// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <windows.h>
#include <string>
namespace bubbsy {
// Inject `dllPath` into a single process by PID (CreateRemoteThread + LoadLibraryW).
bool InjectInto(DWORD pid, const std::wstring& dllPath);
// Inject into every running explorer.exe shell process. Returns count injected.
int InjectIntoExplorer(const std::wstring& dllPath);
}  // namespace bubbsy
