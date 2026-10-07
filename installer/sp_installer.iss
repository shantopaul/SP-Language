; ==============================================================================
; SP Programming Language - Inno Setup Script
; ==============================================================================
; Generates: sp-setup.exe (Native Windows GUI Installer)
; Installs SP compiler, standard library, runtime, updates PATH and SP_HOME,
; and executes self-configuration & toolchain verification upon install.
; ==============================================================================

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif

[Setup]
AppId={{5A8C9E23-D441-4E60-84E3-8B3FA8902F17}
AppName=SP Programming Language
AppVersion={#AppVersion}
AppPublisher=Shanto Paul
AppPublisherURL=https://splang.shantopaul.com
AppSupportURL=https://docs.splang.shantopaul.com
AppUpdatesURL=https://github.com/shantopaul/SP-Language/releases
DefaultDirName={localappdata}\SP-Language
DefaultGroupName=SP Programming Language
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
OutputDir=..\dist
OutputBaseFilename=sp-setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesEnvironment=yes
ChangesAssociations=yes
PrivilegesRequired=lowest

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "addtopath"; Description: "Add SP compiler and tools to User PATH"; GroupDescription: "Environment Configuration:"
Name: "associate_sp"; Description: "Associate .sp files with SP compiler (Run with SP)"; GroupDescription: "File Associations:"

[Files]
; Copy all files staged by package_distribution.ps1
Source: "..\dist\SP-Language\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\SP Programming Language\SP Shell"; Filename: "powershell.exe"; Parameters: "-NoExit -Command ""$env:SP_HOME=''{app}''; $env:Path=''{app}\bin;'' + $env:Path; Write-Host ''SP Language Environment Active'' -ForegroundColor Cyan; spc doctor"""
Name: "{autoprograms}\SP Programming Language\Uninstall SP Language"; Filename: "{uninstallexe}"
Name: "{autodesktop}\SP Shell"; Filename: "powershell.exe"; Parameters: "-NoExit -Command ""$env:SP_HOME=''{app}''; $env:Path=''{app}\bin;'' + $env:Path; Write-Host ''SP Language Environment Active'' -ForegroundColor Cyan; spc doctor"""; Tasks: desktopicon

[Registry]
; Set SP_HOME in user environment
Root: HKCU; Subkey: "Environment"; ValueType: string; ValueName: "SP_HOME"; ValueData: "{app}"; Flags: preservestringtype
; Associate .sp file extension
Root: HKCU; Subkey: "Software\Classes\.sp"; ValueType: string; ValueName: ""; ValueData: "SPLanguageSource"; Flags: uninsdeletevalue; Tasks: associate_sp
Root: HKCU; Subkey: "Software\Classes\SPLanguageSource"; ValueType: string; ValueName: ""; ValueData: "SP Source File"; Flags: uninsdeletekey; Tasks: associate_sp
Root: HKCU; Subkey: "Software\Classes\SPLanguageSource\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\spc.exe,0"; Tasks: associate_sp
Root: HKCU; Subkey: "Software\Classes\SPLanguageSource\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\spc.exe"" run ""%1"""; Tasks: associate_sp
Root: HKCU; Subkey: "Software\Classes\SPLanguageSource\shell\build\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\spc.exe"" build ""%1"""; Tasks: associate_sp

[Run]
; Run 'spc setup' to auto-detect/provision backend toolchains and verify environment
Filename: "{app}\bin\spc.exe"; Parameters: "setup"; StatusMsg: "Configuring SP Language and verifying toolchain..."; Flags: runhidden
; Run 'spc doctor' check at end of installation (optional for user)
Filename: "{app}\bin\spc.exe"; Parameters: "doctor"; Description: "View system health and diagnostic report"; Flags: postinstall shellexec skipifsilent unchecked

[Code]
// Modify PATH in HKCU\Environment safely without truncating
const
    EnvironmentKey = 'Environment';

procedure AddPathToUserEnv(PathToAdd: string);
var
    OldPath: string;
    NewPath: string;
begin
    if RegQueryStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', OldPath) then
    begin
        if Pos(Uppercase(PathToAdd), Uppercase(OldPath)) = 0 then
        begin
            if (OldPath <> '') and (Copy(OldPath, Length(OldPath), 1) <> ';') then
                OldPath := OldPath + ';';
            NewPath := OldPath + PathToAdd;
            RegWriteStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', NewPath);
        end;
    end
    else
    begin
        RegWriteStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', PathToAdd);
    end;
end;

procedure RemovePathFromUserEnv(PathToRemove: string);
var
    OldPath: string;
    NewPath: string;
    P: Integer;
begin
    if RegQueryStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', OldPath) then
    begin
        P := Pos(Uppercase(PathToRemove), Uppercase(OldPath));
        if P > 0 then
        begin
            Delete(OldPath, P, Length(PathToRemove));
            // Remove double semicolons or trailing semicolons
            StringChange(OldPath, ';;', ';');
            if (OldPath <> '') and (Copy(OldPath, Length(OldPath), 1) = ';') then
                Delete(OldPath, Length(OldPath), 1);
            RegWriteStringValue(HKEY_CURRENT_USER, EnvironmentKey, 'Path', OldPath);
        end;
    end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
    if CurStep = ssPostInstall then
    begin
        if WizardIsTaskSelected('addtopath') then
        begin
            AddPathToUserEnv(ExpandConstant('{app}\bin'));
        end;
    end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
    if CurUninstallStep = usPostUninstall then
    begin
        RemovePathFromUserEnv(ExpandConstant('{app}\bin'));
        RegDeleteValue(HKEY_CURRENT_USER, EnvironmentKey, 'SP_HOME');
    end;
end;
