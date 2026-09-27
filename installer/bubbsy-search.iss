; SPDX-License-Identifier: GPL-3.0-or-later
; Bubbsy Search installer (Inno Setup 6)
; Builds a single setup EXE that installs the host + hook DLL, optionally bundles
; a private headless Everything, wires autostart, and restarts Explorer.

#define AppName    "Bubbsy Search"
#define AppVersion "0.1.0"
#define AppPublisher "TheBubbsy"
#define AppExe     "bubbsy-host.exe"

[Setup]
AppId={{B0BB5EEA-5EA2-4C40-9E11-BUBBSYSEARCH01}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL=https://onyachamp.com/
DefaultDirName={autopf}\BubbsySearch
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputBaseFilename=BubbsySearchSetup
Compression=lzma2/max
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
PrivilegesRequired=admin
SetupIconFile=..\assets\bubbsy.ico
UninstallDisplayIcon={app}\{#AppExe}
WizardStyle=modern

[Files]
Source: "..\build\Release\bubbsy-host.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\bubbsy-hook.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\assets\bubbsy.ico";             DestDir: "{app}"; Flags: ignoreversion
; The bundled headless Everything is only laid down when the user says they DON'T
; already have it (see Check). Staged into third_party\everything at packaging time.
Source: "..\third_party\everything\*"; DestDir: "{app}\everything"; \
    Flags: ignoreversion recursesubdirs createallsubdirs; Check: InstallBundledEverything

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[Run]
; Kick off first activation: injects the hook and restarts Explorer.
Filename: "{app}\{#AppExe}"; Parameters: "--setup"; Flags: nowait runasoriginaluser

[UninstallRun]
; Stop the host (which stops our managed Everything and removes autostart).
Filename: "{app}\{#AppExe}"; Parameters: "--uninstall"; Flags: runhidden; RunOnceId: "StopHost"

[Code]
var
  EverythingPage: TInputOptionWizardPage;

procedure InitializeWizard;
begin
  EverythingPage := CreateInputOptionPage(wpSelectDir,
    'voidtools Everything',
    'Bubbsy Search is powered by the Everything search engine.',
    'Do you already have Everything installed on this PC?',
    True, False);   { radio buttons, not checkboxes }
  EverythingPage.Add('No — install a private copy for me (recommended)');
  EverythingPage.Add('Yes — use my existing Everything install');
  EverythingPage.SelectedValueIndex := 0;   { default: install bundled }
end;

{ True when the user chose "No — install a private copy". }
function InstallBundledEverything: Boolean;
begin
  Result := (EverythingPage.SelectedValueIndex = 0);
end;

{ Record the choice into the shared config so the host/DLL know which instance
  to talk to. everythingSource: 0 = Managed, 1 = Existing. }
procedure WriteChoiceToConfig;
var
  Cfg: string;
begin
  Cfg := ExpandConstant('{localappdata}\BubbsySearch\config.ini');
  ForceDirectories(ExpandConstant('{localappdata}\BubbsySearch'));
  if InstallBundledEverything then
  begin
    SetIniString('bubbsy', 'everythingSource', '0', Cfg);
    SetIniString('bubbsy', 'everythingExePath',
      ExpandConstant('{app}\everything\Everything.exe'), Cfg);
    SetIniString('bubbsy', 'everythingIniPath',
      ExpandConstant('{localappdata}\BubbsySearch\Everything.ini'), Cfg);
  end
  else
  begin
    SetIniString('bubbsy', 'everythingSource', '1', Cfg);
    { Existing mode: host auto-detects the default instance; path used only for
      the -search launch, resolved by the host if left blank. }
    SetIniString('bubbsy', 'everythingExePath', '', Cfg);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    WriteChoiceToConfig;
end;
