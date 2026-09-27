# Bubbsy Search

**Instant search and real folder sizes inside Windows File Explorer — from a single install.**

Bubbsy Search brings the speed of [voidtools Everything](https://www.voidtools.com/) directly into
File Explorer. Type in the Explorer search box and get results *instantly* instead of waiting on the
slow Windows Search indexer. Folder sizes show up in the details view without freezing Explorer on
huge directories. You install **one** thing — no need to know what "Everything" or "Windhawk" are.

> **Author:** [TheBubbsy](https://github.com/thebubbsy) · [onyachamp.com](https://onyachamp.com/)

---

## What you get

- ⚡ **Instant Explorer search** — the Explorer search box is redirected to the Everything engine.
- 📁 **Real folder sizes** in Details / Tiles / Content views, pulled from the Everything index (no
  slow recursive crawling).
- 🧱 **Custom search-result columns** — choose which columns show up in search results.
- 🙈 **System / hidden file filters** — hide them from results with one checkbox.
- 🧰 **Human-readable sizes** (MB/GB/TB) with optional IEC units (KiB/MiB/GiB).
- 🚀 **Zero setup** — installs, configures, and auto-starts itself. No manual `.ini` editing.

## How it actually works (the honest version)

Everything's index engine is closed-source and always runs as its own process; you can't merge it
into another program. Explorer results require code running *inside* `explorer.exe`. So Bubbsy Search
is **one product the user installs and sees**, made of three cooperating pieces under the hood:

```
┌─ Bubbsy Search (what the user installs) ───────────────────────────────┐
│                                                                        │
│  bubbsy-host.exe  ── tray app + lifecycle manager (the "one thing")    │
│      │                                                                 │
│      ├── launches & supervises a HEADLESS Everything.exe child         │
│      │     (no window, no tray icon of its own; our managed instance)  │
│      │                                                                 │
│      └── injects bubbsy-hook.dll into explorer.exe                     │
│                │                                                       │
│                └── folder sizes · search redirect · custom columns     │
│                    (talks to the Everything child over IPC)            │
└────────────────────────────────────────────────────────────────────────┘
```

- **`bubbsy-host.exe`** — the process the user thinks of as "the app". Runs in the tray, owns
  settings, manages the Everything child, and injects the hook DLL. Auto-starts at login.
- **Headless Everything** — bundled and run with `-startup` and a dedicated instance name so it
  never shows a window and never collides with a copy the user may already run.
- **`bubbsy-hook.dll`** — replaces the old Windhawk dependency. Hooks `CFSFolder::_GetSize`, the
  Explorer search navigation, and the shell registry values that control result columns.

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for detail.

## "Do you already have Everything?"

The installer asks this. If **yes**, Bubbsy Search does *not* install its own copy — it detects and
drives your existing Everything install. If **no**, it installs a private headless copy that only
Bubbsy Search manages.

## Build

Windows + a recent MSVC or clang-cl toolchain and CMake. See [`docs/BUILD.md`](docs/BUILD.md).

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
# then build the installer with Inno Setup 6:
iscc installer\bubbsy-search.iss
```

## Status

Early scaffold. The Everything IPC, the Explorer hooks, and the installer's Everything-choice page
are implemented; the injector, tray host, and lifecycle manager are implemented and need testing on
real Windows targets. This has **not** yet been compiled/signed into a shipping installer.

## Licensing — please read

`bubbsy-hook.dll` is **derived from m417z's GPLv3 Windhawk mod** "Better file sizes in Explorer", so
the hook DLL and this repository are licensed **GPL-3.0-or-later**. See [`LICENSE`](LICENSE) and
[`NOTICE`](NOTICE). voidtools **Everything is not part of this source tree** — it is bundled at
packaging time under **its own license**, unmodified; Bubbsy Search only talks to it via its public
SDK/IPC. You must comply with Everything's redistribution terms when you build an installer that
bundles it.
