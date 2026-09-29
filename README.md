# Bubbsy Search

**Instant search and real folder sizes inside Windows File Explorer — from one install.**

Type in the File Explorer search box and get results *instantly* (powered by the
[Everything](https://www.voidtools.com/) engine) instead of waiting on Windows Search. Folder sizes
show up in the details view. You install **one** thing — no need to know what "Everything" or
"Windhawk" are.

> Author: [TheBubbsy](https://github.com/thebubbsy) · [onyachamp.com](https://onyachamp.com/)

---

## ⚠️ Status: no installer exists yet

**Right now there is nothing an average user can download and run.** This repo is the *source code*.
To get the one-click `BubbsySearchSetup.exe`, someone has to compile and sign it on a Windows machine
first (see [Build the installer](#build-the-installer-required-once-to-produce-the-exe) below). Once
that's done and a build is uploaded to **Releases**, the install becomes the 3 steps in the next
section. Until then, ignore any "just install it" expectation — it's not built yet.

---

## Install (for users) — once a release exists

1. Go to the [**Releases**](https://github.com/thebubbsy/bubbsy-search/releases) page and download
   **`BubbsySearchSetup.exe`**.
2. Double-click it. If Windows SmartScreen warns you, click **More info → Run anyway** (it warns for
   any app that isn't signed by a big vendor).
3. Click **Next** through the wizard. When it asks **"Do you already have Everything installed?"**:
   - **No** (most people) → it installs everything for you.
   - **Yes** → it uses your existing Everything.

That's it. Explorer restarts once, and from then on your search box is instant. Bubbsy Search runs
quietly in the system tray (bottom-right, near the clock) and starts automatically with Windows.

**To uninstall:** Settings → Apps → *Bubbsy Search* → Uninstall (or use the Start-menu uninstaller).

---

## For developers

### What it is under the hood
One product the user installs, made of three cooperating parts (Everything's engine is closed-source
and runs as its own process; Explorer results require a DLL inside `explorer.exe`, so a single
process truly isn't possible — this is the honest minimum):

- **`bubbsy-host.exe`** — the tray app the user sees. Supervises a hidden headless Everything, injects
  the hook, handles autostart.
- **`bubbsy-hook.dll`** — runs inside `explorer.exe`: folder sizes, search redirect, custom columns.
- **Installer** — bundles it all and asks the "do you have Everything?" question.

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

### Build the binaries
Needs Windows x64 + Visual Studio 2022 (Desktop C++) + CMake ≥ 3.24. Full detail in
[`docs/BUILD.md`](docs/BUILD.md).
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Build the installer (required once, to produce the .exe)
Needs [Inno Setup 6](https://jrsoftware.org/isdl.php). If you want the bundled-Everything option, drop
a redistributable `Everything.exe` into `third_party/everything/` first (see that folder's README).
```powershell
iscc installer\bubbsy-search.iss
# produces installer\Output\BubbsySearchSetup.exe  — this is the file users download
```
Sign it before sharing so users don't get scary SmartScreen warnings (see `docs/BUILD.md`).

---

## Licensing (important)
The hook DLL is derived from m417z's **GPLv3** Windhawk mod, so this project is
**GPL-3.0-or-later** ([`LICENSE`](LICENSE), [`NOTICE`](NOTICE)). voidtools **Everything is not in this
source tree** — it's bundled at packaging time under its own proprietary license, unmodified. Comply
with its terms if you distribute an installer that includes it.
