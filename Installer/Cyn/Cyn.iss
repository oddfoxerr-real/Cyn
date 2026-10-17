#ifndef PayloadDir
  #define PayloadDir "..\..\dist\Cyn"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\dist"
#endif

[Setup]
AppId={{3049F105-E971-49A1-876E-50FDB14EF351}
AppName=Cyn
AppVersion=2.0.48
AppVerName=Cyn 2.0.48
AppPublisher=Cyn contributors
DefaultDirName={autopf}\Cyn
DefaultGroupName=Cyn
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible and not arm64
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
OutputDir={#OutputDir}
OutputBaseFilename=Cyn-Setup-2.0.48-x64
SetupIconFile=..\..\fxsound\Project\icon.ico
UninstallDisplayIcon={app}\Cyn.exe
LicenseFile=..\..\LICENSE
InfoBeforeFile=SETUP-NOTES.txt
Compression=lzma2
SolidCompression=yes
WizardStyle=modern dark
DisableWelcomePage=no
CloseApplications=no
RestartApplications=no
SetupLogging=yes
VersionInfoVersion=2.0.48.0

[Tasks]
Name: desktopicon; Description: "Create a &desktop shortcut"; GroupDescription: "Shortcuts:"

[InstallDelete]
Type: files; Name: "{app}\SPOTIFY.md"

[Files]
Source: "{#PayloadDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "SETUP-NOTES.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "Ensure-AudioDriver.ps1"; Flags: dontcopy
Source: "..\Drivers\Version14\win10\x64\fxvad.inf"; Flags: dontcopy
Source: "..\Drivers\Version14\win10\x64\fxvad.sys"; Flags: dontcopy
Source: "..\Drivers\Version14\win10\x64\fxvadntamd64.cat"; Flags: dontcopy
Source: "..\Drivers\Version14\win10\x64\fxdevcon64.exe"; Flags: dontcopy
Source: "..\Apps\Version14\DfxSetupDrv.exe"; Flags: dontcopy
Source: "runtime-x86\vcruntime140.dll"; Flags: dontcopy

[Icons]
Name: "{autoprograms}\Cyn"; Filename: "{app}\Cyn.exe"; WorkingDir: "{app}"; Parameters: "--view=2"
Name: "{autodesktop}\Cyn"; Filename: "{app}\Cyn.exe"; WorkingDir: "{app}"; Parameters: "--view=2"; Tasks: desktopicon

[Run]
Filename: "{app}\Cyn.exe"; Parameters: "--view=2"; WorkingDir: "{app}"; Description: "Open Cyn"; Flags: nowait postinstall skipifsilent runasoriginaluser

[Code]
var
  DriverReady: Boolean;

function AudioAppBlockReason: String;
var
  Locator, Services, Processes: Variant;
  RunningNames: String;
begin
  Result := '';
  try
    Locator := CreateOleObject('WbemScripting.SWbemLocator');
    Services := Locator.ConnectServer('', 'root\CIMV2');
    RunningNames := '';
    Processes := Services.ExecQuery('SELECT ProcessId FROM Win32_Process WHERE Name="Cyn.exe"');
    if Processes.Count > 0 then RunningNames := 'Cyn';
    Processes := Services.ExecQuery('SELECT ProcessId FROM Win32_Process WHERE Name="FxSound.exe"');
    if Processes.Count > 0 then begin
      if RunningNames <> '' then RunningNames := RunningNames + ' and ';
      RunningNames := RunningNames + 'FxSound';
    end;
    if RunningNames <> '' then
      Result := RunningNames + ' is still running.' + #13#10#13#10 +
        'Choose Exit from its app menu or its icon near the Windows clock. You may need to open the hidden tray icons first.' +
        (#13#10#13#10) + 'Closing the app window only hides it.';
  except
    // Do not misreport a failed process query as a running audio application.
    Log('Audio process check failed: ' + GetExceptionMessage);
    Result := 'Windows could not check whether the audio app is running. Restart setup and try again.';
  end;
end;

function WaitForAudioAppsToExit(SilentMode: Boolean): String;
begin
  Result := AudioAppBlockReason;
  while Result <> '' do begin
    if SilentMode then exit;
    if MsgBox(Result + #13#10#13#10 + 'Click Retry to check again, or Cancel to stop.',
      mbInformation, MB_RETRYCANCEL) <> IDRETRY then begin
      Result := Result + #13#10#13#10 + 'Installation was paused. Click Back to try again or Cancel to exit setup.';
      exit;
    end;
    Result := AudioAppBlockReason;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  Code: Integer;
  Details: AnsiString;
  ResultPath, Params: String;
begin
  Result := WaitForAudioAppsToExit(WizardSilent);
  if Result <> '' then exit;
  if DriverReady then exit;
  ExtractTemporaryFile('Ensure-AudioDriver.ps1');
  ExtractTemporaryFile('fxvad.inf');
  ExtractTemporaryFile('fxvad.sys');
  ExtractTemporaryFile('fxvadntamd64.cat');
  ExtractTemporaryFile('fxdevcon64.exe');
  ExtractTemporaryFile('DfxSetupDrv.exe');
  ExtractTemporaryFile('vcruntime140.dll');
  ResultPath := ExpandConstant('{tmp}\driver-result.txt');
  Params := '-NoProfile -NonInteractive -ExecutionPolicy Bypass -File "' +
    ExpandConstant('{tmp}\Ensure-AudioDriver.ps1') + '" -ResultFile "' + ResultPath + '"';
  WizardForm.StatusLabel.Caption := 'Preparing the audio driver...';
  if not Exec(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'),
    Params, ExpandConstant('{tmp}'), SW_HIDE, ewWaitUntilTerminated, Code) then begin
    Result := 'Could not start audio driver setup. Error: ' + IntToStr(Code);
    exit;
  end;
  Details := '';
  LoadStringFromFile(ResultPath, Details);
  Log(String(Details));
  if Code <> 0 then begin
    Result := 'Audio driver setup could not finish.' + #13#10 + String(Details);
    exit;
  end;
  DriverReady := True;
end;

function InitializeUninstall: Boolean;
begin
  Result := WaitForAudioAppsToExit(UninstallSilent) = '';
end;
