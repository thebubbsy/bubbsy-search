// SPDX-License-Identifier: GPL-3.0-or-later
// Resolves non-exported windows.storage.dll member functions we need to hook
// (e.g. CFSFolder::_GetSize) using DbgHelp + the Microsoft public symbol server.
// This is the capability Windhawk provided for us before; we now do it ourselves.
#pragma once
#include <windows.h>

namespace bubbsy::sym {

// Resolve one decorated symbol name in a loaded module. Returns nullptr on failure.
// PDBs are downloaded from the MS symbol server on first use and cached under
// %LOCALAPPDATA%\BubbsySearch\symbols. Safe to call once at init.
void* Resolve(HMODULE module, const char* undecoratedOrDecoratedName);

// Convenience: resolve CFSFolder::_GetSize for the running OS build.
void* ResolveCFSFolderGetSize();

}  // namespace bubbsy::sym
