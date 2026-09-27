# Building Bubbsy Search

## Prerequisites
- Windows 10/11 x64
- Visual Studio 2022 (Desktop C++) **or** LLVM/clang-cl
- CMake ≥ 3.24
- [Inno Setup 6](https://jrsoftware.org/isdl.php) (for the installer)
- MinHook — either vendored into `third_party/minhook` or fetched by CMake (FetchContent)

## Compile the binaries
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
Outputs:
- `build/Release/bubbsy-host.exe`
- `build/Release/bubbsy-hook.dll`

## Stage the bundled Everything (only if you ship the "download it for me" path)
Place a redistributable `Everything.exe` (or `Everything64.exe`) under
`third_party/everything/`. This is **not committed** (see `.gitignore`) and must be obtained from
voidtools under their license.

```
third_party/everything/
    Everything.exe
    (any DLLs it needs)
```

## Build the installer
```powershell
iscc installer\bubbsy-search.iss
# -> installer\Output\BubbsySearchSetup.exe
```

## Code signing (recommended before distribution)
Normal users get a SmartScreen warning for unsigned installers. Sign both the EXE/DLL and the final
installer with an OV/EV code-signing certificate:
```powershell
signtool sign /fd SHA256 /a /tr http://timestamp.digicert.com /td SHA256 build\Release\*.exe build\Release\*.dll
signtool sign /fd SHA256 /a /tr http://timestamp.digicert.com /td SHA256 installer\Output\BubbsySearchSetup.exe
```
