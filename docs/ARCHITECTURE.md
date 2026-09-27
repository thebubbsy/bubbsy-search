# Architecture

## Design constraints (why it's shaped this way)

1. **Everything's engine is closed-source.** Only its process + public SDK/IPC are usable. We cannot
   extract or link its MFT/USN engine. → We *bundle and manage* `Everything.exe`, we don't reimplement it.
2. **Explorer results require in-process code.** To change what File Explorer shows (folder sizes,
   search results, columns) something must run inside `explorer.exe`. → A hook DLL, injected by us.
   This is the job Windhawk used to do; we do it ourselves so Windhawk isn't a dependency.
3. **Users install one thing.** → A single installer + a single visible process (`bubbsy-host.exe`)
   that hides and supervises everything else.

## Components

### `bubbsy-host.exe` (src/host)
The user-facing process. Responsibilities:
- **Tray UI** (`tray.cpp`) — status, open settings, pause/resume, quit.
- **Everything lifecycle** (`everything_manager.cpp`):
  - Decide whether to use a user-supplied Everything or our private headless one (decided at install
    time, recorded in config; re-validated at runtime).
  - Launch our copy headless: `Everything.exe -startup -instance BubbsySearch -config <ourini>`.
  - First-run: write the tuned `Everything.ini` (10 GB `db_cache_size`, folder-size indexing, USN
    monitoring) *before* the first launch, guarded by a machine-wide named mutex.
  - Health-check the IPC pipe; restart the child if it dies.
- **Injector** (`injector.cpp`) — inject `bubbsy-hook.dll` into `explorer.exe` (all shell instances),
  and re-inject when Explorer restarts (watch via shell-hook / process poll).
- **Autostart** (`autostart.cpp`) — register per-user Run key at `HKCU\...\Run`.
- **Settings** (`config.*`) — a single JSON/INI in `%LOCALAPPDATA%\BubbsySearch\config.json`,
  shared with the hook DLL.

### `bubbsy-hook.dll` (src/hookdll)  — GPLv3, derived from the Windhawk mod
Runs inside `explorer.exe`. Responsibilities (carried over from the proven mod, minus the Windhawk
runtime):
- `CFSFolder::_GetSize` hook → folder sizes from Everything IPC.
- `CFSFolder::CompareIDs` hook → optional mixed file/folder size sorting.
- Explorer search interception → route the query to the managed Everything instance.
- `RegQueryValueExW` hook → inject `System.Size` and custom columns into the shell's
  `ContentViewModeForSearch` / `PreviewDetails` / `TileInfo` / `StatusBar` templates.
- Size formatting hooks (`PSStrFormatByteSizeW`, `LoadStringW`) for IEC units.

Because it's injected by our host (not Windhawk), it uses **MinHook** for inline hooks and resolves
the `windows.storage.dll` symbols via the PDB export names at runtime (see `symbols.cpp`).

### Installer (installer/bubbsy-search.iss) — Inno Setup
- Custom wizard page: **"Do you already have Everything installed?"** (Yes / No radio).
  - *No* → installs `third_party/everything` payload into the app dir as the managed instance.
  - *Yes* → skips the payload; host auto-detects the existing instance at runtime.
- Installs `bubbsy-host.exe`, `bubbsy-hook.dll`, assets/logo.
- Registers autostart, launches the host, and **restarts Explorer** so the hook loads immediately.
- UAC-elevated (needs to write Program Files and, for the managed Everything, its service/USN access).

## Instance isolation
Our Everything runs under instance name `BubbsySearch` and a private `Everything.ini`. This guarantees
we never fight a user's own Everything (different named pipe: `\\.\PIPE\Everything IPC (BubbsySearch)`),
and the hook DLL always talks to *our* instance first, falling back to any default instance only when
the user chose "I already have Everything".

## Data flow: an Explorer search
1. User types in Explorer search box → Explorer builds a `search-ms:` navigation.
2. Hook intercepts it, extracts the query + current folder scope.
3. Hook queries the managed Everything over IPC (named pipe / `WM_COPYDATA`), applying the
   system/hidden filters from shared config.
4. Results are presented in Explorer with the user's chosen columns.
