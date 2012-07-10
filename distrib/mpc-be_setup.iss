; $Id$
;
; (C) 2009-2012 see Authors.txt
;
; This file is part of MPC-BE.
;
; MPC-BE is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; MPC-BE is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

; Requirements:
; Inno Setup Unicode: http://www.jrsoftware.org/isdl.php


; If you want to compile the 64-bit version define "x64build" (uncomment the define below or use build_2010.bat)
#define localize
#define sse_required
;#define x64Build

; Don't forget to update the DirectX SDK number in "include\Version.h" (not updated so often)


; From now on you shouldn't need to change anything

#if VER < EncodeVer(5,5,0)
  #error Update your Inno Setup version (5.5.0 or newer)
#endif

#ifndef UNICODE
  #error Use the Unicode Inno Setup
#endif


#define ISPP_INVOKED
#include "..\include\Version.h"

#define copyright_year "2002-2012"
#define app_name       "MPC-BE"
#define app_version    str(MPC_VERSION_MAJOR) + "." + str(MPC_VERSION_MINOR) + "." + str(MPC_VERSION_STATUS) + "." + str(MPC_VERSION_PATCH)
#define app_version_out    str(MPC_VERSION_MAJOR) + "." + str(MPC_VERSION_MINOR) + "." + str(MPC_VERSION_STATUS) + "." + str(MPC_VERSION_PATCH)+ "." + str(MPC_VERSION_REV)
#define quick_launch   "{userappdata}\Microsoft\Internet Explorer\Quick Launch"


#ifdef x64Build
  #define bindir       = "..\bin\mpc-be_x64"
  #define mpcbe_exe    = "mpc-be64.exe"
  #define mpcbe_ini    = "mpc-be64.ini"
  #define mpcbe_reg    = "mpc-be64"
#else
  #define bindir       = "..\bin\mpc-be_x86"
  #define mpcbe_exe    = "mpc-be.exe"
  #define mpcbe_ini    = "mpc-be.ini"
  #define mpcbe_reg    = "mpc-be"
#endif

