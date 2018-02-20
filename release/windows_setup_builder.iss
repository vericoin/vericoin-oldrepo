; Coin Install Script
; Review the Code carefully when adopting to a new coin. 
; 
; written 2014 by fruor
; I'm not responsible for any setup/installer written with the help of this script
; 
; place this file together with a .conf file for the users wallets in the folder
; where your windows binaries reside (the ones users can download)
; open the .iss file with Inno Setup and compile, output will go to a subfolder called \SETUP

; Choose a name that will match Program Files Folder, the Start Menu Items and the Uninstall Info
#define ProgramName "RoxyCoin"

; Enter the Version Number of your binaries
#define VersionNumber "1.7"

; Enter the Name of the Folder created in \Appdata\Roaming where your binaries will place the user files, including wallet, conf file, blockchain info etc.
#define RoamingName "RoxyCoin"

; Enter the Name of the main QT-File
#define QTexe "roxycoin-qt.exe"

; Subfolder with additional files such as an extra folder for daemon
; Do not include \ symbol it will be added later
; If no subfolder is needed use #define SubDir ""
#define SubDir ""

; Enter the Name of the config file you wish to include                 
#define configfile "roxycoin.conf"

[Setup]
AppPublisher=RoxyCoin
AppPublisherURL=http://www.roxycoin.info/
AppName={#ProgramName}
AppVersion={#VersionNumber}
DefaultDirName={pf32}\{#ProgramName}
DefaultGroupName={#ProgramName}
UninstallDisplayIcon={app}\{#QTexe}
Compression=lzma2
SolidCompression=yes
OutputDir=SETUP

[Dirs]
Name: "fonts";

[Files]
Source: "*.exe"; DestDir: "{app}"; Components: main; Excludes: "*.iss"
Source: "*.dll"; DestDir: "{app}"; Components: main; Excludes: "*.iss" 
Source: "fonts\*.ttf"; DestDir: "{app}\fonts"; Components: main;
Source: {#configfile}; DestDir: "{userappdata}\{#RoamingName}"; Components: config; Flags: uninsneveruninstall
Source: "fonts\Lato-Regular.TTF"; DestDir: "{fonts}"; FontInstall: "Lato"; Flags: onlyifdoesntexist uninsneveruninstall

[Icons]
Name: "{group}\{#ProgramName}"; Filename: "{app}\{#QTexe}"
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#ProgramName}"; Filename: "{app}\{#QTexe}"; WorkingDir: "{app}"; Tasks: desktopicon/common
Name: "{userdesktop}\{#ProgramName}"; Filename: "{app}\{#QTexe}"; WorkingDir: "{app}"; Tasks: desktopicon/user

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}" 
Name: desktopicon\common; Description: "For all users"; GroupDescription: "Additional icons:"; Flags: exclusive 
Name: desktopicon\user; Description: "For the current user only"; GroupDescription: "Additional icons:"; Flags: exclusive unchecked

[Components]
Name: main; Description: Main Program; Types: full compact custom; Flags: fixed
Name: config; Description: A config file including nodes to synchronize with the network; Types: full custom; Flags: fixed

[Code]
var
  ImportWalletFileName: String;
  Targetfile: String;

function WalletImport: Boolean;
begin
  Targetfile := ExpandConstant('{userappdata}\{#RoamingName}\wallet.dat');
  if not FileExists(Targetfile) then 
    begin
      Targetfile := ExpandConstant('{app}\wallet.dat');
      if not FileExists(Targetfile) then 
        begin
          if MsgBox('No Wallet has been found. Would you like to import an existing wallet? If not a wallet will be created once {#ProgramName} is started.',mbConfirmation,MB_YESNO) = IDYES then
            begin
              ImportWalletFileName := '';
              if GetOpenFileName('', ImportWalletFileName, '', 'wallet files (*.dat)|*.dat|All Files|*.*', 'dat') then
                begin
                  if not FileExists(Targetfile) then
                    begin
                      FileCopy (expandconstant(ImportWalletFileName), Targetfile, false);
                    end
                  else
                    begin
                      MsgBox('Wallet already exists, skipping import', mbInformation, MB_OK);
                    end;
                end;
            end;
        end;
    end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin 
  Result := True;
  if CurPageID = wpFinished then
     Result := WalletImport;
end;

// UNINSTALL PREVIOUS VERSION //
function GetUninstallString(): String;
var
  sUnInstPath: String;
  sUnInstallString: String;
begin
  sUnInstPath := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\{#emit SetupSetting("AppName")}_is1');
  if sUnInstPath = '' then
    begin
      sUnInstPath := ExpandConstant('Software\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{#emit SetupSetting("AppName")}_is1');
    end;
  sUnInstallString := '';
  if not RegQueryStringValue(HKLM, sUnInstPath, 'UninstallString', sUnInstallString) then
    RegQueryStringValue(HKCU, sUnInstPath, 'UninstallString', sUnInstallString);
  Result := sUnInstallString;
end;

function UnInstallOldVersion(): Integer;
var
  sUnInstallString: String;
  iResultCode: Integer;
begin
// Return Values:
// 1 - uninstall string is empty
// 2 - error executing the UnInstallString
// 3 - successfully executed the UnInstallString

  // default return value
  Result := 0;

  // get the uninstall string of the old app
  sUnInstallString := GetUninstallString();
  if sUnInstallString <> '' then begin
    sUnInstallString := RemoveQuotes(sUnInstallString);
    if Exec(sUnInstallString, '/SILENT /NORESTART /SUPPRESSMSGBOXES','', SW_HIDE, ewWaitUntilTerminated, iResultCode) then
      Result := 3
    else
      Result := 2;
  end else
    Result := 1;
end;

function IsUpgrade(): Boolean;
begin
  Result := (GetUninstallString() <> '');
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep=ssInstall) then
  begin
    if (IsUpgrade()) then
    begin
      UnInstallOldVersion();
    end;
  end;
end;
////////////////////////////////

[Run]
Filename: "{app}\{#QTexe}"; Flags: postinstall skipifsilent nowait
