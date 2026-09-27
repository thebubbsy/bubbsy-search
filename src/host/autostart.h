// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
namespace bubbsy {
bool SetAutostart(bool enable, const std::wstring& exePath);
bool IsAutostartEnabled();
}  // namespace bubbsy