[Setup]
#ifdef x64Build
AppId={{FE09AF6D-78B2-4093-B012-FCDAF78693CE}
DefaultGroupName={#app_name} x64
OutputBaseFilename=MPC-BE.{#app_version_out}.x64
UninstallDisplayName={#app_name} {#app_version} x64
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
#else
AppId={{903D098F-DD50-4342-AD23-DA868FCA3126}
DefaultGroupName={#app_name}
OutputBaseFilename=MPC-BE.{#app_version_out}.x86
UninstallDisplayName={#app_name} {#app_version}
#endif
AppName={#app_name}
AppVersion={#app_version}
AppVerName={#app_name} {#app_version}
AppPublisher=MPC-BE Team
AppPublisherURL=https://sourceforge.net/p/mpcbe/
AppSupportURL=https://sourceforge.net/p/mpcbe/
AppUpdatesURL=https://sourceforge.net/p/mpcbe/
AppContact=https://sourceforge.net/p/mpcbe/
AppCopyright=Copyright � {#copyright_year} all contributors, see Authors.txt
VersionInfoCompany=MPC-BE Team
VersionInfoCopyright=Copyright � {#copyright_year}, MPC-BE Team
VersionInfoDescription={#app_name} Setup
VersionInfoProductName={#app_name}
VersionInfoProductVersion={#app_version}
VersionInfoProductTextVersion={#app_version}
VersionInfoTextVersion={#app_version}
VersionInfoVersion={#app_version}
UninstallDisplayIcon={app}\{#mpcbe_exe}
DefaultDirName={code:GetInstallFolder}
LicenseFile=..\docs\COPYING.txt
OutputDir=.
SetupIconFile=..\src\apps\mplayerc\res\icon.ico
AppReadmeFile={app}\Readme.txt
WizardImageFile=WizardImageFile.bmp
WizardSmallImageFile=WizardSmallImageFile.bmp
Compression=lzma2/ultra64
SolidCompression=yes
AllowNoIcons=yes
ShowTasksTreeLines=yes
DisableDirPage=auto
DisableProgramGroupPage=auto
MinVersion=0,5.01.2600sp2
AppMutex=MediaPlayerClassicW
ChangesAssociations=true

[Languages]
Name: en; MessagesFile: compiler:Default.isl

#ifdef localize
Name: br; MessagesFile: compiler:Languages\BrazilianPortuguese.isl
Name: by; MessagesFile: Languages\Belarusian.isl
Name: ca; MessagesFile: compiler:Languages\Catalan.isl
Name: cz; MessagesFile: compiler:Languages\Czech.isl
Name: de; MessagesFile: compiler:Languages\German.isl
Name: es; MessagesFile: compiler:Languages\Spanish.isl
Name: eu; MessagesFile: Languages\Basque.isl
Name: fr; MessagesFile: compiler:Languages\French.isl
Name: he; MessagesFile: compiler:Languages\Hebrew.isl
Name: hu; MessagesFile: compiler:Languages\Hungarian.isl
Name: hy; MessagesFile: Languages\Armenian.islu
Name: it; MessagesFile: compiler:Languages\Italian.isl
Name: ja; MessagesFile: compiler:Languages\Japanese.isl
Name: kr; MessagesFile: Languages\Korean.isl
Name: nl; MessagesFile: compiler:Languages\Dutch.isl
Name: pl; MessagesFile: compiler:Languages\Polish.isl
Name: ru; MessagesFile: compiler:Languages\Russian.isl
Name: sc; MessagesFile: Languages\ChineseSimp.isl
Name: sv; MessagesFile: Languages\Swedish.isl
Name: sk; MessagesFile: Languages\Slovak.isl
Name: tc; MessagesFile: Languages\ChineseTrad.isl
Name: tr; MessagesFile: Languages\Turkish.isl
Name: ua; MessagesFile: compiler:Languages\Ukrainian.isl
#endif

; Include installer's custom messages
#include "custom_messages.iss"


[Messages]
#ifdef x64Build
BeveledLabel={#app_name} {#app_version} x64
#else
BeveledLabel={#app_name} {#app_version}
#endif

[Types]
Name: default;            Description: {cm:types_DefaultInstallation}
Name: custom;             Description: {cm:types_CustomInstallation};                     Flags: iscustom

[Components]
Name: "main"; Description: "{#app_name} {#app_version}"; Types: default custom; Flags: fixed
Name: "mpciconlib"; Description: "{cm:comp_mpciconlib}"; Types: default custom

#ifdef localize
Name: "mpcresources"; Description: "{cm:comp_mpcresources}"; Types: default custom; Flags: disablenouninstallwarning
#endif

Name: "mpcberegvid"; Description: "{cm:AssociationVideo}"; Types: custom; Flags: disablenouninstallwarning
Name: "mpcberegaud"; Description: "{cm:AssociationAudio}"; Types: custom; Flags: disablenouninstallwarning
Name: "mpcberegpl"; Description: "{cm:AssociationPlaylist}"; Types: custom; Flags: disablenouninstallwarning

Name: "mpcbeshellext"; Description: "{cm:comp_mpcbeshellext}"; Types: custom; Flags: disablenouninstallwarning

[Tasks]
Name: desktopicon;              Description: {cm:CreateDesktopIcon};     GroupDescription: {cm:AdditionalIcons}
Name: desktopicon\user;         Description: {cm:tsk_CurrentUser};       GroupDescription: {cm:AdditionalIcons}; Flags: exclusive
Name: desktopicon\common;       Description: {cm:tsk_AllUsers};          GroupDescription: {cm:AdditionalIcons}; Flags: unchecked exclusive
Name: quicklaunchicon;          Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked;             OnlyBelowVersion: 0,6.01

;;ResetSettings
Name: reset_settings;             Description: {cm:tsk_ResetSettings};     GroupDescription: {cm:tsk_Other};       Flags: checkedonce unchecked; Check: SettingsExistCheck()


[Files]
Source: "{#bindir}\{#mpcbe_exe}"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "{#bindir}\mpciconlib.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: mpciconlib
#ifdef x64Build
Source: "{#bindir}\MPCBEShellExt64.dll"; DestDir: "{app}"; Flags: ignoreversion noregerror regserver restartreplace uninsrestartdelete; Components: mpcbeshellext
#else
Source: "{#bindir}\MPCBEShellExt.dll"; DestDir: "{app}"; Flags: ignoreversion noregerror regserver restartreplace uninsrestartdelete; Components: mpcbeshellext
#endif

#ifdef localize
Source: "{#bindir}\Lang\mpcresources.br.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.by.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.ca.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.cz.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.de.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.es.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.eu.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.fr.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.he.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.hu.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.hy.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.it.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.ja.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.kr.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.nl.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.pl.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.ru.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.sc.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.sk.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.sv.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.tc.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.tr.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "{#bindir}\Lang\mpcresources.ua.dll"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\be.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\ca.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\cs.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\de.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\es.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\eu.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\fr.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\hu.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\hy.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\it.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\ja.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\ko.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\nl.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\pl.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\pt-BR.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\ru.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\sk.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\sv.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\tr.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\uk.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\zh-CN.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
Source: "..\src\ExtLib\Mediainfo\Language\zh-TW.csv"; DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
#endif
Source: "..\docs\COPYING.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "..\docs\Authors.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "..\docs\Authors mpc-hc team.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "..\docs\Changelog.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "..\docs\Changelog.Rus.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "..\docs\Readme.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: main

[Icons]
#ifdef x64Build
Name: {group}\{#app_name} x64;                   Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version} x64; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0
Name: {commondesktop}\{#app_name} x64;           Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version} x64; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\common
Name: {userdesktop}\{#app_name} x64;             Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version} x64; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\user
Name: {#quick_launch}\{#app_name} x64;           Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version} x64; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: quicklaunchicon
#else
Name: {group}\{#app_name};                       Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version}; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0
Name: {commondesktop}\{#app_name};               Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version}; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\common
Name: {userdesktop}\{#app_name};                 Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version}; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\user
Name: {#quick_launch}\{#app_name};               Filename: {app}\{#mpcbe_exe}; Comment: {#app_name} {#app_version}; WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: quicklaunchicon
#endif
Name: {group}\Changelog;                         Filename: {app}\Changelog.txt; Comment: {cm:ViewChangelog};                WorkingDir: {app}
Name: {group}\{cm:ProgramOnTheWeb,{#app_name}};  Filename: https://sourceforge.net/p/mpcbe/
Name: {group}\{cm:UninstallProgram,{#app_name}}; Filename: {uninstallexe};      Comment: {cm:UninstallProgram,{#app_name}}; WorkingDir: {app}

[Run]
Filename: "{app}\{#mpcbe_exe}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent unchecked; Description: "{cm:LaunchProgram,{#app_name}}"
Filename: "{app}\Changelog.txt"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent unchecked shellexec; Description: "{cm:ViewChangelog}"
Filename: "{app}\{#mpcbe_exe}"; Parameters: "/regvid"; WorkingDir: "{app}"; Flags: runasoriginaluser runhidden; Components: mpcberegvid
Filename: "{app}\{#mpcbe_exe}"; Parameters: "/regaud"; WorkingDir: "{app}"; Flags: runasoriginaluser runhidden; Components: mpcberegaud
Filename: "{app}\{#mpcbe_exe}"; Parameters: "/regpl"; WorkingDir: "{app}"; Flags: runasoriginaluser runhidden; Components: mpcberegpl

[InstallDelete]
Type: files; Name: {userdesktop}\{#app_name}.lnk;   Check: not IsTaskSelected('desktopicon\user')   and IsUpgrade()
Type: files; Name: {commondesktop}\{#app_name}.lnk; Check: not IsTaskSelected('desktopicon\common') and IsUpgrade()
Type: files; Name: {#quick_launch}\{#app_name}.lnk; Check: not IsTaskSelected('quicklaunchicon')    and IsUpgrade(); OnlyBelowVersion: 0,6.01
Type: files; Name: {app}\AUTHORS
Type: files; Name: {app}\ChangeLog
Type: files; Name: {app}\COPYING
#ifdef localize
; remove the old language dlls when upgrading
Type: files; Name: {app}\mpcresources.*.dll
#endif

[UninstallRun]
Filename: "{app}\{#mpcbe_exe}"; Parameters: "/unregall"; WorkingDir: "{app}"; Flags: runhidden

[Registry]
Root: "HKCU"; Subkey: "Software\MPC-BE\ShellExt"; ValueType: string; ValueName: "MpcPath"; ValueData: "{app}\{#mpcbe_exe}"; Flags: uninsdeletekey; Components: mpcbeshellext

[Code]
#if defined(sse_required) || defined(sse2_required)
function IsProcessorFeaturePresent(Feature: Integer): Boolean;
external 'IsProcessorFeaturePresent@kernel32.dll stdcall';
#endif

const installer_mutex = 'mpcbe_setup_mutex';


function GetInstallFolder(Default: String): String;
var
  sInstallPath: String;
begin
  if not RegQueryStringValue(HKLM, 'SOFTWARE\MPC-BE', 'ExePath', sInstallPath) then begin
    Result := ExpandConstant('{pf}\MPC-BE');
  end
  else begin
    RegQueryStringValue(HKLM, 'SOFTWARE\MPC-BE', 'ExePath', sInstallPath);
    Result := ExtractFileDir(sInstallPath);
    if (Result = '') or not DirExists(Result) then begin
      Result := ExpandConstant('{pf}\MPC-BE');
    end;
  end;
end;


function D3DX9DLLExists(): Boolean;
begin
  if FileExists(ExpandConstant('{sys}\D3DX9_{#DIRECTX_SDK_NUMBER}.dll')) then
    Result := True
  else
    Result := False;
end;


#if defined(sse_required)
function Is_SSE_Supported(): Boolean;
begin
  // PF_XMMI_INSTRUCTIONS_AVAILABLE
  Result := IsProcessorFeaturePresent(6);
end;

#elif defined(sse2_required)

function Is_SSE2_Supported(): Boolean;
begin
  // PF_XMMI64_INSTRUCTIONS_AVAILABLE
  Result := IsProcessorFeaturePresent(10);
end;

#endif


function IsUpgrade(): Boolean;
var
  sPrevPath: String;
begin
  sPrevPath := WizardForm.PrevAppDir;
  Result := (sPrevPath <> '');
end;


// Check if MPC-BE's settings exist
function SettingsExistCheck(): Boolean;
begin
  if RegKeyExists(HKEY_CURRENT_USER, 'Software\MPC-BE') or
  FileExists(ExpandConstant('{app}\{#mpcbe_ini}')) then
    Result := True
  else
    Result := False;
end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  // Hide the License page
  if IsUpgrade() and (PageID = wpLicense) then
    Result := True;
end;


procedure CleanUpSettingsAndFiles();
begin
  DeleteFile(ExpandConstant('{app}\{#mpcbe_ini}'));
  DeleteFile(ExpandConstant('{userappdata}\MPC-BE\default.mpcpl'));
  RemoveDir(ExpandConstant('{userappdata}\MPC-BE'));
  RegDeleteKeyIncludingSubkeys(HKCU, 'Software\MPC-BE Filters');
  RegDeleteKeyIncludingSubkeys(HKCU, 'Software\MPC-BE');
  RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\MPC-BE');
end;


procedure CurStepChanged(CurStep: TSetupStep);
var
  iLanguage: Integer;
begin
  if CurStep = ssPostInstall then begin
    if IsTaskSelected('reset_settings') then
      CleanUpSettingsAndFiles();

    iLanguage := StrToInt(ExpandConstant('{cm:langid}'));
    RegWriteStringValue(HKLM, 'SOFTWARE\MPC-BE', 'ExePath', ExpandConstant('{app}\{#mpcbe_exe}'));

    if IsComponentSelected('mpcresources') and FileExists(ExpandConstant('{app}\{#mpcbe_ini}')) then
      SetIniInt('Settings', 'InterfaceLanguage', iLanguage, ExpandConstant('{app}\{#mpcbe_ini}'))
    else
      RegWriteDWordValue(HKCU, 'Software\MPC-BE\Settings', 'InterfaceLanguage', iLanguage);
  end;

  if (CurStep = ssDone) and not WizardSilent() and not D3DX9DLLExists() then
    SuppressibleMsgBox(CustomMessage('msg_NoD3DX9DLL_found'), mbError, MB_OK, MB_OK);

end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  // When uninstalling, ask the user to delete MPC-BE settings
  if (CurUninstallStep = usUninstall) and SettingsExistCheck() then begin
    if SuppressibleMsgBox(CustomMessage('msg_DeleteSettings'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then
      CleanUpSettingsAndFiles();
  end;
end;


function InitializeSetup(): Boolean;
begin
  // Create a mutex for the installer and if it's already running display a message and stop installation
  if CheckForMutexes(installer_mutex) and not WizardSilent() then begin
    SuppressibleMsgBox(CustomMessage('msg_SetupIsRunningWarning'), mbError, MB_OK, MB_OK);
    Result := False;
  end
  else begin
    Result := True;
    CreateMutex(installer_mutex);

#if defined(sse2_required)
    if not Is_SSE2_Supported() then begin
      SuppressibleMsgBox(CustomMessage('msg_simd_sse2'), mbCriticalError, MB_OK, MB_OK);
      Result := False;
    end;
#elif defined(sse_required)
    if not Is_SSE_Supported() then begin
      SuppressibleMsgBox(CustomMessage('msg_simd_sse'), mbCriticalError, MB_OK, MB_OK);
      Result := False;
    end;
#endif

  end;
end;


function InitializeUninstall(): Boolean;
begin
  if CheckForMutexes(installer_mutex) then begin
    SuppressibleMsgBox(CustomMessage('msg_SetupIsRunningWarning'), mbError, MB_OK, MB_OK);
    Result := False;
  end
  else begin
    Result := True;
    CreateMutex(installer_mutex);
  end;
end;
