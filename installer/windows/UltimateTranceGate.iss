; Inno Setup script for Ultimate Trance Gate (Windows, VST3, 64-bit).
; Built in CI:  ISCC /DAppVersion=2.0.0 /DSourceDir=<dir with Ultimate Trance Gate.vst3> /DOutDir=dist installer\windows\UltimateTranceGate.iss

#ifndef AppVersion
  #define AppVersion "2.1.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\..\build\UltimateTranceGate_artefacts\Release\VST3"
#endif
#ifndef OutDir
  #define OutDir "..\..\dist"
#endif
#ifndef OutName
  #define OutName "UltimateTranceGate-" + AppVersion + "-Windows-Setup"
#endif

[Setup]
AppId={{6C1E2A47-9B3D-4F0E-8A51-55D0A7C1E7B2}
AppName=Ultimate Trance Gate
AppVersion={#AppVersion}
AppVerName=Ultimate Trance Gate {#AppVersion}
AppPublisher=mikita_laurenau
AppPublisherURL=https://www.instagram.com/mikita_laurenau/
AppSupportURL=https://github.com/Niklauren/Ultimate-Trance-Gate
AppUpdatesURL=https://github.com/Niklauren/Ultimate-Trance-Gate/releases
DefaultDirName={commoncf64}\VST3
DisableDirPage=no
DirExistsWarning=no
AppendDefaultDirName=no
UsePreviousAppDir=yes
DisableProgramGroupPage=yes
Uninstallable=yes
UninstallDisplayName=Ultimate Trance Gate {#AppVersion}
UninstallFilesDir={commonpf64}\mikita_laurenau\Ultimate Trance Gate
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir={#OutDir}
OutputBaseFilename={#OutName}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ShowLanguageDialog=auto
WizardImageFile=wizard.bmp,wizard@2x.bmp
WizardSmallImageFile=wizard-small.bmp,wizard-small@2x.bmp
WizardImageStretch=yes
CloseApplications=no
VersionInfoVersion={#AppVersion}
VersionInfoCompany=mikita_laurenau
VersionInfoDescription=Ultimate Trance Gate installer

[Languages]
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
ru.WelcomeExtra=Ритмический трансгейт: 241 пресет по жанрам, вариации A/B/C, режимы громкости и фильтра.%n%nПлагин VST3 будет установлен в стандартную папку VST3. Перед установкой закрой DAW.%n%ncreated by mikita_laurenau
en.WelcomeExtra=Rhythmic trance gate: 241 genre presets, A/B/C variations, volume and filter gate modes.%n%nThe VST3 plug-in goes into the standard VST3 folder. Please close your DAW before installing.%n%ncreated by mikita_laurenau
ru.DirHint=Папка для VST3. Оставь как есть, если не знаешь, что менять: её находят все DAW.
en.DirHint=VST3 folder. Keep the default unless you know otherwise: every DAW scans it.
ru.Done=Готово. Открой DAW и пересканируй плагины (Ableton: Settings → Plug-ins → Use VST3 Plug-in System Folders → Rescan). Плагин появится у производителя mikita_laurenau.
en.Done=Done. Open your DAW and rescan plug-ins (Ableton: Settings → Plug-ins → Use VST3 Plug-in System Folders → Rescan). The plug-in is listed under mikita_laurenau.
ru.OpenInstagram=Открыть Instagram mikita_laurenau
en.OpenInstagram=Open mikita_laurenau on Instagram

[Messages]
ru.WelcomeLabel2=Будет установлен [name/ver].
en.WelcomeLabel2=This will install [name/ver] on your computer.

[Files]
Source: "{#SourceDir}\Ultimate Trance Gate.vst3\*"; DestDir: "{app}\Ultimate Trance Gate.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[InstallDelete]
; remove an older copy first so no stale files remain inside the bundle
Type: filesandordirs; Name: "{app}\Ultimate Trance Gate.vst3"

[UninstallDelete]
Type: filesandordirs; Name: "{app}\Ultimate Trance Gate.vst3"

[Run]
Filename: "https://www.instagram.com/mikita_laurenau/"; Description: "{cm:OpenInstagram}"; Flags: postinstall shellexec skipifsilent unchecked nowait

[Code]
procedure InitializeWizard;
begin
  WizardForm.WelcomeLabel2.Caption := WizardForm.WelcomeLabel2.Caption + #13#10#13#10 + CustomMessage('WelcomeExtra');
  WizardForm.SelectDirLabel.Caption := CustomMessage('DirHint');
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpFinished then
    WizardForm.FinishedLabel.Caption := CustomMessage('Done');
end;
