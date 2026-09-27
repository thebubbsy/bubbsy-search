// SPDX-License-Identifier: GPL-3.0-or-later
// Shared configuration for bubbsy-host.exe and bubbsy-hook.dll.
#pragma once
#include <windows.h>
#include <string>

namespace bubbsy {

// Named pipe / IPC instance for our privately managed Everything.
inline constexpr wchar_t kEverythingInstance[] = L"BubbsySearch";
// Window class used by the hook DLL's reply window (must be unique to us).
inline constexpr wchar_t kReceiverClass[] = L"BubbsySearch_EverythingReceiver";

enum class EverythingSource {
    Managed,   // we launched a private headless instance (installer: "I don't have Everything")
    Existing,  // use the user's own Everything (installer: "I already have Everything")
};

struct Settings {
    // Feature toggles (mirror the old mod).
    bool calculateFolderSizes = true;
    bool integrateSearch      = true;
    bool excludeSystemFiles   = true;
    bool excludeHiddenFiles   = false;
    bool sortSizesMixFolders  = true;
    bool disableKbOnlySizes   = true;
    bool useIecTerms          = false;

    // Custom search-result columns, as a shell "prop:" template value.
    // Empty => use the built-in default (name; date modified; folder path; size).
    std::wstring searchColumns;

    // Everything wiring.
    EverythingSource source = EverythingSource::Managed;
    std::wstring everythingExePath;   // resolved path to Everything.exe we manage or detected
    std::wstring everythingIniPath;   // our private ini (managed mode only)

    bool paused = false;              // tray "pause" turns off interception without unloading
};

// Path to %LOCALAPPDATA%\BubbsySearch (created if missing).
std::wstring AppDataDir();
// Path to the shared settings file (%LOCALAPPDATA%\BubbsySearch\config.ini).
std::wstring ConfigPath();

// Load/save. Both host and DLL read this; only the host (and installer) writes it.
Settings LoadSettings();
void SaveSettings(const Settings& s);

}  // namespace bubbsy
